/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
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
 * Author: Chandni Verma <chandniverma2112@gmail.com>
 */

/* Much of code here is a simplified and adapted version of the way
 * contacts are displayed in Empathy, a chat application using Folks
 * (https://live.gnome.org/Empathy)
 * Copyright (C) 2005-2007 Imendio AB
 * Copyright (C) 2007-2010 Collabora Ltd.
 */

#include "config.h"

#include <string.h>

#include <glib/gi18n-lib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <telepathy-glib/account-manager.h>
#include <telepathy-glib/util.h>

#include <folks/folks.h>
#include <folks/folks-telepathy.h>

#include <libgames-contacts/games-individual-manager.h>
#include <libgames-contacts/games-individual-groups.h>
#include <libgames-contacts/games-ui-utils.h>

#include "games-individual-view.h"
#include "games-individual-store.h"
#include "games-cell-renderer-expander.h"
#include "games-cell-renderer-text.h"
#include "games-ui-utils.h"
#include "games-enum-types.h"

/* Active users are those which have recently changed state
 * (e.g. online, offline or from normal to a busy state).
 */

struct _GamesIndividualViewPriv
{
  GamesIndividualStore *store;
  GamesIndividualViewFeatureFlags view_features;

  gboolean show_untrusted;
  gboolean show_uninteresting;

  GtkTreeModelFilter *filter;
  GtkWidget *search_widget;

  guint expand_groups_idle_handler;
  /* owned string (group name) -> bool (whether to expand/contract) */
  GHashTable *expand_groups;

  GtkTreeModelFilterVisibleFunc custom_filter;
  gpointer custom_filter_data;

  GtkCellRenderer *text_renderer;
};

enum
{
  PROP_0,
  PROP_STORE,
  PROP_VIEW_FEATURES,
  PROP_SHOW_UNTRUSTED,
  PROP_SHOW_UNINTERESTING,
};

G_DEFINE_TYPE (GamesIndividualView, games_individual_view,
    GTK_TYPE_TREE_VIEW);


static void
individual_view_cell_set_background (GamesIndividualView *view,
    GtkCellRenderer *cell,
    gboolean is_group,
    gboolean is_active)
{
  if (!is_group && is_active)
    {
      GtkStyleContext *style;
      GdkRGBA color;

      style = gtk_widget_get_style_context (GTK_WIDGET (view));

      gtk_style_context_get_background_color (style, GTK_STATE_FLAG_SELECTED,
          &color);

      /* Here we take the current theme colour and add it to
       * the colour for white and average the two. This
       * gives a colour which is inline with the theme but
       * slightly whiter.
       */
      const GdkRGBA white = { 1.0, 1.0, 1.0, 1.0 };

     	color.red = (color.red + white.red) / 2;
      color.green = (color.green + white.green) / 2;
      color.blue = (color.blue + white.blue) / 2;

      g_object_set (cell, "cell-background-rgba", &color, NULL);
    }
  else
    g_object_set (cell, "cell-background-rgba", NULL, NULL);
}

static void
individual_view_pixbuf_cell_data_func (GtkTreeViewColumn *tree_column,
    GtkCellRenderer *cell,
    GtkTreeModel *model,
    GtkTreeIter *iter,
    GamesIndividualView *view)
{
  GdkPixbuf *pixbuf;
  gboolean is_group;
  gboolean is_active;

  gtk_tree_model_get (model, iter,
      GAMES_INDIVIDUAL_STORE_COL_IS_GROUP, &is_group,
      GAMES_INDIVIDUAL_STORE_COL_IS_ACTIVE, &is_active,
      GAMES_INDIVIDUAL_STORE_COL_ICON_STATUS, &pixbuf, -1);

  g_object_set (cell,
      "visible", !is_group,
      "pixbuf", pixbuf,
      NULL);

  tp_clear_object (&pixbuf);

  individual_view_cell_set_background (view, cell, is_group, is_active);
}

static void
individual_view_group_icon_cell_data_func (GtkTreeViewColumn *tree_column,
    GtkCellRenderer *cell,
    GtkTreeModel *model,
    GtkTreeIter *iter,
    GamesIndividualView *view)
{
  GdkPixbuf *pixbuf = NULL;
  gboolean is_group;
  gchar *name;

  gtk_tree_model_get (model, iter,
      GAMES_INDIVIDUAL_STORE_COL_IS_GROUP, &is_group,
      GAMES_INDIVIDUAL_STORE_COL_NAME, &name, -1);

  if (!is_group)
    goto out;

  if (!tp_strdiff (name, GAMES_INDIVIDUAL_STORE_FAVORITE))
    {
      pixbuf = games_pixbuf_from_icon_name ("emblem-favorite",
          GTK_ICON_SIZE_MENU);
    }
  else if (!tp_strdiff (name, GAMES_INDIVIDUAL_STORE_PEOPLE_NEARBY))
    {
      pixbuf = games_pixbuf_from_icon_name ("im-local-xmpp",
          GTK_ICON_SIZE_MENU);
    }

out:
  g_object_set (cell,
      "visible", pixbuf != NULL,
      "pixbuf", pixbuf,
      NULL);

  tp_clear_object (&pixbuf);

  g_free (name);
}

static void
individual_view_avatar_cell_data_func (GtkTreeViewColumn *tree_column,
    GtkCellRenderer *cell,
    GtkTreeModel *model,
    GtkTreeIter *iter,
    GamesIndividualView *view)
{
  GdkPixbuf *pixbuf;
  gboolean show_avatar;
  gboolean is_group;
  gboolean is_active;

  gtk_tree_model_get (model, iter,
      GAMES_INDIVIDUAL_STORE_COL_PIXBUF_AVATAR, &pixbuf,
      GAMES_INDIVIDUAL_STORE_COL_PIXBUF_AVATAR_VISIBLE, &show_avatar,
      GAMES_INDIVIDUAL_STORE_COL_IS_GROUP, &is_group,
      GAMES_INDIVIDUAL_STORE_COL_IS_ACTIVE, &is_active, -1);

  g_object_set (cell,
      "visible", !is_group && show_avatar,
      "pixbuf", pixbuf,
      NULL);

  tp_clear_object (&pixbuf);

  individual_view_cell_set_background (view, cell, is_group, is_active);
}

static void
individual_view_text_cell_data_func (GtkTreeViewColumn *tree_column,
    GtkCellRenderer *cell,
    GtkTreeModel *model,
    GtkTreeIter *iter,
    GamesIndividualView *view)
{
  gboolean is_group;
  gboolean is_active;

  gtk_tree_model_get (model, iter,
      GAMES_INDIVIDUAL_STORE_COL_IS_GROUP, &is_group,
      GAMES_INDIVIDUAL_STORE_COL_IS_ACTIVE, &is_active, -1);

  individual_view_cell_set_background (view, cell, is_group, is_active);
}

static void
individual_view_expander_cell_data_func (GtkTreeViewColumn *column,
    GtkCellRenderer *cell,
    GtkTreeModel *model,
    GtkTreeIter *iter,
    GamesIndividualView *view)
{
  gboolean is_group;
  gboolean is_active;

  gtk_tree_model_get (model, iter,
      GAMES_INDIVIDUAL_STORE_COL_IS_GROUP, &is_group,
      GAMES_INDIVIDUAL_STORE_COL_IS_ACTIVE, &is_active, -1);

  if (gtk_tree_model_iter_has_child (model, iter))
    {
      GtkTreePath *path;
      gboolean row_expanded;

      path = gtk_tree_model_get_path (model, iter);
      row_expanded =
          gtk_tree_view_row_expanded (GTK_TREE_VIEW
          (gtk_tree_view_column_get_tree_view (column)), path);
      gtk_tree_path_free (path);

      g_object_set (cell,
          "visible", TRUE,
          "expander-style",
          row_expanded ? GTK_EXPANDER_EXPANDED : GTK_EXPANDER_COLLAPSED,
          NULL);
    }
  else
    g_object_set (cell, "visible", FALSE, NULL);

  individual_view_cell_set_background (view, cell, is_group, is_active);
}

static void
individual_view_row_expand_or_collapse_cb (GamesIndividualView *view,
    GtkTreeIter *iter,
    GtkTreePath *path,
    gpointer user_data)
{
  GtkTreeModel *model;
  gchar *name;
  gboolean expanded;

  if (!(view->priv->view_features & GAMES_INDIVIDUAL_VIEW_FEATURE_GROUPS_SAVE))
    return;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));

  gtk_tree_model_get (model, iter,
      GAMES_INDIVIDUAL_STORE_COL_NAME, &name, -1);

  expanded = GPOINTER_TO_INT (user_data);
  games_individual_group_set_expanded (name, expanded);

  g_free (name);
}

static gboolean
individual_view_start_search_cb (GamesIndividualView *view,
    gpointer data)
{
  if (view->priv->search_widget == NULL)
    return FALSE;

  games_individual_view_start_search (view);

  return TRUE;
}

static void
individual_view_search_text_notify_cb (GamesLiveSearch *search,
    GParamSpec *pspec,
    GamesIndividualView *view)
{
  GtkTreePath *path;
  GtkTreeViewColumn *focus_column;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean set_cursor = FALSE;

  gtk_tree_model_filter_refilter (view->priv->filter);

  /* Set cursor on the first contact. If it is already set on a group,
   * set it on its first child contact. Note that first child of a group
   * is its separator, that's why we actually set to the 2nd
   */

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
  gtk_tree_view_get_cursor (GTK_TREE_VIEW (view), &path, &focus_column);

  if (path == NULL)
    {
      path = gtk_tree_path_new_from_string ("0:1");
      set_cursor = TRUE;
    }
  else if (gtk_tree_path_get_depth (path) < 2)
    {
      gboolean is_group;

      gtk_tree_model_get_iter (model, &iter, path);
      gtk_tree_model_get (model, &iter,
          GAMES_INDIVIDUAL_STORE_COL_IS_GROUP, &is_group,
          -1);

      if (is_group)
        {
          gtk_tree_path_down (path);
          gtk_tree_path_next (path);
          set_cursor = TRUE;
        }
    }

  if (set_cursor)
    {
      /* Make sure the path is valid. */
      if (gtk_tree_model_get_iter (model, &iter, path))
        {
          gtk_tree_view_set_cursor (GTK_TREE_VIEW (view), path, focus_column,
              FALSE);
        }
    }

  gtk_tree_path_free (path);
}

static void
individual_view_search_activate_cb (GtkWidget *search,
  GamesIndividualView *view)
{
  GtkTreePath *path;
  GtkTreeViewColumn *focus_column;

  gtk_tree_view_get_cursor (GTK_TREE_VIEW (view), &path, &focus_column);
  if (path != NULL)
    {
      gtk_tree_view_row_activated (GTK_TREE_VIEW (view), path, focus_column);
      gtk_tree_path_free (path);

      gtk_widget_hide (search);
    }
}

static gboolean
individual_view_search_key_navigation_cb (GtkWidget *search,
  GdkEvent *event,
  GamesIndividualView *view)
{
  GdkEvent *new_event;
  gboolean ret = FALSE;

  new_event = gdk_event_copy (event);
  gtk_widget_grab_focus (GTK_WIDGET (view));
  ret = gtk_widget_event (GTK_WIDGET (view), new_event);
  gtk_widget_grab_focus (search);

  gdk_event_free (new_event);

  return ret;
}

static void
individual_view_search_hide_cb (GamesLiveSearch *search,
    GamesIndividualView *view)
{
  GtkTreeModel *model;
  GtkTreePath *cursor_path;
  GtkTreeIter iter;
  gboolean valid = FALSE;

  /* block expand or collapse handlers, they would write the
   * expand or collapsed setting to file otherwise */
  g_signal_handlers_block_by_func (view,
      individual_view_row_expand_or_collapse_cb, GINT_TO_POINTER (TRUE));
  g_signal_handlers_block_by_func (view,
    individual_view_row_expand_or_collapse_cb, GINT_TO_POINTER (FALSE));

  /* restore which groups are expanded and which are not */
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
  for (valid = gtk_tree_model_get_iter_first (model, &iter);
       valid; valid = gtk_tree_model_iter_next (model, &iter))
    {
      gboolean is_group;
      gchar *name = NULL;
      GtkTreePath *path;

      gtk_tree_model_get (model, &iter,
          GAMES_INDIVIDUAL_STORE_COL_NAME, &name,
          GAMES_INDIVIDUAL_STORE_COL_IS_GROUP, &is_group,
          -1);

      if (!is_group)
        {
          g_free (name);
          continue;
        }

      path = gtk_tree_model_get_path (model, &iter);
      if ((view->priv->view_features &
            GAMES_INDIVIDUAL_VIEW_FEATURE_GROUPS_SAVE) == 0 ||
          games_individual_group_get_expanded (name))
        {
          gtk_tree_view_expand_row (GTK_TREE_VIEW (view), path, TRUE);
        }
      else
        {
          gtk_tree_view_collapse_row (GTK_TREE_VIEW (view), path);
        }

      gtk_tree_path_free (path);
      g_free (name);
    }

  /* unblock expand or collapse handlers */
  g_signal_handlers_unblock_by_func (view,
      individual_view_row_expand_or_collapse_cb, GINT_TO_POINTER (TRUE));
  g_signal_handlers_unblock_by_func (view,
      individual_view_row_expand_or_collapse_cb, GINT_TO_POINTER (FALSE));

  /* keep the selected contact visible */
  gtk_tree_view_get_cursor (GTK_TREE_VIEW (view), &cursor_path, NULL);

  if (cursor_path != NULL)
    gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (view), cursor_path, NULL,
        FALSE, 0, 0);

  gtk_tree_path_free (cursor_path);
}

static void
individual_view_search_show_cb (GamesLiveSearch *search,
    GamesIndividualView *view)
{
  /* block expand or collapse handlers during expand all, they would
   * write the expand or collapsed setting to file otherwise */
  g_signal_handlers_block_by_func (view,
      individual_view_row_expand_or_collapse_cb, GINT_TO_POINTER (TRUE));

  gtk_tree_view_expand_all (GTK_TREE_VIEW (view));

  g_signal_handlers_unblock_by_func (view,
      individual_view_row_expand_or_collapse_cb, GINT_TO_POINTER (TRUE));
}

static gboolean
expand_idle_foreach_cb (GtkTreeModel *model,
    GtkTreePath *path,
    GtkTreeIter *iter,
    GamesIndividualView *self)
{
  gboolean is_group;
  gpointer should_expand;
  gchar *name;

  /* We only want groups */
  if (gtk_tree_path_get_depth (path) > 1)
    return FALSE;

  gtk_tree_model_get (model, iter,
      GAMES_INDIVIDUAL_STORE_COL_IS_GROUP, &is_group,
      GAMES_INDIVIDUAL_STORE_COL_NAME, &name,
      -1);

  if (!is_group)
    {
      g_free (name);
      return FALSE;
    }

  if (g_hash_table_lookup_extended (self->priv->expand_groups, name, NULL,
      &should_expand))
    {
      if (GPOINTER_TO_INT (should_expand))
        gtk_tree_view_expand_row (GTK_TREE_VIEW (self), path, FALSE);
      else
        gtk_tree_view_collapse_row (GTK_TREE_VIEW (self), path);

      g_hash_table_remove (self->priv->expand_groups, name);
    }

  g_free (name);

  return FALSE;
}

static gboolean
individual_view_expand_idle_cb (GamesIndividualView *self)
{
  g_signal_handlers_block_by_func (self,
    individual_view_row_expand_or_collapse_cb, GINT_TO_POINTER (TRUE));
  g_signal_handlers_block_by_func (self,
    individual_view_row_expand_or_collapse_cb, GINT_TO_POINTER (FALSE));

  /* The store/filter could've been removed while we were in the idle queue */
  if (self->priv->filter != NULL)
    {
      gtk_tree_model_foreach (GTK_TREE_MODEL (self->priv->filter),
          (GtkTreeModelForeachFunc) expand_idle_foreach_cb, self);
    }

  g_signal_handlers_unblock_by_func (self,
      individual_view_row_expand_or_collapse_cb, GINT_TO_POINTER (FALSE));
  g_signal_handlers_unblock_by_func (self,
      individual_view_row_expand_or_collapse_cb, GINT_TO_POINTER (TRUE));

  /* Empty the table of groups to expand/contract, since it may contain groups
   * which no longer exist in the tree view. This can happen after going
   * offline, for example. */
  g_hash_table_remove_all (self->priv->expand_groups);
  self->priv->expand_groups_idle_handler = 0;
  g_object_unref (self);

  return FALSE;
}

static void
individual_view_row_has_child_toggled_cb (GtkTreeModel *model,
    GtkTreePath *path,
    GtkTreeIter *iter,
    GamesIndividualView *view)
{
  gboolean should_expand, is_group = FALSE;
  gchar *name = NULL;
  gpointer will_expand;

  gtk_tree_model_get (model, iter,
      GAMES_INDIVIDUAL_STORE_COL_IS_GROUP, &is_group,
      GAMES_INDIVIDUAL_STORE_COL_NAME, &name,
      -1);

  if (!is_group || STR_EMPTY (name))
    {
      g_free (name);
      return;
    }

  should_expand = (view->priv->view_features &
          GAMES_INDIVIDUAL_VIEW_FEATURE_GROUPS_SAVE) == 0 ||
      (view->priv->search_widget != NULL &&
          gtk_widget_get_visible (view->priv->search_widget)) ||
      games_individual_group_get_expanded (name);

  /* FIXME: It doesn't work to call gtk_tree_view_expand_row () from within
   * gtk_tree_model_filter_refilter (). We add the rows to expand/contract to
   * a hash table, and expand or contract them as appropriate all at once in
   * an idle handler which iterates over all the group rows. */
  if (!g_hash_table_lookup_extended (view->priv->expand_groups, name, NULL,
      &will_expand) ||
      GPOINTER_TO_INT (will_expand) != should_expand)
    {
      g_hash_table_insert (view->priv->expand_groups, g_strdup (name),
          GINT_TO_POINTER (should_expand));

      if (view->priv->expand_groups_idle_handler == 0)
        {
          view->priv->expand_groups_idle_handler =
              g_idle_add ((GSourceFunc) individual_view_expand_idle_cb,
                  g_object_ref (view));
        }
    }

  g_free (name);
}

static gboolean
individual_view_is_visible_individual (GamesIndividualView *self,
    FolksIndividual *individual,
    gboolean is_online,
    gboolean is_searching,
    const gchar *group,
    gboolean is_fake_group,
    GamesCapabilities individual_caps,
    GamesActionType interest)
{
  GamesLiveSearch *live = GAMES_LIVE_SEARCH (self->priv->search_widget);
  gboolean is_favorite;

  /* We're only giving the visibility wrt filtering here, not things like
   * presence. */
  if (!self->priv->show_untrusted &&
      folks_individual_get_trust_level (individual) == FOLKS_TRUST_LEVEL_NONE)
    {
      return FALSE;
    }

  if (!self->priv->show_uninteresting)
    {
      /* Hide all uninteresting individuals */
      switch (interest)
      {
        case GAMES_ACTION_CHAT:
            /* All individuals in individuals contain TpContacts and hence
             * support chats. Proceed to calculate visibility. */
            break;

        case GAMES_ACTION_PLAY_GLCHESS:
            if ((individual_caps & GAMES_ACTION_PLAY_GLCHESS) == 0)
              return FALSE;
            /*else proceed */
            break;

        default:
            return FALSE;
      }
    }

  is_favorite = folks_favourite_details_get_is_favourite (
      FOLKS_FAVOURITE_DETAILS (individual));
  if (!is_searching) {
    if (is_favorite && is_fake_group &&
        !tp_strdiff (group, GAMES_INDIVIDUAL_STORE_FAVORITE))
        /* Always display favorite contacts in the favorite group */
        return TRUE;

    return (is_online);
  }

  return games_individual_match_string (individual,
      games_live_search_get_text (live),
      games_live_search_get_words (live));
}

static gchar *
get_group (GtkTreeModel *model,
    GtkTreeIter *iter,
    gboolean *is_fake)
{
  GtkTreeIter parent_iter;
  gchar *name = NULL;

  *is_fake = FALSE;

  if (!gtk_tree_model_iter_parent (model, &parent_iter, iter))
    return NULL;

  gtk_tree_model_get (model, &parent_iter,
      GAMES_INDIVIDUAL_STORE_COL_NAME, &name,
      GAMES_INDIVIDUAL_STORE_COL_IS_FAKE_GROUP, is_fake,
      -1);

  return name;
}

gboolean
individual_view_filter_default (GtkTreeModel *model,
    GtkTreeIter *iter,
    gpointer user_data,
    GamesActionType interest)
{
  GamesIndividualView *self = GAMES_INDIVIDUAL_VIEW (user_data);
  FolksIndividual *individual = NULL;
  gboolean is_group, is_separator, valid;
  GtkTreeIter child_iter;
  gboolean visible, is_online;
  gboolean is_searching = TRUE;
  GamesCapabilities individual_caps;

  if (self->priv->search_widget == NULL ||
      !gtk_widget_get_visible (self->priv->search_widget))
     is_searching = FALSE;

  gtk_tree_model_get (model, iter,
      GAMES_INDIVIDUAL_STORE_COL_IS_GROUP, &is_group,
      GAMES_INDIVIDUAL_STORE_COL_IS_SEPARATOR, &is_separator,
      GAMES_INDIVIDUAL_STORE_COL_IS_ONLINE, &is_online,
      GAMES_INDIVIDUAL_STORE_COL_INDIVIDUAL, &individual,
      GAMES_INDIVIDUAL_STORE_COL_INDIVIDUAL_CAPABILITIES, &individual_caps,
      -1);

  if (individual != NULL)
    {
      gchar *group;
      gboolean is_fake_group;

      group = get_group (model, iter, &is_fake_group);

      visible = individual_view_is_visible_individual (self, individual,
          is_online, is_searching, group, is_fake_group, individual_caps,
          interest);

      g_object_unref (individual);
      g_free (group);

      return visible;
    }

  if (is_separator)
    return TRUE;

  /* Not a contact, not a separator, must be a group */
  g_return_val_if_fail (is_group, FALSE);

  /* only show groups which are not empty */
  for (valid = gtk_tree_model_iter_children (model, &child_iter, iter);
       valid; valid = gtk_tree_model_iter_next (model, &child_iter))
    {
      gchar *group;
      gboolean is_fake_group;

      gtk_tree_model_get (model, &child_iter,
        GAMES_INDIVIDUAL_STORE_COL_INDIVIDUAL, &individual,
        GAMES_INDIVIDUAL_STORE_COL_IS_ONLINE, &is_online,
        GAMES_INDIVIDUAL_STORE_COL_INDIVIDUAL_CAPABILITIES,
            &individual_caps,
        -1);

      if (individual == NULL)
        continue;

      group = get_group (model, &child_iter, &is_fake_group);

      visible = individual_view_is_visible_individual (self, individual,
          is_online, is_searching, group, is_fake_group, individual_caps,
          interest);

      g_object_unref (individual);
      g_free (group);

      /* show group if it has at least one visible contact in it */
      if (visible)
        return TRUE;
    }

  return FALSE;
}

static gboolean
individual_view_filter_visible_func (GtkTreeModel *model,
    GtkTreeIter *iter,
    gpointer user_data)
{
  GamesIndividualView *self = GAMES_INDIVIDUAL_VIEW (user_data);

  if (self->priv->custom_filter != NULL)
    return self->priv->custom_filter (model, iter,
        self->priv->custom_filter_data);
  else
    return individual_view_filter_default (model, iter, user_data,
        GAMES_ACTION_CHAT);
}

static void
individual_view_constructed (GObject *object)
{
  GamesIndividualView *view = GAMES_INDIVIDUAL_VIEW (object);
  GtkCellRenderer *cell;
  GtkTreeViewColumn *col;

  /* Setup view */
  g_object_set (view,
      "headers-visible", FALSE,
      "show-expanders", FALSE,
      NULL);

  col = gtk_tree_view_column_new ();

  /* State */
  cell = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (col, cell, FALSE);
  gtk_tree_view_column_set_cell_data_func (col, cell,
      (GtkTreeCellDataFunc) individual_view_pixbuf_cell_data_func,
      view, NULL);

  g_object_set (cell,
      "xpad", 5,
      "ypad", 1,
      "visible", FALSE,
      NULL);

  /* Group icon */
  cell = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (col, cell, FALSE);
  gtk_tree_view_column_set_cell_data_func (col, cell,
      (GtkTreeCellDataFunc) individual_view_group_icon_cell_data_func,
      view, NULL);

  g_object_set (cell,
      "xpad", 0,
      "ypad", 0,
      "visible", FALSE,
      "width", 16,
      "height", 16,
      NULL);

  /* Name */
  view->priv->text_renderer = games_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (col, view->priv->text_renderer, TRUE);
  gtk_tree_view_column_set_cell_data_func (col, view->priv->text_renderer,
      (GtkTreeCellDataFunc) individual_view_text_cell_data_func, view, NULL);

  gtk_tree_view_column_add_attribute (col, view->priv->text_renderer,
      "name", GAMES_INDIVIDUAL_STORE_COL_NAME);
  gtk_tree_view_column_add_attribute (col, view->priv->text_renderer,
      "text", GAMES_INDIVIDUAL_STORE_COL_NAME);
  gtk_tree_view_column_add_attribute (col, view->priv->text_renderer,
      "presence-type", GAMES_INDIVIDUAL_STORE_COL_PRESENCE_TYPE);
  gtk_tree_view_column_add_attribute (col, view->priv->text_renderer,
      "status", GAMES_INDIVIDUAL_STORE_COL_STATUS);
  gtk_tree_view_column_add_attribute (col, view->priv->text_renderer,
      "is_group", GAMES_INDIVIDUAL_STORE_COL_IS_GROUP);
  gtk_tree_view_column_add_attribute (col, view->priv->text_renderer,
      "compact", GAMES_INDIVIDUAL_STORE_COL_COMPACT);

  /* Avatar */
  cell = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (col, cell, FALSE);
  gtk_tree_view_column_set_cell_data_func (col, cell,
      (GtkTreeCellDataFunc) individual_view_avatar_cell_data_func,
      view, NULL);

  g_object_set (cell,
      "xpad", 0,
      "ypad", 0,
      "visible", FALSE,
      "width", 32,
      "height", 32,
      NULL);

  /* Expander */
  cell = games_cell_renderer_expander_new ();
  gtk_tree_view_column_pack_end (col, cell, FALSE);
  gtk_tree_view_column_set_cell_data_func (col, cell,
      (GtkTreeCellDataFunc) individual_view_expander_cell_data_func,
      view, NULL);

  /* Actually add the column now we have added all cell renderers */
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), col);
}

static void
individual_view_set_view_features (GamesIndividualView *view,
    GamesIndividualViewFeatureFlags features)
{
  g_return_if_fail (GAMES_IS_INDIVIDUAL_VIEW (view));

  view->priv->view_features = features;
}

static void
individual_view_dispose (GObject *object)
{
  GamesIndividualView *view = GAMES_INDIVIDUAL_VIEW (object);

  tp_clear_object (&view->priv->store);
  tp_clear_object (&view->priv->filter);

  games_individual_view_set_live_search (view, NULL);

  G_OBJECT_CLASS (games_individual_view_parent_class)->dispose (object);
}

static void
individual_view_finalize (GObject *object)
{
  GamesIndividualView *self = GAMES_INDIVIDUAL_VIEW (object);

  if (self->priv->expand_groups_idle_handler != 0)
    g_source_remove (self->priv->expand_groups_idle_handler);
  g_hash_table_unref (self->priv->expand_groups);

  G_OBJECT_CLASS (games_individual_view_parent_class)->finalize (object);
}

static void
individual_view_get_property (GObject *object,
    guint param_id,
    GValue *value,
    GParamSpec *pspec)
{
  GamesIndividualView *self = GAMES_INDIVIDUAL_VIEW (object);

  switch (param_id)
    {
    case PROP_STORE:
      g_value_set_object (value, self->priv->store);
      break;
    case PROP_VIEW_FEATURES:
      g_value_set_flags (value, self->priv->view_features);
      break;
    case PROP_SHOW_UNTRUSTED:
      g_value_set_boolean (value, self->priv->show_untrusted);
      break;
    case PROP_SHOW_UNINTERESTING:
      g_value_set_boolean (value, self->priv->show_uninteresting);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    };
}

static void
individual_view_set_property (GObject *object,
    guint param_id,
    const GValue *value,
    GParamSpec *pspec)
{
  GamesIndividualView *view = GAMES_INDIVIDUAL_VIEW (object);

  switch (param_id)
    {
    case PROP_STORE:
      games_individual_view_set_store (view, g_value_get_object (value));
      break;
    case PROP_VIEW_FEATURES:
      individual_view_set_view_features (view, g_value_get_flags (value));
      break;
    case PROP_SHOW_UNTRUSTED:
      games_individual_view_set_show_untrusted (view,
          g_value_get_boolean (value));
      break;
    case PROP_SHOW_UNINTERESTING:
      games_individual_view_set_show_uninteresting (view,
          g_value_get_boolean (value));
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    };
}

static void
games_individual_view_class_init (GamesIndividualViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = individual_view_constructed;
  object_class->dispose = individual_view_dispose;
  object_class->finalize = individual_view_finalize;
  object_class->get_property = individual_view_get_property;
  object_class->set_property = individual_view_set_property;

  g_object_class_install_property (object_class,
      PROP_STORE,
      g_param_spec_object ("store",
          "The store of the view",
          "The store of the view",
          GAMES_TYPE_INDIVIDUAL_STORE,
          G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
      PROP_VIEW_FEATURES,
      g_param_spec_flags ("view-features",
          "Features of the view",
          "Flags for all enabled features",
          GAMES_TYPE_INDIVIDUAL_VIEW_FEATURE_FLAGS,
          GAMES_INDIVIDUAL_VIEW_FEATURE_NONE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
      PROP_SHOW_UNTRUSTED,
      g_param_spec_boolean ("show-untrusted",
          "Show Untrusted Individuals",
          "Whether the view should display untrusted individuals; "
          "those who could not be who they say they are.",
          TRUE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class,
      PROP_SHOW_UNINTERESTING,
      g_param_spec_boolean ("show-uninteresting",
          "Show Uninteresting Individuals",
          "Whether the view should not filter out individuals using "
          "capability checks against view's interest. ",
          FALSE, G_PARAM_READWRITE));

  g_type_class_add_private (object_class, sizeof (GamesIndividualViewPriv));
}

static void
games_individual_view_init (GamesIndividualView *view)
{
  GamesIndividualViewPriv *priv = G_TYPE_INSTANCE_GET_PRIVATE (view,
      GAMES_TYPE_INDIVIDUAL_VIEW, GamesIndividualViewPriv);

  view->priv = priv;

  priv->show_untrusted = TRUE;
  priv->show_uninteresting = FALSE;

  /* Get saved group states. */
  games_individual_groups_get_all ();

  priv->expand_groups = g_hash_table_new_full (g_str_hash, g_str_equal,
      (GDestroyNotify) g_free, NULL);

  gtk_tree_view_set_row_separator_func (GTK_TREE_VIEW (view),
      games_individual_store_row_separator_func, NULL, NULL);

  /* Connect to tree view signals rather than override. */
  g_signal_connect (view, "row-expanded",
      G_CALLBACK (individual_view_row_expand_or_collapse_cb),
      GINT_TO_POINTER (TRUE));
  g_signal_connect (view, "row-collapsed",
      G_CALLBACK (individual_view_row_expand_or_collapse_cb),
      GINT_TO_POINTER (FALSE));
}

GamesIndividualView *
games_individual_view_new (GamesIndividualStore *store,
    GamesIndividualViewFeatureFlags view_features)
{
  g_return_val_if_fail (GAMES_IS_INDIVIDUAL_STORE (store), NULL);

  return g_object_new (GAMES_TYPE_INDIVIDUAL_VIEW,
      "store", store,
      "view-features", view_features, NULL);
}

FolksIndividual *
games_individual_view_dup_selected (GamesIndividualView *view)
{
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  GtkTreeModel *model;
  FolksIndividual *individual;

  g_return_val_if_fail (GAMES_IS_INDIVIDUAL_VIEW (view), NULL);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    return NULL;

  gtk_tree_model_get (model, &iter,
      GAMES_INDIVIDUAL_STORE_COL_INDIVIDUAL, &individual, -1);

  return individual;
}

void
games_individual_view_set_live_search (GamesIndividualView *view,
    GamesLiveSearch *search)
{
  /* remove old handlers if old search was not null */
  if (view->priv->search_widget != NULL)
    {
      g_signal_handlers_disconnect_by_func (view,
          individual_view_start_search_cb, NULL);

      g_signal_handlers_disconnect_by_func (view->priv->search_widget,
          individual_view_search_text_notify_cb, view);
      g_signal_handlers_disconnect_by_func (view->priv->search_widget,
          individual_view_search_activate_cb, view);
      g_signal_handlers_disconnect_by_func (view->priv->search_widget,
          individual_view_search_key_navigation_cb, view);
      g_signal_handlers_disconnect_by_func (view->priv->search_widget,
          individual_view_search_hide_cb, view);
      g_signal_handlers_disconnect_by_func (view->priv->search_widget,
          individual_view_search_show_cb, view);
      g_object_unref (view->priv->search_widget);
      view->priv->search_widget = NULL;
    }

  /* connect handlers if new search is not null */
  if (search != NULL)
    {
      view->priv->search_widget = g_object_ref (search);

      g_signal_connect (view, "start-interactive-search",
          G_CALLBACK (individual_view_start_search_cb), NULL);

      g_signal_connect (view->priv->search_widget, "notify::text",
          G_CALLBACK (individual_view_search_text_notify_cb), view);
      g_signal_connect (view->priv->search_widget, "activate",
          G_CALLBACK (individual_view_search_activate_cb), view);
      g_signal_connect (view->priv->search_widget, "key-navigation",
          G_CALLBACK (individual_view_search_key_navigation_cb), view);
      g_signal_connect (view->priv->search_widget, "hide",
          G_CALLBACK (individual_view_search_hide_cb), view);
      g_signal_connect (view->priv->search_widget, "show",
          G_CALLBACK (individual_view_search_show_cb), view);
    }
}

gboolean
games_individual_view_is_searching (GamesIndividualView *self)
{
  g_return_val_if_fail (GAMES_IS_INDIVIDUAL_VIEW (self), FALSE);

  return (self->priv->search_widget != NULL &&
          gtk_widget_get_visible (self->priv->search_widget));
}

gboolean
games_individual_view_get_show_untrusted (GamesIndividualView *self)
{
  g_return_val_if_fail (GAMES_IS_INDIVIDUAL_VIEW (self), FALSE);

  return self->priv->show_untrusted;
}

void
games_individual_view_set_show_untrusted (GamesIndividualView *self,
    gboolean show_untrusted)
{
  g_return_if_fail (GAMES_IS_INDIVIDUAL_VIEW (self));

  self->priv->show_untrusted = show_untrusted;

  g_object_notify (G_OBJECT (self), "show-untrusted");
  gtk_tree_model_filter_refilter (self->priv->filter);
}

GamesIndividualStore *
games_individual_view_get_store (GamesIndividualView *self)
{
  g_return_val_if_fail (GAMES_IS_INDIVIDUAL_VIEW (self), NULL);

  return self->priv->store;
}

void
games_individual_view_set_store (GamesIndividualView *self,
    GamesIndividualStore *store)
{
  g_return_if_fail (GAMES_IS_INDIVIDUAL_VIEW (self));
  g_return_if_fail (store == NULL || GAMES_IS_INDIVIDUAL_STORE (store));

  /* Destroy the old filter and remove the old store */
  if (self->priv->store != NULL)
    {
      g_signal_handlers_disconnect_by_func (self->priv->filter,
          individual_view_row_has_child_toggled_cb, self);

      gtk_tree_view_set_model (GTK_TREE_VIEW (self), NULL);
    }

  tp_clear_object (&self->priv->filter);
  tp_clear_object (&self->priv->store);

  /* Set the new store */
  self->priv->store = store;

  if (store != NULL)
    {
      g_object_ref (store);

      /* Create a new filter */
      self->priv->filter = GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new (
          GTK_TREE_MODEL (self->priv->store), NULL));
      gtk_tree_model_filter_set_visible_func (self->priv->filter,
          individual_view_filter_visible_func, self, NULL);

      g_signal_connect (self->priv->filter, "row-has-child-toggled",
          G_CALLBACK (individual_view_row_has_child_toggled_cb), self);
      gtk_tree_view_set_model (GTK_TREE_VIEW (self),
          GTK_TREE_MODEL (self->priv->filter));
    }
}

void
games_individual_view_start_search (GamesIndividualView *self)
{
  g_return_if_fail (GAMES_IS_INDIVIDUAL_VIEW (self));
  g_return_if_fail (self->priv->search_widget != NULL);

  if (gtk_widget_get_visible (GTK_WIDGET (self->priv->search_widget)))
    gtk_widget_grab_focus (GTK_WIDGET (self->priv->search_widget));
  else
    gtk_widget_show (GTK_WIDGET (self->priv->search_widget));
}

void
games_individual_view_set_custom_filter (GamesIndividualView *self,
    GtkTreeModelFilterVisibleFunc filter,
    gpointer data)
{
  self->priv->custom_filter = filter;
  self->priv->custom_filter_data = data;
}

void
games_individual_view_refilter (GamesIndividualView *self)
{
  gtk_tree_model_filter_refilter (self->priv->filter);
}

void
games_individual_view_select_first (GamesIndividualView *self)
{
  GtkTreeIter iter;

  gtk_tree_model_filter_refilter (self->priv->filter);

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (self->priv->filter), &iter))
    {
      GtkTreeSelection *selection = gtk_tree_view_get_selection (
          GTK_TREE_VIEW (self));

      gtk_tree_selection_select_iter (selection, &iter);
    }
}

void
games_individual_view_set_show_uninteresting (GamesIndividualView *self,
    gboolean show_uninteresting)
{
  g_return_if_fail (GAMES_IS_INDIVIDUAL_VIEW (self));


  self->priv->show_uninteresting = show_uninteresting;

  g_object_notify (G_OBJECT (self), "show-uninteresting");
  gtk_tree_model_filter_refilter (self->priv->filter);
}
