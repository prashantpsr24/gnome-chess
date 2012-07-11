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
 */

 /*
 * Live search is taken from Empathy (https://live.gnome.org/Empathy)
 * Copyright (C) 2010 Collabora Ltd.
 * Copyright (C) 2007-2010 Nokia Corporation.
 */

#ifndef __GAMES_LIVE_SEARCH_H__
#define __GAMES_LIVE_SEARCH_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GAMES_TYPE_LIVE_SEARCH         (games_live_search_get_type ())
#define GAMES_LIVE_SEARCH(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GAMES_TYPE_LIVE_SEARCH, GamesLiveSearch))
#define GAMES_LIVE_SEARCH_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GAMES_TYPE_LIVE_SEARCH, GamesLiveSearchClass))
#define GAMES_IS_LIVE_SEARCH(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GAMES_TYPE_LIVE_SEARCH))
#define GAMES_IS_LIVE_SEARCH_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GAMES_TYPE_LIVE_SEARCH))
#define GAMES_LIVE_SEARCH_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GAMES_TYPE_LIVE_SEARCH, GamesLiveSearchClass))

typedef struct _GamesLiveSearch      GamesLiveSearch;
typedef struct _GamesLiveSearchClass GamesLiveSearchClass;
typedef struct _GamesLiveSearchPriv  GamesLiveSearchPriv;

struct _GamesLiveSearch {
  GtkHBox parent;

  /*<private>*/
  GamesLiveSearchPriv *priv;
};

struct _GamesLiveSearchClass {
  GtkHBoxClass parent_class;
};

GType games_live_search_get_type (void) G_GNUC_CONST;
GtkWidget *games_live_search_new (GtkWidget *hook);

GtkWidget *games_live_search_get_hook_widget (GamesLiveSearch *self);
void games_live_search_set_hook_widget (GamesLiveSearch *self,
    GtkWidget *hook);

const gchar *games_live_search_get_text (GamesLiveSearch *self);
void games_live_search_set_text (GamesLiveSearch *self,
    const gchar *text);

gboolean games_live_search_match (GamesLiveSearch *self,
    const gchar *string);

GPtrArray * games_live_search_strip_utf8_string (const gchar *string);

gboolean games_live_search_match_words (const gchar *string,
    GPtrArray *words);

GPtrArray * games_live_search_get_words (GamesLiveSearch *self);

/* Made public for unit tests */
gboolean games_live_search_match_string (const gchar *string,
   const gchar *prefix);

G_END_DECLS

#endif /* __GAMES_LIVE_SEARCH_H__ */
