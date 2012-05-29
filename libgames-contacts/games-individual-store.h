/* games-individual-store.h: Model Folks individuals 
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

#ifndef GAMES_INDIVIDUAL_STORE_H
#define GAMES_INDIVIDUAL_STORE_H

#include <gtk/gtk.h>
#include <folks/folks.h>

G_BEGIN_DECLS

#define GAMES_TYPE_INDIVIDUAL_STORE            (games_individual_store_get_type ())
#define GAMES_INDIVIDUAL_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GAMES_TYPE_INDIVIDUAL_STORE, GamesIndividualStore))
#define GAMES_INDIVIDUAL_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GAMES_TYPE_INDIVIDUAL_STORE, GamesIndividualStoreClass))
#define GAMES_IS_INDIVIDUAL_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GAMES_TYPE_INDIVIDUAL_STORE))
#define GAMES_IS_INDIVIDUAL_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GAMES_TYPE_INDIVIDUAL_STORE))
#define GAMES_INDIVIDUAL_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GAMES_TYPE_INDIVIDUAL_STORE, GamesIndividualStoreClass))

typedef struct _GamesIndividualStore        GamesIndividualStore;
typedef struct _GamesIndividualStoreClass   GamesIndividualStoreClass;
typedef struct _GamesIndividualStorePriv    GamesIndividualStorePriv;

typedef enum
{
  GAMES_INDIVIDUAL_STORE_SORT_STATE,
  GAMES_INDIVIDUAL_STORE_SORT_NAME
} GamesIndividualStoreSort;

typedef enum
{
  GAMES_INDIVIDUAL_STORE_COL_ICON_STATUS,
  GAMES_INDIVIDUAL_STORE_COL_PIXBUF_AVATAR,
  GAMES_INDIVIDUAL_STORE_COL_PIXBUF_AVATAR_VISIBLE,
  GAMES_INDIVIDUAL_STORE_COL_NAME,
  GAMES_INDIVIDUAL_STORE_COL_PRESENCE_TYPE,
  GAMES_INDIVIDUAL_STORE_COL_STATUS,
  GAMES_INDIVIDUAL_STORE_COL_COMPACT,
  GAMES_INDIVIDUAL_STORE_COL_INDIVIDUAL,
  GAMES_INDIVIDUAL_STORE_COL_IS_GROUP,
  GAMES_INDIVIDUAL_STORE_COL_IS_ACTIVE,
  GAMES_INDIVIDUAL_STORE_COL_IS_ONLINE,
  GAMES_INDIVIDUAL_STORE_COL_IS_SEPARATOR,
  GAMES_INDIVIDUAL_STORE_COL_INDIVIDUAL_CAPABILITIES,
  GAMES_INDIVIDUAL_STORE_COL_IS_FAKE_GROUP,
  GAMES_INDIVIDUAL_STORE_COL_COUNT,
} GamesIndividualStoreCol;

#define GAMES_INDIVIDUAL_STORE_UNGROUPED _("Ungrouped")
#define GAMES_INDIVIDUAL_STORE_FAVORITE  _("Favorite People")
#define GAMES_INDIVIDUAL_STORE_PEOPLE_NEARBY _("People Nearby")

struct _GamesIndividualStore{
  GtkListStore parent;

  /*< private >*/
  GamesIndividualStorePriv *priv;
};

struct _GamesIndividualStoreClass{
  GtkListStoreClass parent_class;

  /* class members */
  void (*reload_individuals) (GamesIndividualStore *self);
  gboolean (*initial_loading) (GamesIndividualStore *self);
};

/* used by GAMES_TYPE_INDIVIDUAL_STORE */
GType                 games_individual_store_get_type              (void) G_GNUC_CONST;

/* Function prototypes */

gboolean games_individual_store_get_show_avatars (
    GamesIndividualStore *store);

void games_individual_store_set_show_avatars (GamesIndividualStore *store,
    gboolean show_avatars);

gboolean games_individual_store_get_show_groups (
    GamesIndividualStore *store);

void games_individual_store_set_show_groups (GamesIndividualStore *store,
    gboolean show_groups);

gboolean games_individual_store_get_is_compact (
    GamesIndividualStore *store);

void games_individual_store_set_is_compact (GamesIndividualStore *store,
    gboolean is_compact);

gboolean games_individual_store_get_show_protocols (
    GamesIndividualStore *store);

void games_individual_store_set_show_protocols (
    GamesIndividualStore *store,
    gboolean show_protocols);

GamesIndividualStoreSort games_individual_store_get_sort_criterium (
    GamesIndividualStore *store);

void games_individual_store_set_sort_criterium (
    GamesIndividualStore *store,
    GamesIndividualStoreSort sort_criterium);

gboolean games_individual_store_row_separator_func (GtkTreeModel *model,
    GtkTreeIter *iter,
    gpointer data);

gchar *games_individual_store_get_parent_group (GtkTreeModel *model,
    GtkTreePath *path,
    gboolean *path_is_group,
    gboolean *is_fake_group);

GdkPixbuf *games_individual_store_get_individual_status_icon (
    GamesIndividualStore *store,
    FolksIndividual *individual);

void individual_store_add_individual_and_connect (
    GamesIndividualStore *self,
    FolksIndividual *individual);

void individual_store_remove_individual_and_disconnect (
    GamesIndividualStore *self,
    FolksIndividual *individual);

/* protected */

void games_individual_store_disconnect_individual (
    GamesIndividualStore *self,
    FolksIndividual *individual);

void games_individual_store_remove_individual (
    GamesIndividualStore *self,
    FolksIndividual *individual);

void games_individual_store_add_individual (
    GamesIndividualStore *self,
    FolksIndividual *individual);

void games_individual_store_refresh_individual (
    GamesIndividualStore *self,
    FolksIndividual *individual);


G_END_DECLS
#endif /* GAMES_INDIVIDUAL_STORE_H */
/* EOF */
