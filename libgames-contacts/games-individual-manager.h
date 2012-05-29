/* games-individual-manager.h: Manage Folks individuals 
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

#ifndef GAMES_INDIVIDUAL_MANAGER_H
#define GAMES_INDIVIDUAL_MANAGER_H

#include <glib.h>
#include <folks/folks.h>

G_BEGIN_DECLS

#define GAMES_TYPE_INDIVIDUAL_MANAGER            (games_individual_manager_get_type ())
#define GAMES_INDIVIDUAL_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GAMES_TYPE_INDIVIDUAL_MANAGER, GamesIndividualManager))
#define GAMES_INDIVIDUAL_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GAMES_TYPE_INDIVIDUAL_MANAGER, GamesIndividualManagerClass))
#define GAMES_IS_INDIVIDUAL_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GAMES_TYPE_INDIVIDUAL_MANAGER))
#define GAMES_IS_INDIVIDUAL_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GAMES_TYPE_INDIVIDUAL_MANAGER))
#define GAMES_INDIVIDUAL_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GAMES_TYPE_INDIVIDUAL_MANAGER))

typedef struct _GamesIndividualManager        GamesIndividualManager;
typedef struct _GamesIndividualManagerClass   GamesIndividualManagerClass;
typedef struct _GamesIndividualManagerPriv    GamesIndividualManagerPriv;

struct _GamesIndividualManager{
  GObject parent;

  /*< private >*/
  GamesIndividualManagerPriv *priv;
};

struct _GamesIndividualManagerClass{
  GObjectClass parent_class;
};

/* used by GAMES_TYPE_INDIVIDUAL_MANAGER */
GType                 games_individual_manager_get_type                      (void);

/* Function prototypes */
GamesIndividualManager  *games_individual_manager_dup_singleton              (void);
GList                *games_individual_manager_get_members                   (GamesIndividualManager *self);
FolksIndividual      *games_individual_manager_lookup_member                 (GamesIndividualManager *manager,
                                                                              const gchar            *id);

gboolean              games_individual_manager_get_contacts_loaded           (GamesIndividualManager *self);


G_END_DECLS
#endif /* GAMES_INDIVIDUAL_MANAGER_H */
/* EOF */
