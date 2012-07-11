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
 * Copyright (C) 2004-2007 Imendio ABdd
 * Copyright (C) 2010 Collabora Ltd.
 */


#include "config.h"

#include <string.h>

#include <libgames-contacts/games-ui-utils.h>
#include "games-cell-renderer-text.h"

struct _GamesCellRendererTextPriv{
	gchar    *name;
	TpConnectionPresenceType presence_type;
	gchar    *status;
	gboolean  is_group;

	gboolean  is_valid;
	gboolean  is_selected;

	gboolean  compact;
};

static void cell_renderer_text_dispose           (GObject                     *object);
static void cell_renderer_text_finalize          (GObject                     *object);
static void cell_renderer_text_get_property      (GObject                     *object,
						  guint                        param_id,
						  GValue                      *value,
						  GParamSpec                  *pspec);
static void cell_renderer_text_set_property      (GObject                     *object,
						  guint                        param_id,
						  const GValue                *value,
						  GParamSpec                  *pspec);
static void cell_renderer_text_render            (GtkCellRenderer             *cell,
						  cairo_t *cr,
						  GtkWidget                   *widget,
						  const GdkRectangle          *background_area,
						  const GdkRectangle          *cell_area,
						  GtkCellRendererState         flags);
static void cell_renderer_text_update_text       (GamesCellRendererText      *cell,
						  GtkWidget                   *widget,
						  gboolean                     selected);

/* Properties */
enum {
	PROP_0,
	PROP_NAME,
	PROP_PRESENCE_TYPE,
	PROP_STATUS,
	PROP_IS_GROUP,
	PROP_COMPACT,
};

G_DEFINE_TYPE (GamesCellRendererText, games_cell_renderer_text, GTK_TYPE_CELL_RENDERER_TEXT);

static void
cell_renderer_text_get_preferred_height_for_width (GtkCellRenderer *renderer,
								GtkWidget *widget,
								gint width,
								gint *minimum_size,
								gint *natural_size)
{
	GamesCellRendererText *self = GAMES_CELL_RENDERER_TEXT (renderer);

	/* Only update if not already valid so we get the right size. */
	cell_renderer_text_update_text (self, widget, self->priv->is_selected);

	GTK_CELL_RENDERER_CLASS (games_cell_renderer_text_parent_class)->
			get_preferred_height_for_width (renderer, widget, width,
					minimum_size, natural_size);
}


static void
games_cell_renderer_text_class_init (GamesCellRendererTextClass *klass)
{
	GObjectClass         *object_class;
	GtkCellRendererClass *cell_class;
	GParamSpec           *spec;

	object_class = G_OBJECT_CLASS (klass);
	cell_class = GTK_CELL_RENDERER_CLASS (klass);

	object_class->dispose = cell_renderer_text_dispose;
	object_class->finalize = cell_renderer_text_finalize;

	object_class->get_property = cell_renderer_text_get_property;
	object_class->set_property = cell_renderer_text_set_property;

	cell_class->get_preferred_height_for_width = cell_renderer_text_get_preferred_height_for_width;
	cell_class->render = cell_renderer_text_render;

	spec = g_param_spec_string ("name", "Name", "Contact name", NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (object_class, PROP_NAME, spec);

	spec = g_param_spec_uint ("presence-type", "TpConnectionPresenceType",
		"The contact's presence type",
		0, G_MAXUINT, /* Telepathy enum, can be extended */
		TP_CONNECTION_PRESENCE_TYPE_UNKNOWN,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (object_class, PROP_PRESENCE_TYPE,
		spec);

	spec = g_param_spec_string ("status", "Status message",
		"Contact's custom status message", NULL,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (object_class, PROP_STATUS, spec);

	spec = g_param_spec_boolean ("is-group", "Is group",
		"Whether this cell is a group", FALSE,
		G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (object_class, PROP_IS_GROUP, spec);

	spec = g_param_spec_boolean ("compact", "Compact",
		"TRUE to show the status alongside the contact name;"
		"FALSE to show it on its own line",
		FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (object_class, PROP_COMPACT, spec);

	g_type_class_add_private (object_class, sizeof (GamesCellRendererTextPriv));
}

static void
games_cell_renderer_text_init (GamesCellRendererText *cell)
{
	GamesCellRendererTextPriv *priv = G_TYPE_INSTANCE_GET_PRIVATE (cell,
		GAMES_TYPE_CELL_RENDERER_TEXT, GamesCellRendererTextPriv);

	cell->priv = priv;
	g_object_set (cell,
		      "ellipsize", PANGO_ELLIPSIZE_END,
		      NULL);

	priv->name = g_strdup ("");
	priv->status = g_strdup ("");
	priv->compact = FALSE;
}

static void
cell_renderer_text_dispose (GObject *object)
{
	GamesCellRendererText *cell = GAMES_CELL_RENDERER_TEXT (object);

	g_free (cell->priv->name);
	g_free (cell->priv->status);

	(G_OBJECT_CLASS (games_cell_renderer_text_parent_class)->dispose) (object);
}

static void
cell_renderer_text_finalize (GObject *object)
{
	(G_OBJECT_CLASS (games_cell_renderer_text_parent_class)->finalize) (object);
}

static void
cell_renderer_text_get_property (GObject    *object,
				 guint       param_id,
				 GValue     *value,
				 GParamSpec *pspec)
{
	GamesCellRendererText     *cell;

	cell = GAMES_CELL_RENDERER_TEXT (object);

	switch (param_id) {
	case PROP_NAME:
		g_value_set_string (value, cell->priv->name);
		break;
	case PROP_PRESENCE_TYPE:
		g_value_set_uint (value, cell->priv->presence_type);
		break;
	case PROP_STATUS:
		g_value_set_string (value, cell->priv->status);
		break;
	case PROP_IS_GROUP:
		g_value_set_boolean (value, cell->priv->is_group);
		break;
	case PROP_COMPACT:
		g_value_set_boolean (value, cell->priv->compact);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
cell_renderer_text_set_property (GObject      *object,
				 guint         param_id,
				 const GValue *value,
				 GParamSpec   *pspec)
{
	GamesCellRendererText     *cell;
	const gchar                *str;

	cell = GAMES_CELL_RENDERER_TEXT (object);

	switch (param_id) {
	case PROP_NAME:
		g_free (cell->priv->name);
		str = g_value_get_string (value);
		cell->priv->name = g_strdup (str ? str : "");
		g_strdelimit (cell->priv->name, "\n\r\t", ' ');
		cell->priv->is_valid = FALSE;
		break;
	case PROP_PRESENCE_TYPE:
		cell->priv->presence_type = g_value_get_uint (value);
		cell->priv->is_valid = FALSE;
		break;
	case PROP_STATUS:
		g_free (cell->priv->status);
		str = g_value_get_string (value);
		cell->priv->status = g_strdup (str ? str : "");
		g_strdelimit (cell->priv->status, "\n\r\t", ' ');
		cell->priv->is_valid = FALSE;
		break;
	case PROP_IS_GROUP:
		cell->priv->is_group = g_value_get_boolean (value);
		cell->priv->is_valid = FALSE;
		break;
	case PROP_COMPACT:
		cell->priv->compact = g_value_get_boolean (value);
		cell->priv->is_valid = FALSE;
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
cell_renderer_text_render (GtkCellRenderer      *cell,
			   cairo_t *cr,
			   GtkWidget            *widget,
			   const GdkRectangle   *background_area,
			   const GdkRectangle   *cell_area,
			   GtkCellRendererState  flags)
{
	GamesCellRendererText *celltext;

	celltext = GAMES_CELL_RENDERER_TEXT (cell);

	cell_renderer_text_update_text (celltext,
					widget,
					(flags & GTK_CELL_RENDERER_SELECTED));

	(GTK_CELL_RENDERER_CLASS (games_cell_renderer_text_parent_class)->render) (
		cell, cr,
		widget,
		background_area,
		cell_area,
		flags);
}

static void
cell_renderer_text_update_text (GamesCellRendererText *cell,
				GtkWidget              *widget,
				gboolean                selected)
{
	GamesCellRendererText *self = GAMES_CELL_RENDERER_TEXT (cell);
	const PangoFontDescription *font_desc;
	PangoAttrList              *attr_list;
	PangoAttribute             *attr_color = NULL, *attr_size;
	GtkStyleContext            *style;
	gchar                      *str;
	gint                        font_size;

	if (self->priv->is_valid && self->priv->is_selected == selected) {
		return;
	}

	if (self->priv->is_group) {
		g_object_set (cell,
			      "visible", TRUE,
			      "weight", PANGO_WEIGHT_BOLD,
			      "text", self->priv->name,
			      "attributes", NULL,
			      "xpad", 1,
			      "ypad", 1,
			      NULL);

		self->priv->is_selected = selected;
		self->priv->is_valid = TRUE;
		return;
	}

	style = gtk_widget_get_style_context (widget);

	attr_list = pango_attr_list_new ();

	font_desc = gtk_style_context_get_font (style, GTK_STATE_FLAG_NORMAL);
	font_size = pango_font_description_get_size (font_desc);
	attr_size = pango_attr_size_new (font_size / 1.2);
	attr_size->start_index = strlen (self->priv->name) + 1;
	attr_size->end_index = -1;
	pango_attr_list_insert (attr_list, attr_size);

	if (!selected) {
		GdkRGBA color;

		gtk_style_context_get_color (style, 0, &color);

		attr_color = pango_attr_foreground_new (color.red * 0xffff,
							color.green * 0xffff,
							color.blue * 0xffff);
		attr_color->start_index = attr_size->start_index;
		attr_color->end_index = -1;
		pango_attr_list_insert (attr_list, attr_color);
	}

	if (self->priv->compact) {
		if (STR_EMPTY (self->priv->status)) {
			str = g_strdup (self->priv->name);
		} else {
			str = g_strdup_printf ("%s %s", self->priv->name, self->priv->status);
		}
	} else {
		const gchar *status = self->priv->status;

		if (STR_EMPTY (self->priv->status)) {
			status = games_presence_get_default_message (self->priv->presence_type);
		}

		if (status == NULL)
			str = g_strdup (self->priv->name);
		else
			str = g_strdup_printf ("%s\n%s", self->priv->name, status);
	}

	g_object_set (cell,
		      "visible", TRUE,
		      "weight", PANGO_WEIGHT_NORMAL,
		      "text", str,
		      "attributes", attr_list,
		      "xpad", 0,
		      "ypad", 1,
		      NULL);

	g_free (str);
	pango_attr_list_unref (attr_list);

	self->priv->is_selected = selected;
	self->priv->is_valid = TRUE;
}

GtkCellRenderer *
games_cell_renderer_text_new (void)
{
	return g_object_new (GAMES_TYPE_CELL_RENDERER_TEXT, NULL);
}
