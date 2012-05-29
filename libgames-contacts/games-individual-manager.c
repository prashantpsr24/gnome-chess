/* games-individual-manager.c: Store, manage and ref Folks individuals
 *  who contain a GamesContact
 *
 * Copyright © 2012 Chandni Verma
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Chandni Verma <chandniverma2112@gmail.com>
 */

#include <folks/folks-telepathy.h>

#include "games-contact.h"
#include "games-individual-manager.h"

#define GAMES_INDIVIDUAL_MANAGER_GET_PRIV(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GAMES_TYPE_INDIVIDUAL_MANAGER, \
      GamesIndividualManagerPriv))

/* Much of code has been influenced by Empathy, a chat
 * application based on Telepathy (https://live.gnome.org/Empathy)
 * Copyright (C) 2007-2010 Collabora Ltd.
 * */

struct _GamesIndividualManagerPriv{
  FolksIndividualAggregator *aggregator;

  /* individual.id -> individual map */
  GHashTable *individuals;

  gboolean contacts_loaded;
};

enum {
  GROUPS_CHANGED,
  FAVOURITES_CHANGED,
  MEMBERS_CHANGED,
  CONTACTS_LOADED,
  LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 }; 

G_DEFINE_TYPE (GamesIndividualManager, games_individual_manager, G_TYPE_OBJECT);

static GamesIndividualManager *manager_singleton = NULL;

static GObject *
individual_manager_constructor (GType type,
                             guint n_construct_properties,
                             GObjectConstructParam *construct_properties)
{
  GObject *retval;

  if (manager_singleton == NULL)
  {
    retval = G_OBJECT_CLASS (games_individual_manager_parent_class)->
        constructor (type, n_construct_properties, construct_properties);

    manager_singleton = GAMES_INDIVIDUAL_MANAGER (retval);
    g_object_add_weak_pointer (retval, (gpointer) &manager_singleton);
  }
  else
  {
    retval = g_object_ref (manager_singleton);
  }

  return retval;
}

static void
individual_group_changed_cb (FolksIndividual *individual,
    gchar *group,
    gboolean is_member,
    GamesIndividualManager *self)
{
  g_signal_emit (self, signals[GROUPS_CHANGED], 0, individual, group,
      is_member);
}

static void
individual_notify_is_favourite_cb (FolksIndividual *individual,
    GParamSpec *pspec,
    GamesIndividualManager *self)
{
  gboolean is_favourite = folks_favourite_details_get_is_favourite (
      FOLKS_FAVOURITE_DETAILS (individual));
  g_signal_emit (self, signals[FAVOURITES_CHANGED], 0, individual,
      is_favourite);
}

static void
add_individual (GamesIndividualManager *self, FolksIndividual *individual)
{
  GamesIndividualManagerPriv *priv = GAMES_INDIVIDUAL_MANAGER_GET_PRIV (self);

  g_hash_table_insert (priv->individuals,
      g_strdup (folks_individual_get_id (individual)),
      g_object_ref (individual));

  g_signal_connect (individual, "group-changed",
      G_CALLBACK (individual_group_changed_cb), self);
  g_signal_connect (individual, "notify::is-favourite",
      G_CALLBACK (individual_notify_is_favourite_cb), self);
}

static void
remove_individual (GamesIndividualManager *self, FolksIndividual *individual)
{
  GamesIndividualManagerPriv *priv = GAMES_INDIVIDUAL_MANAGER_GET_PRIV (self);

  g_signal_handlers_disconnect_by_func (individual,
      individual_group_changed_cb, self);
  g_signal_handlers_disconnect_by_func (individual,
      individual_notify_is_favourite_cb, self);

  g_hash_table_remove (priv->individuals, folks_individual_get_id (individual));
}

/* Returns TRUE if the given Individual contains a TpContact */
static gboolean
individual_contains_tp_contact (FolksIndividual *individual)
{
  GeeSet *personas;
  GeeIterator *iter;
  gboolean retval = FALSE;

  g_return_val_if_fail (FOLKS_IS_INDIVIDUAL (individual), FALSE);

  personas = folks_individual_get_personas (individual);
  iter = gee_iterable_iterator (GEE_ITERABLE (personas));
  while (!retval && gee_iterator_next (iter))
    {
      FolksPersona *persona = gee_iterator_get (iter);
      TpContact *contact = NULL;

      if (games_folks_persona_is_interesting (persona))
        contact = tpf_persona_get_contact (TPF_PERSONA (persona));

      g_clear_object (&persona);

      if (contact != NULL)
        retval = TRUE;
    }
  g_clear_object (&iter);

  return retval;
}

/* This is emitted for *all* individuals in the individual aggregator (not
 * just the ones we keep a reference to), to allow for the case where a new
 * individual doesn't contain a GamesContact, but later has a persona added
 * which does. */
static void
individual_notify_personas_cb (FolksIndividual *individual,
                               GParamSpec *pspec,
                               GamesIndividualManager *self)
{
  GamesIndividualManagerPriv *priv = GAMES_INDIVIDUAL_MANAGER_GET_PRIV (self);

  const gchar *id = folks_individual_get_id (individual);
  gboolean has_contact = individual_contains_tp_contact (individual);
  gboolean had_contact = (g_hash_table_lookup (priv->individuals,
      id) != NULL) ? TRUE : FALSE;

  if (had_contact == TRUE && has_contact == FALSE)
    {
      GList *removed = NULL;

      /* The Individual has lost its GamesContact */
      removed = g_list_prepend (removed, individual);
      g_signal_emit (self, signals[MEMBERS_CHANGED], 0, NULL, NULL, removed,
          TP_CHANNEL_GROUP_CHANGE_REASON_NONE /* FIXME */);
      g_list_free (removed);

      remove_individual (self, individual);
    }
  else if (had_contact == FALSE && has_contact == TRUE)
    {
      GList *added = NULL;

      /* The Individual has gained its first GamesContact */
      add_individual (self, individual);

      added = g_list_prepend (added, individual);
      g_signal_emit (self, signals[MEMBERS_CHANGED], 0, NULL, added, NULL,
          TP_CHANNEL_GROUP_CHANGE_REASON_NONE /* FIXME */);
      g_list_free (added);
    }
}

static void
aggregator_individuals_changed_cb (FolksIndividualAggregator *aggregator,
    GeeMultiMap *changes,
    GamesIndividualManager *self)
{
  GamesIndividualManagerPriv *priv = GAMES_INDIVIDUAL_MANAGER_GET_PRIV (self);
  GeeIterator *iter;
  GeeSet *removed;
  GeeCollection *added;
  GList *added_set = NULL, *added_filtered = NULL, *removed_list = NULL;

  /* We're not interested in the relationships between the added and removed
   * individuals, so just extract collections of them. Note that the added
   * collection may contain duplicates, being a collection, while the removed
   * set won't. */
  removed = gee_multi_map_get_keys (changes);
  added = gee_multi_map_get_values (changes);

  /* Handle the removals first, as one of the added Individuals might have the
   * same ID as one of the removed Individuals (due to linking). */
  iter = gee_iterable_iterator (GEE_ITERABLE (removed));
  while (gee_iterator_next (iter))
    {
      FolksIndividual *ind = gee_iterator_get (iter);

      if (ind == NULL)
        continue;

      g_signal_handlers_disconnect_by_func (ind,
          individual_notify_personas_cb, self);

      if (g_hash_table_lookup (priv->individuals,
          folks_individual_get_id (ind)) != NULL)
        {
          remove_individual (self, ind);
          removed_list = g_list_prepend (removed_list, ind);
        }

      g_clear_object (&ind);
    }
  g_clear_object (&iter);

  /* Filter the individuals for ones which contain GamesContacts */
  iter = gee_iterable_iterator (GEE_ITERABLE (added));
  while (gee_iterator_next (iter))
    {
      FolksIndividual *ind = gee_iterator_get (iter);

      /* Make sure we handle each added individual only once. */
      if (ind == NULL || g_list_find (added_set, ind) != NULL)
        continue;
      added_set = g_list_prepend (added_set, ind);

      g_signal_connect (ind, "notify::personas",
          G_CALLBACK (individual_notify_personas_cb), self);

      /* We add all individuals having TpContacts since this set of 
       * individuals and the manager singleton is common to all games.
       * We leave filtering of individuals having a specific gaming
       * capability upto respective GamesIndividualViews */
      if (individual_contains_tp_contact (ind))
        {
          add_individual (self, ind);
          added_filtered = g_list_prepend (added_filtered, ind);
        }

      g_clear_object (&ind);
    }
  g_clear_object (&iter);

  g_list_free (added_set);

  g_object_unref (added);
  g_object_unref (removed);

  /* Bail if we have no individuals left */
  if (added_filtered == NULL && removed == NULL)
    return;

  added_filtered = g_list_reverse (added_filtered);

  g_signal_emit (self, signals[MEMBERS_CHANGED], 0, NULL,
      added_filtered, removed_list,
      TP_CHANNEL_GROUP_CHANGE_REASON_NONE,
      TRUE);

  g_list_free (added_filtered);
  g_list_free (removed_list);
}

static void
aggregator_is_quiescent_notify_cb (FolksIndividualAggregator *aggregator,
                                   GParamSpec *spec,
                                   GamesIndividualManager *self)
{
  GamesIndividualManagerPriv *priv = GAMES_INDIVIDUAL_MANAGER_GET_PRIV (self);
  gboolean is_quiescent;

  if (priv->contacts_loaded)
    return;

  g_object_get (aggregator, "is-quiescent", &is_quiescent, NULL);

  if (!is_quiescent)
    return;

  priv->contacts_loaded = TRUE;

  g_signal_emit (self, signals[CONTACTS_LOADED], 0);
}

static void
individual_manager_dispose (GObject *object)
{
  GamesIndividualManagerPriv *priv = GAMES_INDIVIDUAL_MANAGER_GET_PRIV (object);

  g_hash_table_unref (priv->individuals);

  g_signal_handlers_disconnect_by_func (priv->aggregator,
      aggregator_individuals_changed_cb, object);
  g_signal_handlers_disconnect_by_func (priv->aggregator,
      aggregator_is_quiescent_notify_cb, object);
  tp_clear_object (&priv->aggregator);

  G_OBJECT_CLASS (games_individual_manager_parent_class)->dispose (object);
}

static void
games_individual_manager_class_init (GamesIndividualManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GamesIndividualManagerPriv));

  object_class->dispose = individual_manager_dispose;
  object_class->constructor = individual_manager_constructor;

  signals[GROUPS_CHANGED] =
      g_signal_new ("groups-changed",
          G_TYPE_FROM_CLASS (klass),
          G_SIGNAL_RUN_LAST,
          0,
          NULL, NULL,
          g_cclosure_marshal_generic,
          G_TYPE_NONE, 3, FOLKS_TYPE_INDIVIDUAL, G_TYPE_STRING, G_TYPE_BOOLEAN);

  signals[FAVOURITES_CHANGED] =
      g_signal_new ("favourites-changed",
          G_TYPE_FROM_CLASS (klass),
          G_SIGNAL_RUN_LAST,
          0,
          NULL, NULL,
          g_cclosure_marshal_generic,
          G_TYPE_NONE, 2, FOLKS_TYPE_INDIVIDUAL, G_TYPE_BOOLEAN);

  signals[MEMBERS_CHANGED] =
      g_signal_new ("members-changed",
          G_TYPE_FROM_CLASS (klass),
          G_SIGNAL_RUN_LAST,
          0,
          NULL, NULL,
          g_cclosure_marshal_generic,
          G_TYPE_NONE,
          4, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_UINT);

  signals[CONTACTS_LOADED] =
      g_signal_new ("contacts-loaded",
          G_TYPE_FROM_CLASS (klass),
          G_SIGNAL_RUN_LAST,
          0,
          NULL, NULL,
          g_cclosure_marshal_generic,
          G_TYPE_NONE,
          0);
}

static void
games_individual_manager_init (GamesIndividualManager *self)
{
  GamesIndividualManagerPriv *priv;

  self->priv = priv = GAMES_INDIVIDUAL_MANAGER_GET_PRIV (self);

  priv->individuals = g_hash_table_new_full (g_str_hash, g_str_equal,
      g_free, g_object_unref);

  priv->aggregator = folks_individual_aggregator_new ();
  g_signal_connect (priv->aggregator, "individuals-changed-detailed",
      G_CALLBACK (aggregator_individuals_changed_cb), self);
  g_signal_connect (priv->aggregator, "notify::is-quiescent",
      G_CALLBACK (aggregator_is_quiescent_notify_cb), self);

  /* Prepare individual aggregator for use */
  folks_individual_aggregator_prepare (priv->aggregator, NULL, NULL);
}

GamesIndividualManager *
games_individual_manager_dup_singleton (void)
{
  return g_object_new (GAMES_TYPE_INDIVIDUAL_MANAGER, NULL);
}

GList *
games_individual_manager_get_members (GamesIndividualManager *self)
{
  GamesIndividualManagerPriv *priv = GAMES_INDIVIDUAL_MANAGER_GET_PRIV (self);

  g_return_val_if_fail (GAMES_IS_INDIVIDUAL_MANAGER (self), NULL);

  return g_hash_table_get_values (priv->individuals);
}

gboolean
games_individual_manager_get_contacts_loaded (GamesIndividualManager *self)
{
  GamesIndividualManagerPriv *priv = GAMES_INDIVIDUAL_MANAGER_GET_PRIV (self);

  return priv->contacts_loaded;
}

FolksIndividual *
games_individual_manager_lookup_member (GamesIndividualManager *self,
    const gchar *id)
{
  GamesIndividualManagerPriv *priv = GAMES_INDIVIDUAL_MANAGER_GET_PRIV (self);

  g_return_val_if_fail (GAMES_IS_INDIVIDUAL_MANAGER (self), NULL);

  return g_hash_table_lookup (priv->individuals, id);
}


