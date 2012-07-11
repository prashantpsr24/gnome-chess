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
 * Copyright (C) 2006-2007 Imendio AB
 */

#ifndef __GAMES_CELL_RENDERER_EXPANDER_H__
#define __GAMES_CELL_RENDERER_EXPANDER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GAMES_TYPE_CELL_RENDERER_EXPANDER		(games_cell_renderer_expander_get_type ())
#define GAMES_CELL_RENDERER_EXPANDER(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GAMES_TYPE_CELL_RENDERER_EXPANDER, GamesCellRendererExpander))
#define GAMES_CELL_RENDERER_EXPANDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GAMES_TYPE_CELL_RENDERER_EXPANDER, GamesCellRendererExpanderClass))
#define GAMES_IS_CELL_RENDERER_EXPANDER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GAMES_TYPE_CELL_RENDERER_EXPANDER))
#define GAMES_IS_CELL_RENDERER_EXPANDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GAMES_TYPE_CELL_RENDERER_EXPANDER))
#define GAMES_CELL_RENDERER_EXPANDER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GAMES_TYPE_CELL_RENDERER_EXPANDER, GamesCellRendererExpanderClass))

typedef struct _GamesCellRendererExpander GamesCellRendererExpander;
typedef struct _GamesCellRendererExpanderClass GamesCellRendererExpanderClass;
typedef struct _GamesCellRendererExpanderPriv GamesCellRendererExpanderPriv;

struct _GamesCellRendererExpander {
  GtkCellRenderer parent;
  GamesCellRendererExpanderPriv *priv;
};

struct _GamesCellRendererExpanderClass {
  GtkCellRendererClass parent_class;

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};

GType            games_cell_renderer_expander_get_type (void) G_GNUC_CONST;
GtkCellRenderer *games_cell_renderer_expander_new      (void);

G_END_DECLS

#endif /* __GAMES_CELL_RENDERER_EXPANDER_H__ */
