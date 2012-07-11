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

/*
 * This code has been influenced by contacts in Empathy, a chat 
 * application based on Telepathy (https://live.gnome.org/Empathy)
 * Copyright (C) 2004-2007 Imendio AB
 */

#ifndef __GAMES_CELL_RENDERER_TEXT_H__
#define __GAMES_CELL_RENDERER_TEXT_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GAMES_TYPE_CELL_RENDERER_TEXT         (games_cell_renderer_text_get_type ())
#define GAMES_CELL_RENDERER_TEXT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GAMES_TYPE_CELL_RENDERER_TEXT, GamesCellRendererText))
#define GAMES_CELL_RENDERER_TEXT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GAMES_TYPE_CELL_RENDERER_TEXT, GamesCellRendererTextClass))
#define GAMES_IS_CELL_RENDERER_TEXT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GAMES_TYPE_CELL_RENDERER_TEXT))
#define GAMES_IS_CELL_RENDERER_TEXT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GAMES_TYPE_CELL_RENDERER_TEXT))
#define GAMES_CELL_RENDERER_TEXT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GAMES_TYPE_CELL_RENDERER_TEXT, GamesCellRendererTextClass))

typedef struct _GamesCellRendererText      GamesCellRendererText;
typedef struct _GamesCellRendererTextClass GamesCellRendererTextClass;
typedef struct _GamesCellRendererTextPriv GamesCellRendererTextPriv;

struct _GamesCellRendererText {
	GtkCellRendererText parent;
	GamesCellRendererTextPriv *priv;
};

struct _GamesCellRendererTextClass {
	GtkCellRendererTextClass    parent_class;
};

GType             games_cell_renderer_text_get_type (void) G_GNUC_CONST;
GtkCellRenderer * games_cell_renderer_text_new      (void);

G_END_DECLS

#endif /* __GAMES_CELL_RENDERER_TEXT_H__ */
