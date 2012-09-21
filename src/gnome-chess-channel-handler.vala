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

public class HandlerApplication : Application
{
    private Gtk.Builder builder;

    /* Objects from gnome-chess-network-window.ui */
    private Gtk.Window? handler_window = null;
    private Gtk.Widget grid_main;

    private Gtk.Widget combobox_presence;
    private Gtk.Widget button_goa;
    private Gtk.Widget button_channels;
    private Gtk.Widget button_contacts;
    private Gtk.Container scrolledwindow_list;
    private Gtk.Widget alignment_play;

    private Gtk.Widget contacts_box;
    private GamesContacts.IndividualStore? individual_store = null;
    private GamesContacts.IndividualManager? individual_manager = null;
    private GamesContacts.IndividualView? individual_view = null;
    private GamesContacts.LiveSearch? search_widget;

    /* Objects from gnome-chess-game-window.ui */
    private Gtk.Widget view_box;
    private Gtk.Widget handler_menubar;

    public HandlerApplication ()
    {
        Object (
            application_id: TelepathyGLib.CLIENT_BUS_NAME_BASE + "Gnome.Chess",
            flags: ApplicationFlags.FLAGS_NONE);

        chess_networking_init ();

        settings = new Settings ("org.gnome.gnome-chess");
        settings_common = new Settings ("org.gnome.gnome-chess.games-common");

        builder = new Gtk.Builder ();
        try
        {
            builder.add_from_file (Path.build_filename (Config.PKGDATADIR,
                "gnome-chess-network-window.ui", null));
        }
        catch (Error e)
        {
            warning ("Could not load UI: %s", e.message);
        }

        settings_common.changed.connect (settings_changed_cb);

        create_handler_window ();
        /* Associate the window to this application */
        handler_window.application = this;

        if (settings.get_boolean ("network-window-fullscreen"))
            handler_window.fullscreen ();
        else if (settings.get_boolean ("network-window-maximized"))
            handler_window.maximize ();

        handler_window.resize (settings.get_int ("network-window-width"),
            settings.get_int ("network-window-height"));

        handler_window.show_all ();
    }

    private void create_handler_window ()
    {
        string [] objects_from_traditional_window =
          {
            "window_game_screen",
            "undo_move_image",
            "warning_image",
            "help_image"
          };
        handler_window = (Gtk.Window) builder.get_object
            ("window_networked_chess");
        handler_window.delete_event.connect (handler_window.hide_on_delete);

        grid_main = (Gtk.Widget) builder.get_object ("grid_main");
        combobox_presence = (Gtk.Widget) builder.get_object
            ("combobox_presence");
        button_goa = (Gtk.Widget) builder.get_object ("button_goa");
        button_channels = (Gtk.Widget) builder.get_object ("button_channels");
        button_contacts = (Gtk.Widget) builder.get_object ("button_contacts");
        scrolledwindow_list = (Gtk.Container) builder.get_object ("scrolledwindow_list");
        contacts_box = (Gtk.Widget) builder.get_object ("contacts_box");
        alignment_play = (Gtk.Widget) builder.get_object ("alignment_play");


        /* Add objects from gnome-chess-game-window.ui */
        try {
            builder.add_objects_from_file (Path.build_filename (Config.PKGDATADIR,
              "gnome-chess-game-window.ui", null),
              objects_from_traditional_window);

            view_box = (Gtk.Widget) builder.get_object ("view_box");
            handler_menubar = (Gtk.Widget) builder.get_object ("menubar");

            handler_menubar.unparent ();
            (grid_main as Gtk.Grid).attach (handler_menubar, 0, 0, 2, 1);

            view_box.reparent (alignment_play);

            grid_main.show_all ();
        }
        catch (Error e)
        {
            warning ("Could not load UI: %s", e.message);
        }

        /* Connect signals of all objects to their respective handlers */
        builder.connect_signals (this);

        handler_window.title = _("Chess");

        prepare_contact_list ();
    }

    private void prepare_contact_list ()
    {
        individual_manager =
            GamesContacts.IndividualManager.dup_singleton ();
        individual_store = new GamesContacts.IndividualStoreManager (
            individual_manager);
        individual_view = new GamesContacts.IndividualView (
            individual_store,
            GamesContacts.IndividualViewFeatureFlags.GROUPS_SAVE);

        /* Create searching capability for the treeview */
        search_widget = new GamesContacts.LiveSearch (
            individual_view as Gtk.Widget);
        individual_view.set_live_search (search_widget);

        /* Add filter for contacts capable of playing chess */
        individual_view.set_custom_filter ((model, iter, data)=>{
              return (data as GamesContacts.IndividualView).filter_default (
                  model, iter, GamesContacts.ActionType.PLAY_GLCHESS);
            });

        /* Evaluate contacts' presence using filter */
        individual_view.refilter ();

        /* Add to scrolled window and show */
        scrolledwindow_list.add (individual_view);
        scrolledwindow_list.show ();
        individual_view.show ();

        (contacts_box as Gtk.Container).add (search_widget);
    }

    [CCode (cname = "G_MODULE_EXPORT quit_cb", instance_pos = -1)]
    private new void quit_cb (Gtk.Widget widget)
    {
        quit_game ();
    }

    /* Quits the application */
    public new void quit_game ()
    {
        Settings.sync ();
    }

    public override void activate ()
    {
      /* Handler window has been created by constructor already. Show it. */
      handler_window.show_all ();
      handler_window.present ();
    }

    [CCode (cname = "G_MODULE_EXPORT network_window_configure_event_cb", instance_pos = -1)]
    public bool network_window_configure_event_cb (Gtk.Widget widget, Gdk.EventConfigure event)
    {
        if (!settings.get_boolean ("network-window-maximized") &&
            !settings.get_boolean ("network-window-fullscreen"))
        {
            settings.set_int ("network-window-width", event.width);
            settings.set_int ("network-window-height", event.height);
        }

        return false;
    }

    [CCode (cname = "G_MODULE_EXPORT network_window_window_state_event_cb", instance_pos = -1)]
    public bool network_window_window_state_event_cb (Gtk.Widget widget, Gdk.EventWindowState event)
    {
        if ((event.changed_mask & Gdk.WindowState.MAXIMIZED) != 0)
        {
            var is_maximized = (event.new_window_state & Gdk.WindowState.MAXIMIZED) != 0;
            settings.set_boolean ("network-window-maximized", is_maximized);
        }
        if ((event.changed_mask & Gdk.WindowState.FULLSCREEN) != 0)
        {
            bool is_fullscreen = (event.new_window_state & Gdk.WindowState.FULLSCREEN) != 0;
            settings.set_boolean ("network-window-fullscreen", is_fullscreen);
            fullscreen_menu.label = is_fullscreen ? Gtk.Stock.LEAVE_FULLSCREEN : Gtk.Stock.FULLSCREEN;
        }

        return false;
    }



    [CCode (cname = "G_MODULE_EXPORT new_game_cb", instance_pos = -1)]
    private new void new_game_cb (Gtk.Widget widget)
    {
    }

}

class GlChessChannelHandler
{
    private static int main (string[] args)
    {
        Intl.setlocale (LocaleCategory.ALL, "");
        Intl.bindtextdomain (Config.GETTEXT_PACKAGE, Config.LOCALEDIR);
        Intl.bind_textdomain_codeset (Config.GETTEXT_PACKAGE, "UTF-8");
        Intl.textdomain (Config.GETTEXT_PACKAGE);

        Gtk.init (ref args);

        Gtk.IconTheme icon_theme = Gtk.IconTheme.get_default ();
        icon_theme.append_search_path ("%s%s%s".printf
            (Config.PKGDATADIR, Path.DIR_SEPARATOR_S, "icons"));

        HandlerApplication app = new HandlerApplication ();
        var result = app.run ();

        return result;
    }
}
