/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* games-ui-utils.h - This file contains generic UI specific code for use in
 *                    displaying games and contacts to players.
 *
 * Copyright (C) 2002-2007 Imendio AB
 * Copyright (C) 2007-2010 Collabora Ltd.
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
 *
 * This file has been prepared from utilities provided in
 * Empathy (https://live.gnome.org/Empathy).
 */

#ifndef __GAMES_UI_UTILS_H__
#define __GAMES_UI_UTILS_H__

#include <gtk/gtk.h>
#include <libxml/uri.h>
#include <libxml/tree.h>

#include <folks/folks.h>

#include "games-contact.h"

G_BEGIN_DECLS

#define STR_EMPTY(x) ((x) == NULL || (x)[0] == '\0')

#define GAMES_RECT_IS_ON_SCREEN(x,y,w,h) ((x) + (w) > 0 && \
					    (y) + (h) > 0 && \
					    (x) < gdk_screen_width () && \
					    (y) < gdk_screen_height ())

typedef void (*GamesPixbufAvatarFromIndividualCb) (FolksIndividual *individual,
		GdkPixbuf *pixbuf,
		gpointer user_data);

void            games_gtk_init                        (void);

/* Glade */
GtkBuilder *    games_builder_get_file                (const gchar      *filename,
							 const gchar      *first_object,
							 ...);
void            games_builder_connect                 (GtkBuilder       *gui,
							 gpointer          user_data,
							 const gchar      *first_object,
							 ...);
GtkWidget     *games_builder_unref_and_keep_widget    (GtkBuilder       *gui,
							 GtkWidget        *root);

/* Pixbufs */
const gchar * games_icon_name_for_presence            (TpConnectionPresenceType  presence);
const gchar * games_icon_name_for_contact             (GamesContact   *contact);
TpConnectionPresenceType games_folks_presence_type_to_tp (FolksPresenceType type);
const gchar * games_icon_name_for_individual          (FolksIndividual  *individual);
const gchar * games_protocol_name_for_contact         (GamesContact   *contact);
GdkPixbuf *   games_pixbuf_from_data                  (gchar            *data,
							 gsize             data_size);
GdkPixbuf *   games_pixbuf_from_data_and_mime         (gchar            *data,
							 gsize             data_size,
							 gchar           **mime_type);
void games_pixbuf_avatar_from_individual_scaled_async (FolksIndividual     *individual,
							 gint                 width,
							 gint                 height,
							 GCancellable        *cancellable,
							 GAsyncReadyCallback  callback,
							 gpointer             user_data);
GdkPixbuf * games_pixbuf_avatar_from_individual_scaled_finish (
							 FolksIndividual  *individual,
							 GAsyncResult     *result,
							 GError          **error);
GdkPixbuf *   games_pixbuf_from_avatar_scaled         (GamesAvatar    *avatar,
							 gint              width,
							 gint              height);
GdkPixbuf *   games_pixbuf_avatar_from_contact_scaled (GamesContact   *contact,
							 gint              width,
							 gint              height);
GdkPixbuf *   games_pixbuf_protocol_from_contact_scaled (GamesContact   *contact,
							 gint              width,
							 gint              height);
GdkPixbuf *   games_pixbuf_contact_status_icon (GamesContact   *contact,
							 gboolean          show_protocol);
GdkPixbuf *   games_pixbuf_contact_status_icon_with_icon_name (GamesContact   *contact,
							 const gchar       *icon_name,
							 gboolean          show_protocol);
GdkPixbuf *   games_pixbuf_scale_down_if_necessary    (GdkPixbuf        *pixbuf,
							 gint              max_size);
GdkPixbuf *   games_pixbuf_from_icon_name             (const gchar      *icon_name,
							 GtkIconSize       icon_size);
GdkPixbuf *   games_pixbuf_from_icon_name_sized       (const gchar      *icon_name,
							 gint              size);
gchar *       games_filename_from_icon_name           (const gchar      *icon_name,
							 GtkIconSize       icon_size);

/* Windows */
void        games_window_present                      (GtkWindow *window);
void        games_window_present_with_time            (GtkWindow        *window,
							 guint32 timestamp);
GtkWindow * games_get_toplevel_window                 (GtkWidget        *widget);

void games_move_to_window_desktop (GtkWindow *window,
    guint32 timestamp);

/* URL */
gchar *     games_make_absolute_url                   (const gchar      *url);

gchar *     games_make_absolute_url_len               (const gchar      *url,
							 guint             len);
void        games_url_show                            (GtkWidget        *parent,
							 const char       *url);

/* Misc */
gint64      games_get_current_action_time             (void);

gboolean games_individual_match_string (
    FolksIndividual *individual,
    const gchar *text,
    GPtrArray *words);

void games_launch_program (const gchar *dir,
    const gchar *name,
    const gchar *args);

gboolean    games_xml_validate                        (xmlDoc          *doc,
                                                       const gchar     *dtd_filename);

G_END_DECLS

#endif /*  __GAMES_UI_UTILS_H__ */
