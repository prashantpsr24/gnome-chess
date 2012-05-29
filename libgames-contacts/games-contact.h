/*
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
 *
 */

#ifndef __GAMES_CONTACT_H__
#define __GAMES_CONTACT_H__

#include <glib-object.h>

#include <telepathy-glib/contact.h>
#include <telepathy-glib/account.h>
#include <folks/folks.h>

G_BEGIN_DECLS

#define GAMES_TYPE_CONTACT         (games_contact_get_type ())
#define GAMES_CONTACT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GAMES_TYPE_CONTACT, GamesContact))
#define GAMES_CONTACT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GAMES_TYPE_CONTACT, GamesContactClass))
#define GAMES_IS_CONTACT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GAMES_TYPE_CONTACT))
#define GAMES_IS_CONTACT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GAMES_TYPE_CONTACT))
#define GAMES_CONTACT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GAMES_TYPE_CONTACT, GamesContactClass))

typedef struct _GamesContact      GamesContact;
typedef struct _GamesContactClass GamesContactClass;

struct _GamesContact
{
  GObject parent;
  gpointer priv;
};

struct _GamesContactClass
{
  GObjectClass parent_class;
};

typedef struct {
  guchar *data;
  gsize len;
  gchar *format;
  gchar *token;
  gchar *filename;
  guint refcount;
} GamesAvatar;

typedef enum {
  GAMES_CAPABILITIES_NONE = 0,
  GAMES_CAPABILITIES_TUBE_GLCHESS = 1 << 0,
  GAMES_CAPABILITIES_UNKNOWN = 1 << 1
} GamesCapabilities;

GType                     games_contact_get_type             (void) G_GNUC_CONST;
TpContact                *games_contact_get_tp_contact       (GamesContact *contact);
const gchar              *games_contact_get_id               (GamesContact *contact);
const gchar              *games_contact_get_alias            (GamesContact *contact);
void                      games_contact_set_alias            (GamesContact *contact,
                                                              const gchar *alias);
GamesAvatar              *games_contact_get_avatar           (GamesContact *contact);
TpAccount                *games_contact_get_account          (GamesContact *contact);
FolksPersona             *games_contact_get_persona          (GamesContact *contact);
void                      games_contact_set_persona          (GamesContact *contact,
                                                              FolksPersona *persona);
TpConnection             *games_contact_get_connection       (GamesContact *contact);
TpConnectionPresenceType  games_contact_get_presence         (GamesContact *contact);
const gchar              *games_contact_get_presence_message (GamesContact *contact);
guint                     games_contact_get_handle           (GamesContact *contact);
GamesCapabilities         games_contact_get_capabilities     (GamesContact *contact);
gboolean                  games_contact_is_user              (GamesContact *contact);
void                      games_contact_set_is_user          (GamesContact *contact,
                                                              gboolean is_user);
gboolean                  games_contact_is_online            (GamesContact *contact);
const gchar              *games_presence_get_default_message (TpConnectionPresenceType presence);
const gchar              *games_contact_get_status           (GamesContact *contact);
gboolean                  games_contact_can_play_glchess     (GamesContact *contact);
gboolean                  games_folks_persona_is_interesting (FolksPersona *persona);
GamesContact             *games_contact_dup_from_folks_individual (FolksIndividual *individual);

typedef enum {
  GAMES_ACTION_CHAT,
  GAMES_ACTION_PLAY_GLCHESS,
} GamesActionType;

gboolean                  games_contact_can_do_action        (GamesContact *self,
                                                              GamesActionType action_type);

#define GAMES_TYPE_AVATAR (games_avatar_get_type ())
GType                     games_avatar_get_type              (void) G_GNUC_CONST;
GamesAvatar              *games_avatar_new                   (const guchar *data,
                                                              gsize len,
                                                              const gchar *format,
                                                              const gchar *filename);
GamesAvatar              *games_avatar_ref                   (GamesAvatar *avatar);
void                      games_avatar_unref                 (GamesAvatar *avatar);

gboolean                  games_avatar_save_to_file          (GamesAvatar *avatar,
                                                              const gchar *filename, GError **error);

gboolean                  games_contact_equal                (gconstpointer contact1,
                                                              gconstpointer contact2);

GamesContact             *games_contact_dup_from_tp_contact  (TpContact *tp_contact);
GamesContact             *games_contact_dup_best_for_action  (FolksIndividual *individual,
                                                              GamesActionType action_type);

G_END_DECLS

#endif /* __GAMES_CONTACT_H__ */
