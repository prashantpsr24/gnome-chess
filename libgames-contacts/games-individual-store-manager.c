/* games-individual-store-manager.c: Is a GamesIndividualStore which connects to
 *                                   GamesIndividualManager signals to update
 *                                   itself and manages initial loading of
 *                                   contacts.
 *
 * Copyright (C) 2012 Chandni Verma
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

/*
 * This code has been influenced by folks usage in Empathy,
 * a chat application based on Telepathy (https://live.gnome.org/Empathy)
 * Copyright (C) 2005-2007 Imendio ABdd
 * Copyright (C) 2007-2011 Collabora Ltd.
 */

#include "config.h"

#include <string.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <folks/folks.h>
#include <folks/folks-telepathy.h>
#include <telepathy-glib/util.h>

/*#include <libgames-support/games-utils.h>*/
#include "games-enum-types.h"
#include "games-individual-manager.h"

#include "games-individual-store-manager.h"

#include "games-ui-utils.h"
#include "games-enum-types.h"

struct _GamesIndividualStoreManagerPriv
{
  GamesIndividualManager *manager;
  gboolean setup_idle_id;
};

enum
{
  PROP_0,
  PROP_INDIVIDUAL_MANAGER,
};


G_DEFINE_TYPE (GamesIndividualStoreManager, games_individual_store_manager,
    GAMES_TYPE_INDIVIDUAL_STORE);

static void
individual_store_manager_members_changed_cb (GamesIndividualManager *manager,
    const gchar *message,
    GList *added,
    GList *removed,
    guint reason,
    GamesIndividualStoreManager *self)
{
  GList *l;
  GamesIndividualStore *store = GAMES_INDIVIDUAL_STORE (self);

  for (l = removed; l; l = l->next)
    {
      g_debug ("Individual %s (%s) %s",
          folks_individual_get_id (l->data),
          folks_alias_details_get_alias (FOLKS_ALIAS_DETAILS (l->data)),
          "removed");

      individual_store_remove_individual_and_disconnect (store, l->data);
    }

  for (l = added; l; l = l->next)
    {
      g_debug ("Individual %s (%s) %s", folks_individual_get_id (l->data),
          folks_alias_details_get_alias (FOLKS_ALIAS_DETAILS (l->data)),
          "added");

      individual_store_add_individual_and_connect (store, l->data);
    }
}

static void
individual_store_manager_groups_changed_cb (GamesIndividualManager *manager,
    FolksIndividual *individual,
    gchar *group,
    gboolean is_member,
    GamesIndividualStoreManager *self)
{
  GamesIndividualStore *store = GAMES_INDIVIDUAL_STORE (self);

  g_debug ("Updating groups for individual %s (%s)",
      folks_individual_get_id (individual),
      folks_alias_details_get_alias (FOLKS_ALIAS_DETAILS (individual)));

  /* We do this to make sure the groups are correct, if not, we
   * would have to check the groups already set up for each
   * contact and then see what has been updated.
   */
  games_individual_store_refresh_individual (store, individual);
}

static void
individual_store_manager_add_existing_individuals (
    GamesIndividualStoreManager *self,
    const gchar *message)
{
  GList *individuals;

  individuals = games_individual_manager_get_members (self->priv->manager);
  if (individuals != NULL)
    {
      individual_store_manager_members_changed_cb (self->priv->manager,
          message, individuals, NULL, 0, self);
      g_list_free (individuals);
    }
}

static gboolean
individual_store_manager_manager_setup (gpointer user_data)
{
  GamesIndividualStoreManager *self = user_data;

  /* Signal connection. */

  g_debug ("handling individual renames unimplemented");

  g_signal_connect (self->priv->manager,
      "members-changed",
      G_CALLBACK (individual_store_manager_members_changed_cb), self);

  g_signal_connect (self->priv->manager,
      "groups-changed",
      G_CALLBACK (individual_store_manager_groups_changed_cb), self);

  /* Add contacts already created. */
  individual_store_manager_add_existing_individuals (self, "initial add");

  self->priv->setup_idle_id = 0;
  return FALSE;
}

static void
individual_store_manager_set_individual_manager (
    GamesIndividualStoreManager *self,
    GamesIndividualManager *manager)
{
  g_assert (self->priv->manager == NULL); /* construct only */
  self->priv->manager = g_object_ref (manager);

  /* Let a chance to have all properties set before populating. They are all
   * set after constructed () gets called and before this object is returned
   * to the code creating it.
   *
   * Loading @manager is added to an idle source here to avoid having every
   * code creating a GamesIndividualManagerStore object do it (since loading
   * can take some time) after obtaining this object. */
  self->priv->setup_idle_id = g_idle_add (
      individual_store_manager_manager_setup, self);
}

static void
individual_store_manager_dispose (GObject *object)
{
  GamesIndividualStoreManager *self = GAMES_INDIVIDUAL_STORE_MANAGER (
      object);
  GamesIndividualStore *store = GAMES_INDIVIDUAL_STORE (object);
  GList *individuals, *l;

  individuals = games_individual_manager_get_members (self->priv->manager);
  for (l = individuals; l; l = l->next)
    {
      games_individual_store_disconnect_individual (store,
          FOLKS_INDIVIDUAL (l->data));
    }
  tp_clear_pointer (&individuals, g_list_free);

  if (self->priv->manager != NULL)
    {
      g_signal_handlers_disconnect_by_func (self->priv->manager,
          G_CALLBACK (individual_store_manager_members_changed_cb), object);
      g_signal_handlers_disconnect_by_func (self->priv->manager,
          G_CALLBACK (individual_store_manager_groups_changed_cb), object);
      g_clear_object (&self->priv->manager);
    }

  if (self->priv->setup_idle_id != 0)
    {
      g_source_remove (self->priv->setup_idle_id);
      self->priv->setup_idle_id = 0;
    }

  G_OBJECT_CLASS (games_individual_store_manager_parent_class)->dispose (
      object);
}

static void
individual_store_manager_get_property (GObject *object,
    guint param_id,
    GValue *value,
    GParamSpec *pspec)
{
  GamesIndividualStoreManager *self = GAMES_INDIVIDUAL_STORE_MANAGER (
      object);

  switch (param_id)
    {
    case PROP_INDIVIDUAL_MANAGER:
      g_value_set_object (value, self->priv->manager);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    };
}

static void
individual_store_manager_set_property (GObject *object,
    guint param_id,
    const GValue *value,
    GParamSpec *pspec)
{
  switch (param_id)
    {
    case PROP_INDIVIDUAL_MANAGER:
      individual_store_manager_set_individual_manager (
          GAMES_INDIVIDUAL_STORE_MANAGER (object),
          g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    };
}

static void
individual_store_manager_reload_individuals (GamesIndividualStore *store)
{
  GamesIndividualStoreManager *self = GAMES_INDIVIDUAL_STORE_MANAGER (
      store);

  individual_store_manager_add_existing_individuals (self,
      "re-adding members: toggled group visibility");
}

static gboolean
individual_store_manager_initial_loading (GamesIndividualStore *store)
{
  GamesIndividualStoreManager *self = GAMES_INDIVIDUAL_STORE_MANAGER (
      store);

  return self->priv->setup_idle_id != 0;
}

static void
games_individual_store_manager_class_init (
    GamesIndividualStoreManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GamesIndividualStoreClass *store_class = GAMES_INDIVIDUAL_STORE_CLASS (
      klass);

  object_class->dispose = individual_store_manager_dispose;
  object_class->get_property = individual_store_manager_get_property;
  object_class->set_property = individual_store_manager_set_property;

  store_class->reload_individuals = individual_store_manager_reload_individuals;
  store_class->initial_loading = individual_store_manager_initial_loading;

  g_object_class_install_property (object_class,
      PROP_INDIVIDUAL_MANAGER,
      g_param_spec_object ("individual-manager",
          "Individual manager",
          "Individual manager",
          GAMES_TYPE_INDIVIDUAL_MANAGER,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

  g_type_class_add_private (object_class,
      sizeof (GamesIndividualStoreManagerPriv));
}

static void
games_individual_store_manager_init (GamesIndividualStoreManager *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      GAMES_TYPE_INDIVIDUAL_STORE_MANAGER, GamesIndividualStoreManagerPriv);
}

GamesIndividualStoreManager *
games_individual_store_manager_new (GamesIndividualManager *manager)
{
  g_return_val_if_fail (GAMES_IS_INDIVIDUAL_MANAGER (manager), NULL);

  return g_object_new (GAMES_TYPE_INDIVIDUAL_STORE_MANAGER,
      "individual-manager", manager, NULL);
}

GamesIndividualManager *
games_individual_store_manager_get_manager (
    GamesIndividualStoreManager *self)
{
  g_return_val_if_fail (GAMES_IS_INDIVIDUAL_STORE_MANAGER (self), FALSE);

  return self->priv->manager;
}
