/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* games-ui-utils.c - This file contains generic UI specific code for use in
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
 * This file has been prepared from utilities found in Empathy
 * (https://live.gnome.org/Empathy)
 */

#include <config.h>

#include <X11/Xatom.h>
#include <gdk/gdkx.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gio/gio.h>

#include <libxml/uri.h>

#include <telepathy-glib/util.h>
#include <folks/folks.h>

#include "games-ui-utils.h"
#include "games-live-search.h"

/* Contact presence image names */

#define GAMES_IMAGE_OFFLINE             "user-offline"
/* user-invisible is not (yet?) in the naming spec but already implemented by
 * some theme */
#define GAMES_IMAGE_HIDDEN              "user-invisible"
#define GAMES_IMAGE_AVAILABLE           "user-available"
#define GAMES_IMAGE_BUSY                "user-busy"
#define GAMES_IMAGE_AWAY                "user-away"
#define GAMES_IMAGE_EXT_AWAY            "user-extended-away"
#define GAMES_IMAGE_IDLE                "user-idle"
#define GAMES_IMAGE_PENDING             "games-pending"


gboolean
games_xml_validate (xmlDoc      *doc,
    const gchar *dtd_filename)
{
  gchar *path;
  xmlChar *escaped;
  xmlValidCtxt  cvp;
  xmlDtd *dtd;
  gboolean ret;

  path = g_build_filename (PKGDATADIR, PACKAGE, dtd_filename, NULL);

  g_debug ("Loading dtd file %s", path);

  /* The list of valid chars is taken from libxml. */
  escaped = xmlURIEscapeStr ((const xmlChar *) path,
    (const xmlChar *)":@&=+$,/?;");
  g_free (path);

  memset (&cvp, 0, sizeof (cvp));
  dtd = xmlParseDTD (NULL, escaped);
  ret = xmlValidateDtd (&cvp, doc, dtd);

  xmlFree (escaped);
  xmlFreeDtd (dtd);

  return ret;
}

void
games_builder_connect (GtkBuilder  *gui,
			 gpointer     user_data,
			 const gchar *first_object,
			 ...)
{
	va_list      args;
	const gchar *name;
	const gchar *sig;
	GObject     *object;
	GCallback    callback;

	va_start (args, first_object);
	for (name = first_object; name; name = va_arg (args, const gchar *)) {
		sig = va_arg (args, const gchar *);
		callback = va_arg (args, GCallback);

		object = gtk_builder_get_object (gui, name);
		if (!object) {
			g_warning ("File is missing object '%s'.", name);
			continue;
		}

		g_signal_connect (object, sig, callback, user_data);
	}

	va_end (args);
}

GtkWidget *
games_builder_unref_and_keep_widget (GtkBuilder *gui,
				       GtkWidget  *widget)
{
	/* On construction gui sinks the initial reference to widget. When gui
	 * is finalized it will drop its ref to widget. We take our own ref to
	 * prevent widget being finalised. The widget is forced to have a
	 * floating reference, like when it was initially unowned so that it can
	 * be used like any other GtkWidget. */

	g_object_ref (widget);
	g_object_force_floating (G_OBJECT (widget));
	g_object_unref (gui);

	return widget;
}

const gchar *
games_icon_name_for_presence (TpConnectionPresenceType presence)
{
	switch (presence) {
	case TP_CONNECTION_PRESENCE_TYPE_AVAILABLE:
		return GAMES_IMAGE_AVAILABLE;
	case TP_CONNECTION_PRESENCE_TYPE_BUSY:
		return GAMES_IMAGE_BUSY;
	case TP_CONNECTION_PRESENCE_TYPE_AWAY:
		return GAMES_IMAGE_AWAY;
	case TP_CONNECTION_PRESENCE_TYPE_EXTENDED_AWAY:
		if (gtk_icon_theme_has_icon (gtk_icon_theme_get_default (),
					     GAMES_IMAGE_EXT_AWAY))
			return GAMES_IMAGE_EXT_AWAY;

		/* The 'extended-away' icon is not an official one so we fallback to idle if
		 * it's not implemented */
		return GAMES_IMAGE_IDLE;
	case TP_CONNECTION_PRESENCE_TYPE_HIDDEN:
		if (gtk_icon_theme_has_icon (gtk_icon_theme_get_default (),
					     GAMES_IMAGE_HIDDEN))
			return GAMES_IMAGE_HIDDEN;

		/* The 'hidden' icon is not an official one so we fallback to offline if
		 * it's not implemented */
		return GAMES_IMAGE_OFFLINE;
	case TP_CONNECTION_PRESENCE_TYPE_OFFLINE:
	case TP_CONNECTION_PRESENCE_TYPE_ERROR:
		return GAMES_IMAGE_OFFLINE;
	case TP_CONNECTION_PRESENCE_TYPE_UNKNOWN:
		return GAMES_IMAGE_PENDING;
	case TP_CONNECTION_PRESENCE_TYPE_UNSET:
	default:
		return NULL;
	}

	return NULL;
}

const gchar *
games_icon_name_for_contact (GamesContact *contact)
{
	TpConnectionPresenceType presence;

	g_return_val_if_fail (GAMES_IS_CONTACT (contact),
			      GAMES_IMAGE_OFFLINE);

	presence = games_contact_get_presence (contact);
	return games_icon_name_for_presence (presence);
}

/* Translate Folks' general presence type to the Tp presence type */
TpConnectionPresenceType
games_folks_presence_type_to_tp (FolksPresenceType type)
{
  return (TpConnectionPresenceType) type;
}

const gchar *
games_icon_name_for_individual (FolksIndividual *individual)
{
	FolksPresenceType folks_presence;
	TpConnectionPresenceType presence;

	folks_presence =
	    folks_presence_details_get_presence_type (
	        FOLKS_PRESENCE_DETAILS (individual));
	presence = games_folks_presence_type_to_tp (folks_presence);

	return games_icon_name_for_presence (presence);
}

const gchar *
games_protocol_name_for_contact (GamesContact   *contact)
{
	TpAccount     *account;

	g_return_val_if_fail (GAMES_IS_CONTACT (contact), NULL);

	account = games_contact_get_account (contact);
	if (account == NULL) {
		return NULL;
	}

	return tp_account_get_icon_name (account);
}

GdkPixbuf *
games_pixbuf_from_data (gchar *data,
			  gsize  data_size)
{
	return games_pixbuf_from_data_and_mime (data, data_size, NULL);
}

GdkPixbuf *
games_pixbuf_from_data_and_mime (gchar  *data,
				   gsize   data_size,
				   gchar **mime_type)
{
	GdkPixbufLoader *loader;
	GdkPixbufFormat *format;
	GdkPixbuf       *pixbuf = NULL;
	gchar          **mime_types;
	GError          *error = NULL;

	if (!data) {
		return NULL;
	}

	loader = gdk_pixbuf_loader_new ();
	if (!gdk_pixbuf_loader_write (loader, (guchar *) data, data_size, &error)) {
		g_debug ("Failed to write to pixbuf loader: %s",
			error ? error->message : "No error given");
		goto out;
	}
	if (!gdk_pixbuf_loader_close (loader, &error)) {
		g_debug ("Failed to close pixbuf loader: %s",
			error ? error->message : "No error given");
		goto out;
	}

	pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
	if (pixbuf) {
		g_object_ref (pixbuf);

		if (mime_type != NULL) {
			format = gdk_pixbuf_loader_get_format (loader);
			mime_types = gdk_pixbuf_format_get_mime_types (format);

			*mime_type = g_strdup (*mime_types);
			if (mime_types[1] != NULL) {
				g_debug ("Loader supports more than one mime "
					"type! Picking the first one, %s",
					*mime_type);
			}
			g_strfreev (mime_types);
		}
	}

out:
	g_clear_error (&error);
	g_object_unref (loader);

	return pixbuf;
}

struct SizeData {
	gint     width;
	gint     height;
	gboolean preserve_aspect_ratio;
};

static void
pixbuf_from_avatar_size_prepared_cb (GdkPixbufLoader *loader,
				     int              width,
				     int              height,
				     struct SizeData *data)
{
	g_return_if_fail (width > 0 && height > 0);

	if (data->preserve_aspect_ratio && (data->width > 0 || data->height > 0)) {
		if (width < data->width && height < data->height) {
			width = width;
			height = height;
		}

		if (data->width < 0) {
			width = width * (double) data->height / (gdouble) height;
			height = data->height;
		} else if (data->height < 0) {
			height = height * (double) data->width / (double) width;
			width = data->width;
		} else if ((double) height * (double) data->width >
			   (double) width * (double) data->height) {
			width = 0.5 + (double) width * (double) data->height / (double) height;
			height = data->height;
		} else {
			height = 0.5 + (double) height * (double) data->width / (double) width;
			width = data->width;
		}
	} else {
		if (data->width > 0) {
			width = data->width;
		}

		if (data->height > 0) {
			height = data->height;
		}
	}

	gdk_pixbuf_loader_set_size (loader, width, height);
}

static void
games_avatar_pixbuf_roundify (GdkPixbuf *pixbuf)
{
	gint width, height, rowstride;
	guchar *pixels;

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	pixels = gdk_pixbuf_get_pixels (pixbuf);

	if (width < 6 || height < 6) {
		return;
	}

	/* Top left */
	pixels[3] = 0;
	pixels[7] = 0x80;
	pixels[11] = 0xC0;
	pixels[rowstride + 3] = 0x80;
	pixels[rowstride * 2 + 3] = 0xC0;

	/* Top right */
	pixels[width * 4 - 1] = 0;
	pixels[width * 4 - 5] = 0x80;
	pixels[width * 4 - 9] = 0xC0;
	pixels[rowstride + (width * 4) - 1] = 0x80;
	pixels[(2 * rowstride) + (width * 4) - 1] = 0xC0;

	/* Bottom left */
	pixels[(height - 1) * rowstride + 3] = 0;
	pixels[(height - 1) * rowstride + 7] = 0x80;
	pixels[(height - 1) * rowstride + 11] = 0xC0;
	pixels[(height - 2) * rowstride + 3] = 0x80;
	pixels[(height - 3) * rowstride + 3] = 0xC0;

	/* Bottom right */
	pixels[height * rowstride - 1] = 0;
	pixels[(height - 1) * rowstride - 1] = 0x80;
	pixels[(height - 2) * rowstride - 1] = 0xC0;
	pixels[height * rowstride - 5] = 0x80;
	pixels[height * rowstride - 9] = 0xC0;
}

static gboolean
games_gdk_pixbuf_is_opaque (GdkPixbuf *pixbuf)
{
	gint height, rowstride, i;
	guchar *pixels;
	guchar *row;

	height = gdk_pixbuf_get_height (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	pixels = gdk_pixbuf_get_pixels (pixbuf);

	row = pixels;
	for (i = 3; i < rowstride; i+=4) {
		if (row[i] < 0xfe) {
			return FALSE;
		}
	}

	for (i = 1; i < height - 1; i++) {
		row = pixels + (i*rowstride);
		if (row[3] < 0xfe || row[rowstride-1] < 0xfe) {
			return FALSE;
		}
	}

	row = pixels + ((height-1) * rowstride);
	for (i = 3; i < rowstride; i+=4) {
		if (row[i] < 0xfe) {
			return FALSE;
		}
	}

	return TRUE;
}

static GdkPixbuf *
avatar_pixbuf_from_loader (GdkPixbufLoader *loader)
{
	GdkPixbuf *pixbuf;

	pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
	if (!gdk_pixbuf_get_has_alpha (pixbuf)) {
		GdkPixbuf *rounded_pixbuf;

		rounded_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
						 gdk_pixbuf_get_width (pixbuf),
						 gdk_pixbuf_get_height (pixbuf));
		gdk_pixbuf_copy_area (pixbuf, 0, 0,
				      gdk_pixbuf_get_width (pixbuf),
				      gdk_pixbuf_get_height (pixbuf),
				      rounded_pixbuf,
				      0, 0);
		pixbuf = rounded_pixbuf;
	} else {
		g_object_ref (pixbuf);
	}

	if (games_gdk_pixbuf_is_opaque (pixbuf)) {
		games_avatar_pixbuf_roundify (pixbuf);
	}

	return pixbuf;
}

GdkPixbuf *
games_pixbuf_from_avatar_scaled (GamesAvatar *avatar,
				  gint          width,
				  gint          height)
{
	GdkPixbuf        *pixbuf;
	GdkPixbufLoader	 *loader;
	struct SizeData   data;
	GError           *error = NULL;

	if (!avatar) {
		return NULL;
	}

	data.width = width;
	data.height = height;
	data.preserve_aspect_ratio = TRUE;

	loader = gdk_pixbuf_loader_new ();

	g_signal_connect (loader, "size-prepared",
			  G_CALLBACK (pixbuf_from_avatar_size_prepared_cb),
			  &data);

	if (avatar->len == 0) {
		g_warning ("Avatar has 0 length");
		return NULL;
	} else if (!gdk_pixbuf_loader_write (loader, avatar->data, avatar->len, &error)) {
		g_warning ("Couldn't write avatar image:%p with "
			   "length:%" G_GSIZE_FORMAT " to pixbuf loader: %s",
			   avatar->data, avatar->len, error->message);
		g_error_free (error);
		return NULL;
	}

	gdk_pixbuf_loader_close (loader, NULL);
	pixbuf = avatar_pixbuf_from_loader (loader);

	g_object_unref (loader);

	return pixbuf;
}

GdkPixbuf *
games_pixbuf_avatar_from_contact_scaled (GamesContact *contact,
					  gint           width,
					  gint           height)
{
	GamesAvatar *avatar;

	g_return_val_if_fail (GAMES_IS_CONTACT (contact), NULL);

	avatar = games_contact_get_avatar (contact);

	return games_pixbuf_from_avatar_scaled (avatar, width, height);
}

typedef struct {
	FolksIndividual *individual;
	GSimpleAsyncResult *result;
	guint width;
	guint height;
	struct SizeData size_data;
	GdkPixbufLoader *loader;
	GCancellable *cancellable;
	guint8 data[512];
} PixbufAvatarFromIndividualClosure;

static PixbufAvatarFromIndividualClosure *
pixbuf_avatar_from_individual_closure_new (FolksIndividual    *individual,
					   GSimpleAsyncResult *result,
					   gint                width,
					   gint                height,
					   GCancellable       *cancellable)
{
	PixbufAvatarFromIndividualClosure *closure;

	g_return_val_if_fail (FOLKS_IS_INDIVIDUAL (individual), NULL);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (result), NULL);

	closure = g_new0 (PixbufAvatarFromIndividualClosure, 1);
	closure->individual = g_object_ref (individual);
	closure->result = g_object_ref (result);
	closure->width = width;
	closure->height = height;
	if (cancellable != NULL)
		closure->cancellable = g_object_ref (cancellable);

	return closure;
}

static void
pixbuf_avatar_from_individual_closure_free (
		PixbufAvatarFromIndividualClosure *closure)
{
	g_clear_object (&closure->cancellable);
	tp_clear_object (&closure->loader);
	g_object_unref (closure->individual);
	g_object_unref (closure->result);
	g_free (closure);
}

static void
avatar_icon_load_close_cb (GObject      *object,
                           GAsyncResult *result,
                           gpointer      user_data)
{
	GError *error = NULL;

	g_input_stream_close_finish (G_INPUT_STREAM (object), result, &error);

	if (error != NULL) {
		g_debug ("Failed to close pixbuf stream: %s", error->message);
		g_error_free (error);
	}
}

static void
avatar_icon_load_read_cb (GObject      *object,
                          GAsyncResult *result,
                          gpointer      user_data)
{
	GInputStream *stream = G_INPUT_STREAM (object);
	PixbufAvatarFromIndividualClosure *closure = user_data;
	gssize n_read;
	GError *error = NULL;

	/* Finish reading this chunk from the stream */
	n_read = g_input_stream_read_finish (stream, result, &error);
	if (error != NULL) {
		g_debug ("Failed to finish read from pixbuf stream: %s",
			error->message);
		g_simple_async_result_set_from_error (closure->result, error);
		goto out_close;
	}

	/* Write the chunk to the pixbuf loader */
	if (!gdk_pixbuf_loader_write (closure->loader, (guchar *) closure->data,
			n_read, &error)) {
		g_debug ("Failed to write to pixbuf loader: %s",
			error ? error->message : "No error given");
		g_simple_async_result_set_from_error (closure->result, error);
		goto out_close;
	}

	if (n_read == 0) {
		/* EOF? */
		if (!gdk_pixbuf_loader_close (closure->loader, &error)) {
			g_debug ("Failed to close pixbuf loader: %s",
				error ? error->message : "No error given");
			g_simple_async_result_set_from_error (closure->result, error);
			goto out;
		}

		/* We're done. */
		g_simple_async_result_set_op_res_gpointer (closure->result,
			avatar_pixbuf_from_loader (closure->loader),
			g_object_unref);

		goto out;
	} else {
		/* Loop round and read another chunk. */
		g_input_stream_read_async (stream, closure->data,
			G_N_ELEMENTS (closure->data),
			G_PRIORITY_DEFAULT, closure->cancellable,
			avatar_icon_load_read_cb, closure);

		return;
	}

out_close:
	/* We must close the pixbuf loader before unreffing it. */
	gdk_pixbuf_loader_close (closure->loader, NULL);

out:
	/* Close the file for safety (even though it should be
	 * automatically closed when the stream is finalised). */
	g_input_stream_close_async (stream, G_PRIORITY_DEFAULT, NULL,
		(GAsyncReadyCallback) avatar_icon_load_close_cb, NULL);

	g_simple_async_result_complete (closure->result);

	g_clear_error (&error);
	pixbuf_avatar_from_individual_closure_free (closure);
}

static void
avatar_icon_load_cb (GObject      *object,
                     GAsyncResult *result,
                     gpointer      user_data)
{
	GLoadableIcon *icon = G_LOADABLE_ICON (object);
	PixbufAvatarFromIndividualClosure *closure = user_data;
	GInputStream *stream;
	GError *error = NULL;

	stream = g_loadable_icon_load_finish (icon, result, NULL, &error);
	if (error != NULL) {
		g_debug ("Failed to open avatar stream: %s", error->message);
		g_simple_async_result_set_from_error (closure->result, error);
		goto out;
	}

	closure->size_data.width = closure->width;
	closure->size_data.height = closure->height;
	closure->size_data.preserve_aspect_ratio = TRUE;

	/* Load the data into a pixbuf loader in chunks. */
	closure->loader = gdk_pixbuf_loader_new ();

	g_signal_connect (closure->loader, "size-prepared",
			  G_CALLBACK (pixbuf_from_avatar_size_prepared_cb),
	                  &(closure->size_data));

	/* Begin to read the first chunk. */
	g_input_stream_read_async (stream, closure->data,
			G_N_ELEMENTS (closure->data),
			G_PRIORITY_DEFAULT, closure->cancellable,
			avatar_icon_load_read_cb, closure);

	g_object_unref (stream);

	return;

out:
	g_simple_async_result_complete (closure->result);

	g_clear_error (&error);
	tp_clear_object (&stream);
	pixbuf_avatar_from_individual_closure_free (closure);
}

void
games_pixbuf_avatar_from_individual_scaled_async (
		FolksIndividual     *individual,
		gint                 width,
		gint                 height,
		GCancellable        *cancellable,
		GAsyncReadyCallback  callback,
		gpointer             user_data)
{
	GLoadableIcon *avatar_icon;
	GSimpleAsyncResult *result;
	PixbufAvatarFromIndividualClosure *closure;

	result = g_simple_async_result_new (G_OBJECT (individual),
			callback, user_data,
			games_pixbuf_avatar_from_individual_scaled_async);

	avatar_icon =
		folks_avatar_details_get_avatar (FOLKS_AVATAR_DETAILS (individual));
	if (avatar_icon == NULL) {
		g_simple_async_result_set_error (result, G_IO_ERROR,
			G_IO_ERROR_NOT_FOUND, "no avatar found");

		g_simple_async_result_complete (result);
		g_object_unref (result);
		return;
	}

	closure = pixbuf_avatar_from_individual_closure_new (individual, result,
							     width, height,
							     cancellable);

	g_return_if_fail (closure != NULL);

	g_loadable_icon_load_async (avatar_icon, width, cancellable,
			avatar_icon_load_cb, closure);

	g_object_unref (result);
}

/* Return a ref on the GdkPixbuf */
GdkPixbuf *
games_pixbuf_avatar_from_individual_scaled_finish (
		FolksIndividual *individual,
		GAsyncResult *result,
		GError **error)
{
	GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (result);
	gboolean result_valid;
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (FOLKS_IS_INDIVIDUAL (individual), NULL);
	g_return_val_if_fail (G_IS_SIMPLE_ASYNC_RESULT (simple), NULL);

	if (g_simple_async_result_propagate_error (simple, error))
		return NULL;

	result_valid = g_simple_async_result_is_valid (result,
			G_OBJECT (individual),
			games_pixbuf_avatar_from_individual_scaled_async);
	g_return_val_if_fail (result_valid, NULL);

	pixbuf = g_simple_async_result_get_op_res_gpointer (simple);
	return pixbuf != NULL ? g_object_ref (pixbuf) : NULL;
}

GdkPixbuf *
games_pixbuf_contact_status_icon (GamesContact *contact,
				   gboolean       show_protocol)
{
	const gchar *icon_name;

	g_return_val_if_fail (GAMES_IS_CONTACT (contact), NULL);

	icon_name = games_icon_name_for_contact (contact);

	if (icon_name == NULL) {
		return NULL;
	}
	return games_pixbuf_contact_status_icon_with_icon_name (contact,
	    icon_name,
	    show_protocol);
}

GdkPixbuf *
games_pixbuf_contact_status_icon_with_icon_name (GamesContact *contact,
					  const gchar    *icon_name,
					  gboolean       show_protocol)
{
	GdkPixbuf *pix_status;
	GdkPixbuf *pix_protocol;
	gchar     *icon_filename;
	gint       height, width;
	gint       numerator, denominator;

	g_return_val_if_fail (GAMES_IS_CONTACT (contact) ||
			(show_protocol == FALSE), NULL);
	g_return_val_if_fail (icon_name != NULL, NULL);

	numerator = 3;
	denominator = 4;

	icon_filename = games_filename_from_icon_name (icon_name,
							 GTK_ICON_SIZE_MENU);
	if (icon_filename == NULL) {
		g_debug ("icon name: %s could not be found\n", icon_name);
		return NULL;
	}

	pix_status = gdk_pixbuf_new_from_file (icon_filename, NULL);

	if (pix_status == NULL) {
		g_debug ("Could not open icon %s\n", icon_filename);
		g_free (icon_filename);
		return NULL;
	}

	g_free (icon_filename);

	if (!show_protocol)
		return pix_status;

	height = gdk_pixbuf_get_height (pix_status);
	width = gdk_pixbuf_get_width (pix_status);

	pix_protocol = games_pixbuf_protocol_from_contact_scaled (contact,
								    width * numerator / denominator,
								    height * numerator / denominator);

	if (pix_protocol == NULL) {
		return pix_status;
	}
	gdk_pixbuf_composite (pix_protocol, pix_status,
	    0, height - height * numerator / denominator,
	    width * numerator / denominator, height * numerator / denominator,
	    0, height - height * numerator / denominator,
	    1, 1,
	    GDK_INTERP_BILINEAR, 255);

	g_object_unref (pix_protocol);

	return pix_status;
}

GdkPixbuf *
games_pixbuf_protocol_from_contact_scaled (GamesContact *contact,
					  gint           width,
					  gint           height)
{
	TpAccount *account;
	gchar     *filename;
	GdkPixbuf *pixbuf = NULL;

	g_return_val_if_fail (GAMES_IS_CONTACT (contact), NULL);

	account = games_contact_get_account (contact);
	filename = games_filename_from_icon_name (tp_account_get_icon_name (account),
						    GTK_ICON_SIZE_MENU);
	if (filename != NULL) {
		pixbuf = gdk_pixbuf_new_from_file_at_size (filename, width, height, NULL);
		g_free (filename);
	}

	return pixbuf;
}

GdkPixbuf *
games_pixbuf_scale_down_if_necessary (GdkPixbuf *pixbuf, gint max_size)
{
	gint      width, height;
	gdouble   factor;

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);

	if (width > 0 && (width > max_size || height > max_size)) {
		factor = (gdouble) max_size / MAX (width, height);

		width = width * factor;
		height = height * factor;

		return gdk_pixbuf_scale_simple (pixbuf,
						width, height,
						GDK_INTERP_HYPER);
	}

	return g_object_ref (pixbuf);
}

GdkPixbuf *
games_pixbuf_from_icon_name_sized (const gchar *icon_name,
				     gint size)
{
	GtkIconTheme *theme;
	GdkPixbuf *pixbuf;
	GError *error = NULL;

	if (!icon_name) {
		return NULL;
	}

	theme = gtk_icon_theme_get_default ();

	pixbuf = gtk_icon_theme_load_icon (theme,
					   icon_name,
					   size,
					   0,
					   &error);
	if (error) {
		g_debug ("Error loading icon: %s", error->message);
		g_clear_error (&error);
	}

	return pixbuf;
}

GdkPixbuf *
games_pixbuf_from_icon_name (const gchar *icon_name,
			       GtkIconSize  icon_size)
{
	gint  w, h;
	gint  size = 48;

	if (!icon_name) {
		return NULL;
	}

	if (gtk_icon_size_lookup (icon_size, &w, &h)) {
		size = (w + h) / 2;
	}

	return games_pixbuf_from_icon_name_sized (icon_name, size);
}

gchar *
games_filename_from_icon_name (const gchar *icon_name,
				 GtkIconSize  icon_size)
{
	GtkIconTheme *icon_theme;
	GtkIconInfo  *icon_info;
	gint          w, h;
	gint          size = 48;
	gchar        *ret;

	icon_theme = gtk_icon_theme_get_default ();

	if (gtk_icon_size_lookup (icon_size, &w, &h)) {
		size = (w + h) / 2;
	}

	icon_info = gtk_icon_theme_lookup_icon (icon_theme, icon_name, size, 0);
	if (icon_info == NULL)
		return NULL;

	ret = g_strdup (gtk_icon_info_get_filename (icon_info));
	gtk_icon_info_free (icon_info);

	return ret;
}

gint64
games_get_current_action_time (void)
{
  return (tp_user_action_time_from_x11 (gtk_get_current_event_time ()));
}

void
games_launch_program (const gchar *dir,
    const gchar *name,
    const gchar *args)
{
  GdkDisplay *display;
  GError *error = NULL;
  gchar *path, *cmd;
  GAppInfo *app_info;
  GdkAppLaunchContext *context = NULL;

  path = g_build_filename (dir, name, NULL);

  if (args != NULL)
    cmd = g_strconcat (path, " ", args, NULL);
  else
    cmd = g_strdup (path);

  app_info = g_app_info_create_from_commandline (cmd, NULL, 0, &error);
  if (app_info == NULL)
    {
      g_debug ("Failed to create app info: %s", error->message);
      g_error_free (error);
      goto out;
    }

  display = gdk_display_get_default ();
  context = gdk_display_get_app_launch_context (display);

  if (!g_app_info_launch (app_info, NULL, (GAppLaunchContext *) context,
      &error))
    {
      g_warning ("Failed to launch %s: %s", name, error->message);
      g_error_free (error);
      goto out;
    }

out:
  tp_clear_object (&app_info);
  tp_clear_object (&context);
  g_free (path);
  g_free (cmd);
}

/* @words = games_live_search_strip_utf8_string (@text);
 *
 * User has to pass both so we don't have to compute @words ourself each time
 * this function is called. */
gboolean
games_individual_match_string (FolksIndividual *individual,
    const char *text,
    GPtrArray *words)
{
  const gchar *str;
  GeeSet *personas;
  GeeIterator *iter;
  gboolean retval = FALSE;

  /* check alias name */
  str = folks_alias_details_get_alias (FOLKS_ALIAS_DETAILS (individual));

  if (games_live_search_match_words (str, words))
    return TRUE;

  personas = folks_individual_get_personas (individual);

  /* check contact id, remove the @server.com part */
  iter = gee_iterable_iterator (GEE_ITERABLE (personas));
  while (retval == FALSE && gee_iterator_next (iter))
    {
      FolksPersona *persona = gee_iterator_get (iter);
      const gchar *p;

      if (games_folks_persona_is_interesting (persona))
        {
          str = folks_persona_get_display_id (persona);

          /* Accept the persona if @text is a full prefix of his ID; that allows
           * user to find, say, a jabber contact by typing his JID. */
          if (g_str_has_prefix (str, text))
            {
              retval = TRUE;
            }
          else
            {
              gchar *dup_str = NULL;
              gboolean visible;

              p = strstr (str, "@");
              if (p != NULL)
                str = dup_str = g_strndup (str, p - str);

              visible = games_live_search_match_words (str, words);
              g_free (dup_str);
              if (visible)
                retval = TRUE;
            }
        }
      g_clear_object (&persona);
    }
  g_clear_object (&iter);

  /* FIXME: Add more rules here, we could check phone numbers in
   * contact's vCard for example. */
  return retval;
}


/* Most of the workspace manipulation code has been copied from libwnck
 * Copyright (C) 2001 Havoc Pennington
 * Copyright (C) 2005-2007 Vincent Untz
 */
static void
_wnck_activate_workspace (Screen *screen,
    int new_active_space,
    Time timestamp)
{
  Display *display;
  Window   root;
  XEvent   xev;

  display = DisplayOfScreen (screen);
  root = RootWindowOfScreen (screen);

  xev.xclient.type = ClientMessage;
  xev.xclient.serial = 0;
  xev.xclient.send_event = True;
  xev.xclient.display = display;
  xev.xclient.window = root;
  xev.xclient.message_type = gdk_x11_get_xatom_by_name ("_NET_CURRENT_DESKTOP");
  xev.xclient.format = 32;
  xev.xclient.data.l[0] = new_active_space;
  xev.xclient.data.l[1] = timestamp;
  xev.xclient.data.l[2] = 0;
  xev.xclient.data.l[3] = 0;
  xev.xclient.data.l[4] = 0;

  gdk_error_trap_push ();
  XSendEvent (display, root, False,
      SubstructureRedirectMask | SubstructureNotifyMask,
      &xev);
  XSync (display, False);
  gdk_error_trap_pop_ignored ();
}

static gboolean
_wnck_get_cardinal (Screen *screen,
    Window xwindow,
    Atom atom,
    int *val)
{
  Display *display;
  Atom type;
  int format;
  gulong nitems;
  gulong bytes_after;
  gulong *num;
  int err, result;

  display = DisplayOfScreen (screen);

  *val = 0;

  gdk_error_trap_push ();
  type = None;
  result = XGetWindowProperty (display, xwindow, atom,
      0, G_MAXLONG, False, XA_CARDINAL, &type, &format, &nitems,
      &bytes_after, (void *) &num);
  err = gdk_error_trap_pop ();
  if (err != Success ||
      result != Success)
    return FALSE;

  if (type != XA_CARDINAL)
    {
      XFree (num);
      return FALSE;
    }

  *val = *num;

  XFree (num);

  return TRUE;
}

static int
window_get_workspace (Screen *xscreen,
    Window win)
{
  int number;

  if (!_wnck_get_cardinal (xscreen, win,
        gdk_x11_get_xatom_by_name ("_NET_WM_DESKTOP"), &number))
    return -1;

  return number;
}

/* Ask X to move to the desktop on which @window currently is
 * and the present @window. */
void
games_move_to_window_desktop (GtkWindow *window,
    guint32 timestamp)
{
  GdkScreen *screen;
  Screen *xscreen;
  GdkWindow *gdk_window;
  int workspace;

  screen = gtk_window_get_screen (window);
  xscreen = gdk_x11_screen_get_xscreen (screen);
  gdk_window = gtk_widget_get_window (GTK_WIDGET (window));

  workspace = window_get_workspace (xscreen,
      gdk_x11_window_get_xid (gdk_window));
  if (workspace == -1)
    goto out;

  _wnck_activate_workspace (xscreen, workspace, timestamp);

out:
  gtk_window_present_with_time (window, timestamp);
}

