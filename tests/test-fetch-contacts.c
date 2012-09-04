#include <config.h>

#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <telepathy-glib/telepathy-glib.h>

#include <libgames-contacts/games-individual-store-manager.h>
#include <libgames-contacts/games-individual-view.h>

static int filter_index;
static GtkWidget *b1, *b2;
static GPtrArray *filters;
static GamesIndividualManager *ind_mgr;
static GamesIndividualStore *store;
static GamesIndividualView *tree_view;
static GamesLiveSearch *search_widget;

static gint64 prepare_aggregator_timestamp;

void populate_contacts (GamesIndividualView *tree_view, gpointer filter)
{
  games_individual_view_set_custom_filter (tree_view,
    (GtkTreeModelFilterVisibleFunc) filter);

  games_individual_view_refilter (tree_view);
}

void
prev_clicked_cb (GtkButton *button, GamesIndividualView *tree_view)
{
  populate_contacts (tree_view, g_ptr_array_index (filters, --filter_index));
  if (filter_index == 0)
    gtk_widget_hide (GTK_WIDGET (b1));
  if (filter_index == filters->len-2)
    gtk_widget_show (GTK_WIDGET (b2));
}

void
next_clicked_cb (GtkButton *button, GamesIndividualView *tree_view)
{
  populate_contacts (tree_view, g_ptr_array_index (filters, ++filter_index));
  if (filter_index == 1)
    gtk_widget_show (GTK_WIDGET (b1));
  if (filter_index == filters->len-1)
    gtk_widget_hide (GTK_WIDGET (b2));
}

void
contacts_loaded_cb (GamesIndividualManager *ind_mgr,
    GtkTreeView *view)
{
  g_debug ("\n\nTime to load contacts: %lld milli-seconds.\n\n",
    (g_get_monotonic_time () - prepare_aggregator_timestamp)/(1000));
}

gboolean
all_players (GtkTreeModel *model,
    GtkTreeIter *iter,
    gpointer data)
{
  return individual_view_filter_default (tree_view, model, iter,
      GAMES_ACTION_CHAT);
}

gboolean
chess_players (GtkTreeModel *model,
    GtkTreeIter *iter,
    gpointer data)
{
  return individual_view_filter_default (tree_view, model, iter,
      GAMES_ACTION_PLAY_GLCHESS);
}

static void
window_closed (GtkWidget *window, gpointer data)
{
  g_object_unref (ind_mgr);
  g_ptr_array_free (filters, TRUE);
  g_object_unref (store);

  /* Widgets internal to window will automatically be destroyed */

  gtk_main_quit ();
}

int main (int argc, char **argv)
{
  GtkWidget *window;
  GtkWidget *scroller;
  GtkWidget *grid;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy", G_CALLBACK (window_closed), NULL);
  scroller = gtk_scrolled_window_new (NULL, NULL);
  grid = gtk_grid_new ();
  gtk_orientable_set_orientation (GTK_ORIENTABLE (grid),
      GTK_ORIENTATION_VERTICAL);
  filters = g_ptr_array_new ();

  /* This leads to calling games_individual_manager_init () where
   * aggregator is prepared. Record time. */
  ind_mgr = games_individual_manager_dup_singleton ();
  prepare_aggregator_timestamp = g_get_monotonic_time ();

  /* Check for readiness of the individual aggregator 
   * and display list whenever ready */
  if (!games_individual_manager_get_contacts_loaded (ind_mgr))
  {
    g_signal_connect (ind_mgr, "contacts-loaded",
        G_CALLBACK (contacts_loaded_cb), tree_view);
  }
  
  store = GAMES_INDIVIDUAL_STORE (games_individual_store_manager_new (ind_mgr));
  tree_view = games_individual_view_new (store,
      GAMES_INDIVIDUAL_VIEW_FEATURE_GROUPS_SAVE);
  g_object_set (G_OBJECT (tree_view), "expand", TRUE, NULL);
  g_object_set (G_OBJECT (grid), "expand", TRUE, NULL);
  gtk_window_set_default_size (GTK_WINDOW (window), 500, 600);

  /* Search bar */
  search_widget = GAMES_LIVE_SEARCH (games_live_search_new (
      GTK_WIDGET (tree_view)));
  games_individual_view_set_live_search (tree_view,
      GAMES_LIVE_SEARCH (search_widget));

  filter_index = 0;


  gtk_container_add (GTK_CONTAINER (window), grid);

  gtk_container_add (GTK_CONTAINER (scroller), GTK_WIDGET (tree_view));
  gtk_container_add (GTK_CONTAINER (grid), GTK_WIDGET (scroller));
  gtk_container_add (GTK_CONTAINER (grid), GTK_WIDGET (search_widget));

  b1 = gtk_button_new_with_label ("Previous");
  b2 = gtk_button_new_with_label ("Next");
  g_signal_connect (b1, "clicked", G_CALLBACK (prev_clicked_cb), tree_view);
  g_signal_connect (b2, "clicked", G_CALLBACK (next_clicked_cb), tree_view);
  gtk_container_add (GTK_CONTAINER (grid), b1);
  gtk_container_add (GTK_CONTAINER (grid), b2);

  g_ptr_array_add (filters, (GtkTreeModelFilterVisibleFunc) all_players);
  g_ptr_array_add (filters, (GtkTreeModelFilterVisibleFunc) chess_players);

  populate_contacts (tree_view, g_ptr_array_index (filters, filter_index));

  gtk_widget_show_all (GTK_WIDGET (window));

  if (filter_index == 0)
    gtk_widget_hide (GTK_WIDGET (b1));
  /*if (filter_index == filters->len-1)*/
  /*gtk_widget_hide (GTK_WIDGET (b2));*/


  gtk_main ();
  return 0;
}
