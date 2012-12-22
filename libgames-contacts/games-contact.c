/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
/*
 * games-contact.c: A wrapper for TpContact to keep a track of contacts'
 * gaming capabilities since folks doesn't directly provide them for TpfPersona
 * (bgo#626179). As TpContact, GamesContact is one for each persona for an
 * individual, logically.
 *
 * Copyright (C) 2012 Chandni Verma
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#include <string.h>

#include <glib/gi18n-lib.h>

#include <telepathy-glib/account-manager.h>
#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/util.h>

#include <folks/folks.h>
#include <folks/folks-telepathy.h>

#include "games-contact.h"
#include "games-individual-manager.h"
#include "games-enum-types.h"

/* Much of code here is a simplified adaptation of the way contacts are
 * wrapped in Empathy, a chat application using Folks.
 * (https://live.gnome.org/Empathy)
 * Copyright (C) 2007-2009 Collabora Ltd.
 */

typedef struct {
  TpContact *tp_contact;
  TpAccount *account;
  FolksPersona *persona;
  gchar *id;
  gchar *alias;
  GamesAvatar *avatar;
  TpConnectionPresenceType presence;
  guint handle;
  GamesCapabilities capabilities;
  gboolean is_user;
} GamesContactPriv;

static void contact_finalize (GObject *object);
static void contact_get_property (GObject *object, guint param_id,
    GValue *value, GParamSpec *pspec);
static void contact_set_property (GObject *object, guint param_id,
    const GValue *value, GParamSpec *pspec);

static void set_capabilities_from_tp_caps (GamesContact *self,
    TpCapabilities *caps);

static void contact_set_avatar (GamesContact *contact,
    GamesAvatar *avatar);
static void contact_set_avatar_from_tp_contact (GamesContact *contact);

G_DEFINE_TYPE (GamesContact, games_contact, G_TYPE_OBJECT);

enum
{
  PROP_0,
  PROP_TP_CONTACT,
  PROP_ACCOUNT,
  PROP_PERSONA,
  PROP_ID,
  PROP_ALIAS,
  PROP_LOGGED_ALIAS,
  PROP_AVATAR,
  PROP_PRESENCE,
  PROP_PRESENCE_MESSAGE,
  PROP_HANDLE,
  PROP_CAPABILITIES,
  PROP_IS_USER,
};

enum {
  PRESENCE_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

/* TpContact* -> GamesContact*, both borrowed ref */
static GHashTable *contacts_table = NULL;

static void
tp_contact_notify_cb (TpContact *tp_contact,
                      GParamSpec *param,
                      GObject *contact)
{
  GamesContactPriv *priv = (GAMES_CONTACT (contact))->priv;

  /* Forward property notifications */
  if (!tp_strdiff (param->name, "alias"))
    g_object_notify (contact, "alias");
  else if (!tp_strdiff (param->name, "presence-type")) {
    TpConnectionPresenceType presence;

    presence = games_contact_get_presence (GAMES_CONTACT (contact));
    g_signal_emit (contact, signals[PRESENCE_CHANGED], 0, presence,
      priv->presence);
    priv->presence = presence;
    g_object_notify (contact, "presence");
  }
  else if (!tp_strdiff (param->name, "identifier"))
    g_object_notify (contact, "id");
  else if (!tp_strdiff (param->name, "handle"))
    g_object_notify (contact, "handle");
  else if (!tp_strdiff (param->name, "capabilities"))
    {
      set_capabilities_from_tp_caps (GAMES_CONTACT (contact),
          tp_contact_get_capabilities (tp_contact));
    }
  else if (!tp_strdiff (param->name, "avatar-file"))
    {
      contact_set_avatar_from_tp_contact (GAMES_CONTACT (contact));
    }
}

static void
folks_persona_notify_cb (FolksPersona *folks_persona,
                         GParamSpec *param,
                         GObject *contact)
{
  if (!tp_strdiff (param->name, "presence-message"))
    g_object_notify (contact, "presence-message");
}

static void
contact_dispose (GObject *object)
{
  GamesContactPriv *priv = (GAMES_CONTACT (object))->priv;

  if (priv->tp_contact != NULL)
    {
      g_signal_handlers_disconnect_by_func (priv->tp_contact,
          tp_contact_notify_cb, object);
    }
  tp_clear_object (&priv->tp_contact);

  if (priv->account)
    g_object_unref (priv->account);
  priv->account = NULL;

  if (priv->persona)
    {
      g_signal_handlers_disconnect_by_func (priv->persona,
          folks_persona_notify_cb, object);
      g_object_unref (priv->persona);
    }
  priv->persona = NULL;

  if (priv->avatar != NULL)
    {
      games_avatar_unref (priv->avatar);
      priv->avatar = NULL;
    }

  G_OBJECT_CLASS (games_contact_parent_class)->dispose (object);
}

static void
contact_constructed (GObject *object)
{
  GamesContact *contact = (GamesContact *) object;
  GamesContactPriv *priv = contact->priv;
  TpContact *self_contact;

  if (priv->tp_contact == NULL)
    return;

  priv->presence = games_contact_get_presence (contact);

  set_capabilities_from_tp_caps (contact,
      tp_contact_get_capabilities (priv->tp_contact));

  contact_set_avatar_from_tp_contact (contact);

  /* Set is-user property. We could use handles too */
  self_contact = tp_connection_get_self_contact (
      tp_contact_get_connection (priv->tp_contact));
  games_contact_set_is_user (contact, self_contact == priv->tp_contact);

  g_signal_connect (priv->tp_contact, "notify",
    G_CALLBACK (tp_contact_notify_cb), contact);
}

static void
games_contact_class_init (GamesContactClass *class)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (class);

  object_class->finalize = contact_finalize;
  object_class->dispose = contact_dispose;
  object_class->get_property = contact_get_property;
  object_class->set_property = contact_set_property;
  object_class->constructed = contact_constructed;

  g_object_class_install_property (object_class,
      PROP_TP_CONTACT,
      g_param_spec_object ("tp-contact",
        "TpContact",
        "The TpContact associated with the contact",
        TP_TYPE_CONTACT,
        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
      PROP_ACCOUNT,
      g_param_spec_object ("account",
        "The account",
        "The account associated with the contact",
        TP_TYPE_ACCOUNT,
        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
      PROP_PERSONA,
      g_param_spec_object ("persona",
        "Persona",
        "The FolksPersona associated with the contact",
        FOLKS_TYPE_PERSONA,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
      PROP_ID,
      g_param_spec_string ("id",
        "Contact id",
        "String identifying contact",
        NULL,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
      PROP_ALIAS,
      g_param_spec_string ("alias",
        "Contact alias",
        "An alias for the contact",
        NULL,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
      PROP_AVATAR,
      g_param_spec_boxed ("avatar",
        "Avatar image",
        "The avatar image",
        GAMES_TYPE_AVATAR,
        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
      PROP_PRESENCE,
      g_param_spec_uint ("presence",
        "Contact presence",
        "Presence of contact",
        TP_CONNECTION_PRESENCE_TYPE_UNSET,
        NUM_TP_CONNECTION_PRESENCE_TYPES,
        TP_CONNECTION_PRESENCE_TYPE_UNSET,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
      PROP_PRESENCE_MESSAGE,
      g_param_spec_string ("presence-message",
        "Contact presence message",
        "Presence message of contact",
        NULL,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
      PROP_HANDLE,
      g_param_spec_uint ("handle",
        "Contact Handle",
        "The handle of the contact",
        0,
        G_MAXUINT,
        0,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
      PROP_CAPABILITIES,
      g_param_spec_flags ("capabilities",
        "Contact Capabilities",
        "Capabilities of the contact",
        GAMES_TYPE_CAPABILITIES,
        GAMES_CAPABILITIES_UNKNOWN,
        G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
      PROP_IS_USER,
      g_param_spec_boolean ("is-user",
        "Contact is-user",
        "Is contact the user",
        FALSE,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  signals[PRESENCE_CHANGED] =
    g_signal_new ("presence-changed",
                  G_TYPE_FROM_CLASS (class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_generic,
                  G_TYPE_NONE,
                  2, G_TYPE_UINT,
                  G_TYPE_UINT);

  g_type_class_add_private (object_class, sizeof (GamesContactPriv));
}

static void
games_contact_init (GamesContact *contact)
{
  GamesContactPriv *priv = G_TYPE_INSTANCE_GET_PRIVATE (contact,
    GAMES_TYPE_CONTACT, GamesContactPriv);

  contact->priv = priv;
}

static void
contact_finalize (GObject *object)
{
  GamesContactPriv *priv;

  priv = (GAMES_CONTACT (object))->priv;

  g_debug ("finalize: %p", object);

  g_free (priv->alias);
  g_free (priv->id);

  G_OBJECT_CLASS (games_contact_parent_class)->finalize (object);
}

static void
games_contact_set_capabilities (GamesContact *contact,
                                  GamesCapabilities capabilities)
{
  GamesContactPriv *priv;

  g_return_if_fail (GAMES_IS_CONTACT (contact));

  priv = contact->priv;

  if (priv->capabilities == capabilities)
    return;

  priv->capabilities = capabilities;

  g_object_notify (G_OBJECT (contact), "capabilities");
}

static void
games_contact_set_id (GamesContact *contact,
                        const gchar *id)
{
  GamesContactPriv *priv;

  g_return_if_fail (GAMES_IS_CONTACT (contact));
  g_return_if_fail (id != NULL);

  priv = contact->priv;

  /* We temporally ref the contact because it could be destroyed
   * during the signal emition */
  g_object_ref (contact);
  if (tp_strdiff (id, priv->id))
    {
      g_free (priv->id);
      priv->id = g_strdup (id);

      g_object_notify (G_OBJECT (contact), "id");

      /* And since alias defaults to ID */
      if (priv->alias == NULL || g_strcmp0 (priv->alias, "") == 0)
          g_object_notify (G_OBJECT (contact), "alias");
    }

  g_object_unref (contact);
}

static void
games_contact_set_presence (GamesContact *contact,
                              TpConnectionPresenceType presence)
{
  GamesContactPriv *priv;
  TpConnectionPresenceType old_presence;

  g_return_if_fail (GAMES_IS_CONTACT (contact));

  priv = contact->priv;

  if (presence == priv->presence)
    return;

  old_presence = priv->presence;
  priv->presence = presence;

  g_signal_emit (contact, signals[PRESENCE_CHANGED], 0, presence, old_presence);

  g_object_notify (G_OBJECT (contact), "presence");
}

static void
games_contact_set_presence_message (GamesContact *contact,
                                      const gchar *message)
{
  GamesContactPriv *priv = contact->priv;

  g_return_if_fail (GAMES_IS_CONTACT (contact));

  if (priv->persona != NULL)
    {
      folks_presence_details_set_presence_message (
          FOLKS_PRESENCE_DETAILS (priv->persona), message);
    }
}

static void
games_contact_set_handle (GamesContact *contact,
                            guint handle)
{
  GamesContactPriv *priv;

  g_return_if_fail (GAMES_IS_CONTACT (contact));

  priv = contact->priv;

  g_object_ref (contact);
  if (handle != priv->handle)
    {
      priv->handle = handle;
      g_object_notify (G_OBJECT (contact), "handle");
    }
  g_object_unref (contact);
}

static void
contact_get_property (GObject *object,
                      guint param_id,
                      GValue *value,
                      GParamSpec *pspec)
{
  GamesContact *contact = GAMES_CONTACT (object);

  switch (param_id)
    {
      case PROP_TP_CONTACT:
        g_value_set_object (value, games_contact_get_tp_contact (contact));
        break;
      case PROP_ACCOUNT:
        g_value_set_object (value, games_contact_get_account (contact));
        break;
      case PROP_PERSONA:
        g_value_set_object (value, games_contact_get_persona (contact));
        break;
      case PROP_ID:
        g_value_set_string (value, games_contact_get_id (contact));
        break;
      case PROP_ALIAS:
        g_value_set_string (value, games_contact_get_alias (contact));
        break;
      case PROP_AVATAR:
        g_value_set_boxed (value, games_contact_get_avatar (contact));
        break;
      case PROP_PRESENCE:
        g_value_set_uint (value, games_contact_get_presence (contact));
        break;
      case PROP_PRESENCE_MESSAGE:
        g_value_set_string (value, games_contact_get_presence_message (contact));
        break;
      case PROP_HANDLE:
        g_value_set_uint (value, games_contact_get_handle (contact));
        break;
      case PROP_CAPABILITIES:
        g_value_set_flags (value, games_contact_get_capabilities (contact));
        break;
      case PROP_IS_USER:
        g_value_set_boolean (value, games_contact_is_user (contact));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    };
}

static void
contact_set_property (GObject *object,
                      guint param_id,
                      const GValue *value,
                      GParamSpec *pspec)
{
  GamesContact *contact = GAMES_CONTACT (object);
  GamesContactPriv *priv = contact->priv;

  switch (param_id)
    {
      case PROP_TP_CONTACT:
        priv->tp_contact = g_value_dup_object (value);
        break;
      case PROP_ACCOUNT:
        g_assert (priv->account == NULL);
        priv->account = g_value_dup_object (value);
        break;
      case PROP_PERSONA:
        games_contact_set_persona (contact, g_value_get_object (value));
        break;
      case PROP_ID:
        games_contact_set_id (contact, g_value_get_string (value));
        break;
      case PROP_ALIAS:
        games_contact_set_alias (contact, g_value_get_string (value));
        break;
      case PROP_PRESENCE:
        games_contact_set_presence (contact, g_value_get_uint (value));
        break;
      case PROP_PRESENCE_MESSAGE:
        games_contact_set_presence_message (contact, g_value_get_string (value));
        break;
      case PROP_HANDLE:
        games_contact_set_handle (contact, g_value_get_uint (value));
        break;
      case PROP_CAPABILITIES:
        games_contact_set_capabilities (contact, g_value_get_flags (value));
        break;
      case PROP_IS_USER:
        games_contact_set_is_user (contact, g_value_get_boolean (value));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    };
}

static void
remove_tp_contact (gpointer data,
    GObject *object)
{
  g_hash_table_remove (contacts_table, data);
}

static GamesContact *
games_contact_new (TpContact *tp_contact)
{
  GamesContact *retval;

  g_return_val_if_fail (TP_IS_CONTACT (tp_contact), NULL);

  retval = g_object_new (GAMES_TYPE_CONTACT,
      "tp-contact", tp_contact,
      NULL);

  g_object_weak_ref (G_OBJECT (retval), remove_tp_contact, tp_contact);

  return retval;
}

TpContact *
games_contact_get_tp_contact (GamesContact *contact)
{
  GamesContactPriv *priv;

  g_return_val_if_fail (GAMES_IS_CONTACT (contact), NULL);

  priv = contact->priv;

  return priv->tp_contact;
}

const gchar *
games_contact_get_id (GamesContact *contact)
{
  GamesContactPriv *priv;

  g_return_val_if_fail (GAMES_IS_CONTACT (contact), NULL);

  priv = contact->priv;

  if (priv->tp_contact != NULL)
    return tp_contact_get_identifier (priv->tp_contact);

  return priv->id;
}

const gchar *
games_contact_get_alias (GamesContact *contact)
{
  GamesContactPriv *priv;
  const gchar        *alias = NULL;

  g_return_val_if_fail (GAMES_IS_CONTACT (contact), NULL);

  priv = contact->priv;

  if (priv->alias != NULL && g_strcmp0 (priv->alias, "") != 0)
    alias = priv->alias;
  else if (priv->tp_contact != NULL)
    alias = tp_contact_get_alias (priv->tp_contact);

  if (alias != NULL && g_strcmp0 (alias, "") != 0)
    return alias;
  else
    return games_contact_get_id (contact);
}

void
games_contact_set_alias (GamesContact *contact,
                          const gchar *alias)
{
  GamesContactPriv *priv;
  FolksPersona *persona;

  g_return_if_fail (GAMES_IS_CONTACT (contact));

  priv = contact->priv;

  g_object_ref (contact);

  /* Set the alias on the persona */
  persona = games_contact_get_persona (contact);
  /* We are not creating contacts hence all our GamesContacts have to have
   * personas beforehand */
  g_assert (persona != NULL);

  if (FOLKS_IS_ALIAS_DETAILS (persona))
    {
      g_debug ("Setting alias for contact %s to %s",
          games_contact_get_id (contact), alias);

      folks_alias_details_set_alias (FOLKS_ALIAS_DETAILS (persona), alias);
    }

  if (tp_strdiff (alias, priv->alias))
    {
      g_free (priv->alias);
      priv->alias = g_strdup (alias);
      g_object_notify (G_OBJECT (contact), "alias");
    }

  g_object_unref (contact);
}

GamesAvatar *
games_contact_get_avatar (GamesContact *contact)
{
  GamesContactPriv *priv;

  g_return_val_if_fail (GAMES_IS_CONTACT (contact), NULL);

  priv = contact->priv;

  return priv->avatar;
}

static void
contact_set_avatar (GamesContact *contact,
                    GamesAvatar *avatar)
{
  GamesContactPriv *priv;

  g_return_if_fail (GAMES_IS_CONTACT (contact));

  priv = contact->priv;

  if (priv->avatar == avatar)
    return;

  if (priv->avatar)
    {
      games_avatar_unref (priv->avatar);
      priv->avatar = NULL;
    }

  if (avatar)
      priv->avatar = games_avatar_ref (avatar);

  g_object_notify (G_OBJECT (contact), "avatar");
}

TpAccount *
games_contact_get_account (GamesContact *contact)
{
  GamesContactPriv *priv;

  g_return_val_if_fail (GAMES_IS_CONTACT (contact), NULL);

  priv = contact->priv;

  if (priv->account == NULL && priv->tp_contact != NULL)
    {
      TpConnection *connection;

      /* FIXME: This assume the account manager already exists */
      connection = tp_contact_get_connection (priv->tp_contact);
      priv->account =
        g_object_ref (tp_connection_get_account (connection));
    }

  return priv->account;
}

FolksPersona *
games_contact_get_persona (GamesContact *contact)
{
  GamesContactPriv *priv;

  g_return_val_if_fail (GAMES_IS_CONTACT (contact), NULL);

  priv = contact->priv;

  if (priv->persona == NULL && priv->tp_contact != NULL)
    {
      TpfPersona *persona;

      persona = tpf_persona_dup_for_contact (priv->tp_contact);
      if (persona != NULL)
        {
          games_contact_set_persona (contact, (FolksPersona *) persona);
          g_object_unref (persona);
        }
    }

  return priv->persona;
}

void
games_contact_set_persona (GamesContact *contact,
    FolksPersona *persona)
{
  GamesContactPriv *priv;

  g_return_if_fail (GAMES_IS_CONTACT (contact));
  g_return_if_fail (TPF_IS_PERSONA (persona));

  priv = contact->priv;

  if (persona == priv->persona)
    return;

  if (priv->persona != NULL)
    {
      g_signal_handlers_disconnect_by_func (priv->persona,
          folks_persona_notify_cb, contact);
      g_object_unref (priv->persona);
    }
  priv->persona = g_object_ref (persona);

  g_signal_connect (priv->persona, "notify",
    G_CALLBACK (folks_persona_notify_cb), contact);

  g_object_notify (G_OBJECT (contact), "persona");

  /* Set the persona's alias, since ours could've been set using
   * games_contact_set_alias() before we had a persona; this happens when
   * adding a contact. */
  if (priv->alias != NULL)
    games_contact_set_alias (contact, priv->alias);
}

TpConnection *
games_contact_get_connection (GamesContact *contact)
{
  GamesContactPriv *priv;

  g_return_val_if_fail (GAMES_IS_CONTACT (contact), NULL);

  priv = contact->priv;

  if (priv->tp_contact != NULL)
    return tp_contact_get_connection (priv->tp_contact);

  return NULL;
}

TpConnectionPresenceType
games_contact_get_presence (GamesContact *contact)
{
  GamesContactPriv *priv;

  g_return_val_if_fail (GAMES_IS_CONTACT (contact),
    TP_CONNECTION_PRESENCE_TYPE_UNSET);

  priv = contact->priv;

  if (priv->tp_contact != NULL)
    return tp_contact_get_presence_type (priv->tp_contact);

  return priv->presence;
}

const gchar *
games_contact_get_presence_message (GamesContact *contact)
{
  GamesContactPriv *priv;

  g_return_val_if_fail (GAMES_IS_CONTACT (contact), NULL);

  priv = contact->priv;

  if (priv->persona != NULL)
    return folks_presence_details_get_presence_message (
        FOLKS_PRESENCE_DETAILS (priv->persona));

  if (priv->tp_contact != NULL)
    return tp_contact_get_presence_message (priv->tp_contact);

  return NULL;
}

guint
games_contact_get_handle (GamesContact *contact)
{
  GamesContactPriv *priv;

  g_return_val_if_fail (GAMES_IS_CONTACT (contact), 0);

  priv = contact->priv;

  if (priv->tp_contact != NULL)
    return tp_contact_get_handle (priv->tp_contact);

  return priv->handle;
}

GamesCapabilities
games_contact_get_capabilities (GamesContact *contact)
{
  GamesContactPriv *priv;

  g_return_val_if_fail (GAMES_IS_CONTACT (contact), 0);

  priv = contact->priv;

  return priv->capabilities;
}

gboolean
games_contact_is_user (GamesContact *contact)
{
  GamesContactPriv *priv;

  g_return_val_if_fail (GAMES_IS_CONTACT (contact), FALSE);

  priv = contact->priv;

  return priv->is_user;
}

void
games_contact_set_is_user (GamesContact *contact,
                             gboolean is_user)
{
  GamesContactPriv *priv;

  g_return_if_fail (GAMES_IS_CONTACT (contact));

  priv = contact->priv;

  if (priv->is_user == is_user)
    return;

  priv->is_user = is_user;

  g_object_notify (G_OBJECT (contact), "is-user");
}

gboolean
games_contact_is_online (GamesContact *contact)
{
  g_return_val_if_fail (GAMES_IS_CONTACT (contact), FALSE);

  switch (games_contact_get_presence (contact))
    {
      case TP_CONNECTION_PRESENCE_TYPE_OFFLINE:
      case TP_CONNECTION_PRESENCE_TYPE_UNKNOWN:
      case TP_CONNECTION_PRESENCE_TYPE_ERROR:
        return FALSE;
      /* Contacts without presence are considered online so we can display IRC
       * contacts in rooms. */
      case TP_CONNECTION_PRESENCE_TYPE_UNSET:
      case TP_CONNECTION_PRESENCE_TYPE_AVAILABLE:
      case TP_CONNECTION_PRESENCE_TYPE_AWAY:
      case TP_CONNECTION_PRESENCE_TYPE_EXTENDED_AWAY:
      case TP_CONNECTION_PRESENCE_TYPE_HIDDEN:
      case TP_CONNECTION_PRESENCE_TYPE_BUSY:
      default:
        return TRUE;
    }
}

const gchar *
games_presence_get_default_message (TpConnectionPresenceType presence)
{
  switch (presence)
    {
      case TP_CONNECTION_PRESENCE_TYPE_AVAILABLE:
        return _("Available");
      case TP_CONNECTION_PRESENCE_TYPE_BUSY:
        return _("Busy");
      case TP_CONNECTION_PRESENCE_TYPE_AWAY:
      case TP_CONNECTION_PRESENCE_TYPE_EXTENDED_AWAY:
        return _("Away");
      case TP_CONNECTION_PRESENCE_TYPE_HIDDEN:
        return _("Invisible");
      case TP_CONNECTION_PRESENCE_TYPE_OFFLINE:
        return _("Offline");
      case TP_CONNECTION_PRESENCE_TYPE_UNKNOWN:
        /* translators: presence type is unknown */
        return C_("presence", "Unknown");
      case TP_CONNECTION_PRESENCE_TYPE_UNSET:
      case TP_CONNECTION_PRESENCE_TYPE_ERROR:
      default:
        return NULL;
    }

  return NULL;
}

const gchar *
games_contact_get_status (GamesContact *contact)
{
  const gchar *message;

  g_return_val_if_fail (GAMES_IS_CONTACT (contact), "");

  message = games_contact_get_presence_message (contact);
  if (message != NULL && g_strcmp0 (message, "") != 0)
    return message;

  return games_presence_get_default_message (
      games_contact_get_presence (contact));
}

gboolean
games_contact_can_play_glchess (GamesContact *contact)
{
  GamesContactPriv *priv;

  g_return_val_if_fail (GAMES_IS_CONTACT (contact), FALSE);

  priv = contact->priv;

  return priv->capabilities & GAMES_CAPABILITIES_TUBE_GLCHESS;
}

gboolean
games_contact_can_do_action (GamesContact *self,
    GamesActionType action_type)
{
  gboolean sensitivity = FALSE;

  switch (action_type)
    {
      case GAMES_ACTION_CHAT:
        sensitivity = TRUE;
        break;
      case GAMES_ACTION_PLAY_GLCHESS:
        sensitivity = games_contact_can_play_glchess (self);
        break;
      default:
        g_assert_not_reached ();
    }

  return (sensitivity ? TRUE : FALSE);
}

GType
games_avatar_get_type (void)
{
  static GType type_id = 0;

  if (!type_id)
    {
      type_id = g_boxed_type_register_static ("GamesAvatar",
          (GBoxedCopyFunc) games_avatar_ref,
          (GBoxedFreeFunc) games_avatar_unref);
    }

  return type_id;
}

/**
 * games_avatar_new:
 * @data: the avatar data
 * @len: the size of avatar data
 * @format: the mime type of the avatar image
 * @filename: the filename where the avatar is stored in cache
 *
 * Create a #GamesAvatar from the provided data.
 *
 * Returns: a new #GamesAvatar
 */
GamesAvatar *
games_avatar_new (const guchar *data,
                    gsize len,
                    const gchar *format,
                    const gchar *filename)
{
  GamesAvatar *avatar;

  avatar = g_slice_new0 (GamesAvatar);
  avatar->data = g_memdup (data, len);
  avatar->len = len;
  avatar->format = g_strdup (format);
  avatar->filename = g_strdup (filename);
  avatar->refcount = 1;

  return avatar;
}

void
games_avatar_unref (GamesAvatar *avatar)
{
  g_return_if_fail (avatar != NULL);

  avatar->refcount--;
  if (avatar->refcount == 0)
    {
      g_free (avatar->data);
      g_free (avatar->format);
      g_free (avatar->filename);
      g_slice_free (GamesAvatar, avatar);
    }
}

GamesAvatar *
games_avatar_ref (GamesAvatar *avatar)
{
  g_return_val_if_fail (avatar != NULL, NULL);

  avatar->refcount++;

  return avatar;
}

/**
 * games_avatar_save_to_file:
 * @avatar: the avatar
 * @filename: name of a file to write avatar to
 * @error: return location for a GError, or NULL
 *
 * Save the avatar to a file named filename
 *
 * Returns: %TRUE on success, %FALSE if an error occurred
 */
gboolean
games_avatar_save_to_file (GamesAvatar *self,
                             const gchar *filename,
                             GError **error)
{
  return g_file_set_contents (filename, (const gchar *) self->data, self->len,
      error);
}

/**
 * games_contact_equal:
 * @contact1: an #GamesContact
 * @contact2: an #GamesContact
 *
 * Returns FALSE if one of the contacts is NULL but the other is not.
 * Otherwise returns TRUE if both pointer are equal or if they bith
 * refer to the same id.
 * It's only necessary to call this function if your contact objects
 * come from logs where contacts are created dynamically and comparing
 * pointers is not enough.
 */
gboolean
games_contact_equal (gconstpointer contact1,
                       gconstpointer contact2)
{
  GamesContact *c1;
  GamesContact *c2;
  const gchar *id1;
  const gchar *id2;

  if ((contact1 == NULL) != (contact2 == NULL)) {
    return FALSE;
  }
  if (contact1 == contact2) {
    return TRUE;
  }
  c1 = GAMES_CONTACT (contact1);
  c2 = GAMES_CONTACT (contact2);
  id1 = games_contact_get_id (c1);
  id2 = games_contact_get_id (c2);
  if (!tp_strdiff (id1, id2)) {
    return TRUE;
  }
  return FALSE;
}

static GamesCapabilities
tp_caps_to_capabilities (GamesContact *self,
    TpCapabilities *caps)
{
  GamesCapabilities capabilities = 0;

  if (tp_capabilities_supports_dbus_tubes (caps, TP_HANDLE_TYPE_CONTACT,
        TP_CLIENT_BUS_NAME_BASE "Gnome.Chess"))
  {
    g_debug ("Contact %s(%s) has glchess playing capabilities",
        games_contact_get_alias (self), games_contact_get_id (self));
    capabilities |= GAMES_CAPABILITIES_TUBE_GLCHESS;
  }

  return capabilities;
}

static void
set_capabilities_from_tp_caps (GamesContact *self,
    TpCapabilities *caps)
{
  GamesCapabilities capabilities;

  g_debug ("Got capabilities for contact %s(%s)",
      games_contact_get_alias (self), games_contact_get_id (self));

  if (caps == NULL)
    return;

  capabilities = tp_caps_to_capabilities (self, caps);
  games_contact_set_capabilities (self, capabilities);
}

static void
contact_set_avatar_from_tp_contact (GamesContact *contact)
{
  GamesContactPriv *priv = contact->priv;
  const gchar *mime;
  GFile *file;

  mime = tp_contact_get_avatar_mime_type (priv->tp_contact);
  file = tp_contact_get_avatar_file (priv->tp_contact);

  if (file != NULL)
    {
      GamesAvatar *avatar;
      gchar *data;
      gsize len;
      gchar *path;
      GError *error = NULL;

      if (!g_file_load_contents (file, NULL, &data, &len, NULL, &error))
        {
          g_debug ("Failed to load avatar: %s", error->message);

          g_error_free (error);
          contact_set_avatar (contact, NULL);
          return;
        }

      path = g_file_get_path (file);

      avatar = games_avatar_new ((guchar *) data, len, mime, path);

      contact_set_avatar (contact, avatar);
      games_avatar_unref (avatar);
      g_free (path);
      g_free (data);
    }
  else
    {
      contact_set_avatar (contact, NULL);
    }
}

GamesContact *
games_contact_dup_from_tp_contact (TpContact *tp_contact)
{
  GamesContact *contact = NULL;

  g_return_val_if_fail (TP_IS_CONTACT (tp_contact), NULL);

  if (contacts_table == NULL)
    contacts_table = g_hash_table_new (g_direct_hash, g_direct_equal);
  else
    contact = g_hash_table_lookup (contacts_table, tp_contact);

  if (contact == NULL)
    {
      contact = games_contact_new (tp_contact);

      /* The hash table does not keep any ref.
       * contact keeps a ref to tp_contact, and is removed from the table in
       * contact_dispose() */
      g_hash_table_insert (contacts_table, tp_contact, contact);
    }
  else
    {
      g_object_ref (contact);
    }

  return contact;
}

static int
presence_cmp_func (GamesContact *a,
    GamesContact *b)
{
  FolksPresenceDetails *presence_a, *presence_b;

  presence_a = FOLKS_PRESENCE_DETAILS (games_contact_get_persona (a));
  presence_b = FOLKS_PRESENCE_DETAILS (games_contact_get_persona (b));

  /* We negate the result because we're sorting in reverse order (i.e. such that
   * the Personas with the highest presence are at the beginning of the list. */
  return -folks_presence_details_typecmp (
      folks_presence_details_get_presence_type (presence_a),
      folks_presence_details_get_presence_type (presence_b));
}

static gint
glchess_tube_cmp_func (GamesContact *a,
    GamesContact *b)
{
  gboolean can_play_glchess_a, can_play_glchess_b;

  can_play_glchess_a = games_contact_can_play_glchess (a);
  can_play_glchess_b = games_contact_can_play_glchess (b);

  if (can_play_glchess_a == can_play_glchess_b)
    /* Fallback to sort by presence to break ties */
    return presence_cmp_func (a, b);
  else if (can_play_glchess_a)
    return -1;
  else
    return 1;
}

static GCompareFunc
get_sort_func_for_action (GamesActionType action_type)
{
  switch (action_type)
    {
      case GAMES_ACTION_CHAT:
        return (GCompareFunc) presence_cmp_func;
      case GAMES_ACTION_PLAY_GLCHESS:
        return (GCompareFunc) glchess_tube_cmp_func;
      default:
        g_assert_not_reached ();
    }
}

/**
 * games_contact_dup_best_for_action:
 * @individual: a #FolksIndividual
 * @action_type: the type of action to be performed on the contact
 *
 * Chooses a #FolksPersona from the given @individual which is best-suited for
 * the given @action_type. "Best-suited" is determined by choosing the persona
 * with the highest presence out of all the personas which can perform the
 * given @action_type (e.g. are capable of playing glchess remotely).
 *
 * Return value: an #GamesContact for the best persona, or %NULL;
 * unref with g_object_unref()
 */
GamesContact *
games_contact_dup_best_for_action (FolksIndividual *individual,
    GamesActionType action_type)
{
  GeeSet *personas;
  GeeIterator *iter;
  GList *contacts;
  GamesContact *best_contact = NULL;

  /* Build a list of GamesContacts that we can sort */
  personas = folks_individual_get_personas (individual);
  contacts = NULL;

  iter = gee_iterable_iterator (GEE_ITERABLE (personas));
  while (gee_iterator_next (iter))
    {
      FolksPersona *persona = gee_iterator_get (iter);
      TpContact *tp_contact;
      GamesContact *contact = NULL;

      if (!games_folks_persona_is_interesting (persona))
        goto while_finish;

      tp_contact = tpf_persona_get_contact (TPF_PERSONA (persona));
      if (tp_contact == NULL)
        goto while_finish;

      contact = games_contact_dup_from_tp_contact (tp_contact);
      games_contact_set_persona (contact, FOLKS_PERSONA (persona));

      /* Only choose the contact if they're actually capable of the specified
       * action. */
      if (games_contact_can_do_action (contact, action_type))
        contacts = g_list_prepend (contacts, g_object_ref (contact));

while_finish:
      g_clear_object (&contact);
      g_clear_object (&persona);
    }
  g_clear_object (&iter);

  /* Sort the contacts by some heuristic based on the action type, then take
   * the top contact. */
  if (contacts != NULL)
    {
      contacts = g_list_sort (contacts, get_sort_func_for_action (action_type));
      best_contact = g_object_ref (contacts->data);
    }

  g_list_foreach (contacts, (GFunc) g_object_unref, NULL);
  g_list_free (contacts);

  return best_contact;
}

/*
 * Returns whether a given persona wraps a usable TpContact
 */
gboolean
games_folks_persona_is_interesting (FolksPersona *persona)
{
  /* We're not interested in non-Telepathy personas */
  if (!TPF_IS_PERSONA (persona))
    return FALSE;

  /* We're not interested in user personas which haven't been added to the
   * contact list (see bgo#637151). */
  if (folks_persona_get_is_user (persona) &&
      !tpf_persona_get_is_in_contact_list (TPF_PERSONA (persona)))
    {
      return FALSE;
    }

  return TRUE;
}

/* Returns the GamesContact corresponding to the first TpContact contained
 * within the given Individual.
 * Note that this is a temporary convenience since actually,
 * FolksIndividuals are not 1:1 to TpContacts.
 *
 * The returned GamesContact object must be unreffed when done with it.
 */
GamesContact *
games_contact_dup_from_folks_individual (FolksIndividual *individual)
{
  GeeSet *personas;
  GeeIterator *iter;
  GamesContact *contact = NULL;

  g_return_val_if_fail (FOLKS_IS_INDIVIDUAL (individual), NULL);

  personas = folks_individual_get_personas (individual);
  iter = gee_iterable_iterator (GEE_ITERABLE (personas));
  while (gee_iterator_next (iter) && (contact == NULL))
    {
      TpfPersona *persona = gee_iterator_get (iter);

      if (games_folks_persona_is_interesting (FOLKS_PERSONA (persona)))
        {
          TpContact *tp_contact;

          tp_contact = tpf_persona_get_contact (persona);
          if (tp_contact != NULL)
            {
              contact = games_contact_dup_from_tp_contact (tp_contact);
              games_contact_set_persona (contact, FOLKS_PERSONA (persona));
            }
        }
      g_clear_object (&persona);
    }
  g_clear_object (&iter);

  if (contact == NULL)
    {
      g_debug ("Can't create an GamesContact for Individual %s",
          folks_individual_get_id (individual));
    }

  return contact;
}
