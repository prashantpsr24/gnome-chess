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

#include <telepathy-glib/telepathy-glib.h>

#include "games-channel-request-utils.h"
#include "games-ui-utils.h"


void
games_chat_with_contact (GamesContact *contact,
    gint64 timestamp)
{
  games_chat_with_contact_id (
      games_contact_get_account (contact), games_contact_get_id (contact),
      timestamp, NULL, NULL);
}

static void
ensure_text_channel_cb (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
  GError *error = NULL;

  if (!tp_account_channel_request_ensure_channel_finish (
        TP_ACCOUNT_CHANNEL_REQUEST (source), result, &error))
    {
      g_debug ("Failed to ensure text channel: %s", error->message);
      g_error_free (error);
    }
}

static void
create_text_channel (TpAccount *account,
    TpHandleType target_handle_type,
    const gchar *target_id,
    gint64 timestamp,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
  GHashTable *request;
  TpAccountChannelRequest *req;

  request = tp_asv_new (
      TP_PROP_CHANNEL_CHANNEL_TYPE, G_TYPE_STRING,
        TP_IFACE_CHANNEL_TYPE_TEXT,
      TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, G_TYPE_UINT, target_handle_type,
      TP_PROP_CHANNEL_TARGET_ID, G_TYPE_STRING, target_id,
      NULL);

  req = tp_account_channel_request_new (account, request, timestamp);
  tp_account_channel_request_set_delegate_to_preferred_handler (req, TRUE);

  tp_account_channel_request_ensure_channel_async (req, GAMES_CHAT_BUS_NAME,
      NULL, callback ? callback : ensure_text_channel_cb, user_data);

  g_hash_table_unref (request);
  g_object_unref (req);
}

/* @callback is optional, but if it's provided, it should call the right
 * _finish() func that we call in ensure_text_channel_cb() */
void
games_chat_with_contact_id (TpAccount *account,
    const gchar *contact_id,
    gint64 timestamp,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
  create_text_channel (account, TP_HANDLE_TYPE_CONTACT,
      contact_id, timestamp, callback, user_data);
}

