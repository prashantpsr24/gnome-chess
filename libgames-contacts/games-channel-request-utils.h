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


#ifndef __GAMES_CHANNEL_REQUEST_UTILS__
#define __GAMES_CHANNEL_REQUEST_UTILS__

#include <glib.h>
#include <gio/gio.h>

#include <telepathy-glib/channel.h>

#include "games-contact.h"

G_BEGIN_DECLS

#define GAMES_CHAT_BUS_NAME_SUFFIX "Games.Chat"
#define GAMES_CHAT_BUS_NAME TP_CLIENT_BUS_NAME_BASE GAMES_CHAT_BUS_NAME_SUFFIX

/* Requesting 1 to 1 text channels */
void games_chat_with_contact_id (TpAccount *account,
  const gchar *contact_id,
  gint64 timestamp,
  GAsyncReadyCallback callback,
  gpointer user_data);

void games_chat_with_contact (GamesContact *contact,
  gint64 timestamp);

G_END_DECLS

#endif /* __GAMES_CHANNEL_REQUEST_UTILS__ */
