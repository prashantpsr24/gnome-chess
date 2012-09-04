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
 */

/* Much of code here is a simplified adaptation of the way contacts
 * are displayed in Empathy, a chat application using folks.
 * (https://live.gnome.org/Empathy)
 * Copyright (C) 2005-2007 Imendio AB
 * Copyright (C) 2007-2010 Collabora Ltd.
 */


#ifndef __GAMES_INDIVIDUAL_VIEW_H__
#define __GAMES_INDIVIDUAL_VIEW_H__

#include <gtk/gtk.h>

#include <folks/folks.h>

#include "games-enum-types.h"

#include "games-live-search.h"
#include "games-individual-store.h"

G_BEGIN_DECLS
#define GAMES_TYPE_INDIVIDUAL_VIEW         (games_individual_view_get_type ())
#define GAMES_INDIVIDUAL_VIEW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GAMES_TYPE_INDIVIDUAL_VIEW, GamesIndividualView))
#define GAMES_INDIVIDUAL_VIEW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GAMES_TYPE_INDIVIDUAL_VIEW, GamesIndividualViewClass))
#define GAMES_IS_INDIVIDUAL_VIEW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GAMES_TYPE_INDIVIDUAL_VIEW))
#define GAMES_IS_INDIVIDUAL_VIEW_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GAMES_TYPE_INDIVIDUAL_VIEW))
#define GAMES_INDIVIDUAL_VIEW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GAMES_TYPE_INDIVIDUAL_VIEW, GamesIndividualViewClass))
typedef struct _GamesIndividualView GamesIndividualView;
typedef struct _GamesIndividualViewClass GamesIndividualViewClass;
typedef struct _GamesIndividualViewPriv GamesIndividualViewPriv;

typedef enum
{
  GAMES_INDIVIDUAL_VIEW_FEATURE_NONE = 0,
  GAMES_INDIVIDUAL_VIEW_FEATURE_GROUPS_SAVE = 1 << 0,
} GamesIndividualViewFeatureFlags;

struct _GamesIndividualView
{
  GtkTreeView parent;
  GamesIndividualViewPriv *priv;
};

struct _GamesIndividualViewClass
{
  GtkTreeViewClass parent_class;
};

GType games_individual_view_get_type (void) G_GNUC_CONST;

GamesIndividualView *games_individual_view_new (
    GamesIndividualStore *store,
    GamesIndividualViewFeatureFlags view_features);

FolksIndividual *games_individual_view_dup_selected (
    GamesIndividualView *view);

GtkWidget *games_individual_view_get_individual_menu (
    GamesIndividualView *view);

GtkWidget *games_individual_view_get_group_menu (GamesIndividualView *view);

void games_individual_view_set_live_search (GamesIndividualView *view,
    GamesLiveSearch *search);

gboolean games_individual_view_get_show_offline (
    GamesIndividualView *view);

void games_individual_view_set_show_offline (
    GamesIndividualView *view,
    gboolean show_offline);

gboolean games_individual_view_get_show_untrusted (
    GamesIndividualView *self);

void games_individual_view_set_show_untrusted (GamesIndividualView *self,
    gboolean show_untrusted);

void games_individual_view_set_show_uninteresting (
    GamesIndividualView *view,
    gboolean show_uninteresting);

gboolean games_individual_view_is_searching (
    GamesIndividualView *view);

GamesIndividualStore *games_individual_view_get_store (
    GamesIndividualView *self);
void games_individual_view_set_store (GamesIndividualView *self,
    GamesIndividualStore *store);

void games_individual_view_start_search (GamesIndividualView *self);

gboolean individual_view_filter_default (GamesIndividualView *self,
    GtkTreeModel *model,
    GtkTreeIter *iter,
    GamesActionType interest);

void games_individual_view_set_custom_filter (GamesIndividualView *self,
    GtkTreeModelFilterVisibleFunc filter);

void games_individual_view_refilter (GamesIndividualView *self);

void games_individual_view_select_first (GamesIndividualView *self);

G_END_DECLS
#endif /* __GAMES_INDIVIDUAL_VIEW_H__ */
