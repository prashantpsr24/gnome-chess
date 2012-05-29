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

#ifndef __GAMES_INDIVIDUAL_STORE_MANAGER_H__
#define __GAMES_INDIVIDUAL_STORE_MANAGER_H__

#include <gtk/gtk.h>

#include "games-individual-manager.h"

#include "games-individual-store.h"

G_BEGIN_DECLS
#define GAMES_TYPE_INDIVIDUAL_STORE_MANAGER         (games_individual_store_manager_get_type ())
#define GAMES_INDIVIDUAL_STORE_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GAMES_TYPE_INDIVIDUAL_STORE_MANAGER, GamesIndividualStoreManager))
#define GAMES_INDIVIDUAL_STORE_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GAMES_TYPE_INDIVIDUAL_STORE_MANAGER, GamesIndividualStoreManagerClass))
#define GAMES_IS_INDIVIDUAL_STORE_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GAMES_TYPE_INDIVIDUAL_STORE_MANAGER))
#define GAMES_IS_INDIVIDUAL_STORE_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GAMES_TYPE_INDIVIDUAL_STORE_MANAGER))
#define GAMES_INDIVIDUAL_STORE_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GAMES_TYPE_INDIVIDUAL_STORE_MANAGER, GamesIndividualStoreManagerClass))

typedef struct _GamesIndividualStoreManager GamesIndividualStoreManager;
typedef struct _GamesIndividualStoreManagerClass GamesIndividualStoreManagerClass;
typedef struct _GamesIndividualStoreManagerPriv GamesIndividualStoreManagerPriv;

struct _GamesIndividualStoreManager
{
  GamesIndividualStore parent;
  GamesIndividualStoreManagerPriv *priv;
};

struct _GamesIndividualStoreManagerClass
{
  GamesIndividualStoreClass parent_class;
};

GType games_individual_store_manager_get_type (void) G_GNUC_CONST;

GamesIndividualStoreManager *games_individual_store_manager_new (
    GamesIndividualManager *manager);

GamesIndividualManager *games_individual_store_manager_get_manager (
    GamesIndividualStoreManager *store);

G_END_DECLS
#endif /* __GAMES_INDIVIDUAL_STORE_MANAGER_H__ */
