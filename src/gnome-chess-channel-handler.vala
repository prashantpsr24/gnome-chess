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

    private Gtk.TreeView channel_view;

    /* Objects from gnome-chess-game-window.ui */
    private Gtk.Widget view_box;
    private Gtk.Widget handler_menubar;

    private bool is_activated = false;
    private TelepathyGLib.SimpleHandler tp_handler;

    private enum ChannelsColumn
    {
        USER_ACTION_TIME,  /* tp-time of channel creation */
        TARGET_CONTACT,    /* target contact */
        CHANNEL,           /* tube channel */
        OUR_COLOUR_WHITE,  /* our piece color */
        PGN_GAME,
        HISTORY_MODEL,
        HISTORY_COMBO_ACTIVE_ITER,
        CHESS_GAME,
        CHESS_SCENE,
        INFO_BAR,
        INFO_BAR_VISIBILITY,
        GAME_NEEDS_SAVING,
        NUM_COLS
    }

    /* Channels */
    Gtk.ListStore channels;
    Gtk.TreeIter? selected_channel_iter = null;

    public HandlerApplication ()
    {
        Object (
            application_id: "org.gnome.gnome-chess.handler",
            flags: ApplicationFlags.FLAGS_NONE);

        chess_networking_init ();
        channels = new Gtk.ListStore (ChannelsColumn.NUM_COLS,
            typeof (int64),                  /* USER_ACTION_TIME */
            typeof (TelepathyGLib.Contact),  /* TARGET_CONTACT */
            typeof (TelepathyGLib.Channel),  /* CHANNEL */
            typeof (bool),                   /* OUR_COLOUR_WHITE */
            typeof (PGNGame),                /* PGN_GAME */
            typeof (Gtk.TreeModel),          /* HISTORY_MODEL */
            typeof (Gtk.TreeIter),           /* HISTORY_COMBO_ACTIVE_ITER */
            typeof (ChessGame),              /* CHESS_GAME */
            typeof (ChessScene),             /* CHESS_SCENE */
            typeof (Gtk.InfoBar),            /* INFO_BAR */
            typeof (bool),                   /* INFO_BAR_VISIBILITY */
            typeof (bool)                    /* GAME_NEEDS_SAVING */
           // typeof (), /*  */
            );

        (channel_view).cursor_changed.connect (cursor_changed_cb);

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

    private bool change_view (Gtk.TreeModel model, Gtk.TreePath path, Gtk.TreeIter channel_iter)
    {
        ChessGame? game = null;
        ChessScene? scene = null;
        bool view_visible = true;

        channels.@get (channel_iter, ChannelsColumn.CHESS_GAME, out game, ChannelsColumn.CHESS_SCENE, out scene);
        assert (game != null && scene != null);
        ChessView chess_view = (game as Object).get_data<ChessView> ("chess-view");
        if (chess_view != null)
        {
            view_visible = chess_view.get_visible ();
            view_container.remove (chess_view);
            chess_view.destroy ();
        }
        if (settings_common.get_boolean ("show-3d"))
            chess_view = new ChessView3D ();
        else
            chess_view = new ChessView2D ();
        chess_view.set_size_request (300, 300);
        chess_view.scene = scene;
        view_container.add (chess_view);
        chess_view.set_visible (view_visible);

        return false;
    }

    private void settings_changed_cb (Settings settings_common, string key)
    {
        if (key == "show-3d")
        {
            channels.@foreach (change_view);
        }
    }

    private void show_game ()
    {
        TelepathyGLib.Contact? remote_player = null;
        int64 action_time = -1;

        if (selected_channel_iter == null)
        {
            debug ("No channel iteration selected");
            return;
        }
        channels.@get (selected_channel_iter, ChannelsColumn.TARGET_CONTACT, out remote_player, ChannelsColumn.USER_ACTION_TIME, out action_time);

        if (remote_player != null && action_time != -1)
        {
            debug ("Showing game with %s. Initiation timeframe: %llu", remote_player.identifier, action_time);
        }
    }

    private void cursor_changed_cb ()
    {
        Gtk.TreePath? path;
        Gtk.TreeIter iter;

        channel_view.get_cursor (out path, null);
        if (path == null)
        {
            debug ("Couldn't obtain path the cursor is at");
            return;
        }

        if (channels.get_iter (out iter, path))
        {
            selected_channel_iter = iter;

            show_game ();
        }
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

        channel_view = new Gtk.TreeView ();

        /* Add objects from gnome-chess-game-window.ui */
        try {
            builder.add_objects_from_file (Path.build_filename (Config.PKGDATADIR,
              "gnome-chess-game-window.ui", null),
              objects_from_traditional_window);

            view_box = (Gtk.Widget) builder.get_object ("view_box");
            view_container = (Gtk.Container) builder.get_object ("view_container");
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

        channel_view = new Gtk.TreeView ();
        channel_view.model = channels;
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

    [CCode (cname = "G_MODULE_EXPORT channels_clicked_cb", instance_pos=-1)]
    public void channels_clicked_cb (Gtk.Button button)
    {
        scrolledwindow_list.remove (contacts_box);
        scrolledwindow_list.add (channel_view);
    }

    [CCode (cname = "G_MODULE_EXPORT contacts_clicked_cb", instance_pos=-1)]
    public void contacts_clicked_cb (Gtk.Button button)
    {
        scrolledwindow_list.remove (channel_view);
        scrolledwindow_list.add (contacts_box);
    }

    [CCode (cname = "G_MODULE_EXPORT goa_clicked_cb", instance_pos=-1)]
    public void goa_clicked_cb (Gtk.Button button)
    {
    }

    [CCode (cname = "G_MODULE_EXPORT quit_cb", instance_pos = -1)]
    public override void quit_cb (Gtk.Widget widget)
    {
        quit_game ();
    }

    private bool save_game_and_remove_channel (Gtk.TreeModel model, Gtk.TreePath path, Gtk.TreeIter channel_iter)
    {
        TelepathyGLib.Channel? channel = null;
        int64 initiation_time = -1;
        ChessGame game;
        PGNGame pgn_game;
        bool game_needs_saving;

        channels.@get (channel_iter, ChannelsColumn.USER_ACTION_TIME, out initiation_time, ChannelsColumn.CHANNEL, out channel);

        debug ("Removing chess tube channel with %s initiated at instance: %lld", channel.target_contact.identifier, initiation_time);

        return false;
    }

    /* Quits the application */
    private void quit_game ()
    {
        Settings.sync ();
        channels.@foreach (save_game_and_remove_channel);
    }

    private new void game_move_cb (ChessGame game, ChessMove move)
    {
        Gtk.TreeIter *channel_iter;
        ChessView chess_view;

        channel_iter = (game as Object).get_data ("channel-iter");
        chess_view = (game as Object).get_data<ChessView> ("chess-view");

        /* Need to save after each move */
        channels.@set (*channel_iter, ChannelsColumn.GAME_NEEDS_SAVING, true);

        if (move.number > pgn_game.moves.length ())
            pgn_game.moves.append (move.get_san ());

        Gtk.ListStore model;
        ChessScene scene;
        channels.@get (*channel_iter, ChannelsColumn.HISTORY_MODEL, out model, ChannelsColumn.CHESS_SCENE, out scene);

        Gtk.TreeIter iter;
        model.append (out iter);
        model.set (iter, 1, move.number, -1);
        set_move_text (iter, move, scene, model);

        /* Follow the latest move */
        if (move.number == game.n_moves && scene.move_number == -1)
            channels.@set (*channel_iter, ChannelsColumn.HISTORY_COMBO_ACTIVE_ITER, iter);

        update_history_model (game, scene, model);

        /* UI updation */
        if (selected_channel_iter == *channel_iter)
        {
            update_history_panel (game,scene, model);
            update_control_buttons ();
        }

        /* TODO: Acknowledge if opponent made this move */

        chess_view.queue_draw ();
    }

    private void create_and_add_game (TelepathyGLib.DBusTubeChannel tube,
        Gtk.TreeIter iter)
    {
        /* Create a new game with the offered parameters */
        Variant tube_params = tube.dup_parameters_vardict ();
        bool our_color_white;
        tube_params.lookup ("offerer-white", "b", out our_color_white);
        int32 duration;
        tube_params.lookup ("duration", "i", out duration);

        var pgn_game = new PGNGame ();
        var now = new DateTime.now_local ();
        var now_UTC = now.to_utc ();
        pgn_game.date = now.format ("%Y.%m.%d");
        pgn_game.time = now.format ("%H:%M:%S");
        pgn_game.utcdate = now_UTC.format ("%Y.%m.%d");
        pgn_game.utctime = now_UTC.format ("%H:%M:%S");
        if (duration > 0)
            pgn_game.time_control = "%d".printf (duration);

        var history_model = new Gtk.ListStore (2, typeof (string), typeof (int));

        string fen = ChessGame.STANDARD_SETUP;
        string[] moves = new string[pgn_game.moves.length ()];
        var i = 0;
        foreach (var move in pgn_game.moves)
            moves[i++] = move;

        if (pgn_game.set_up)
        {
            if (pgn_game.fen != null)
                fen = pgn_game.fen;
            else
                warning ("Chess game has SetUp tag but no FEN tag");
        }
        var game = new ChessGame (fen, moves);
        (game as Object).set_data<PGNGame> ("pgn-game", pgn_game);
        (game as Object).set_data<Gtk.TreeIter*> ("channel-iter", &iter);

        if (pgn_game.time_control != null)
        {
            var controls = pgn_game.time_control.split (":");
            foreach (var control in controls)
            {
                /* We only support simple timeouts */
                var timeout_duration = int.parse (control);
                if (timeout_duration > 0)
                    game.clock = new ChessClock (timeout_duration, timeout_duration);
            }
        }

        game.moved.connect (game_move_cb);
        game.ended.connect (game_end_cb);
        if (game.clock != null)
            game.clock.tick.connect (game_clock_tick_cb);

        ChessScene scene = new ChessScene ();
        scene.changed.connect (scene_changed_cb);
        scene.choose_promotion_type.connect (show_promotion_type_selector);
        scene.game = game;

        Gtk.InfoBar info_bar;

        info_bar = new Gtk.InfoBar ();
        var content_area = (Gtk.Container) info_bar.get_content_area ();
        (view_box as Gtk.Box).pack_start (info_bar, false, true, 0);
        var vbox = new Gtk.Box (Gtk.Orientation.VERTICAL, 6);
        vbox.show ();
        content_area.add (vbox);
        var info_title_label = new Gtk.Label ("");
        info_title_label.show ();
        vbox.pack_start (info_title_label, false, true, 0);
        vbox.hexpand = true;
        vbox.vexpand = false;
        var info_label = new Gtk.Label ("");
        info_label.show ();
        vbox.pack_start (info_label, true, true, 0);

        info_bar.hide ();

        (game as Object).set_data<Gtk.InfoBar> ("info-bar", info_bar);
        (game as Object).set_data<Gtk.Label> ("info-title-label", info_title_label);
        (game as Object).set_data<Gtk.Label> ("info-label", info_label);

        ChessView chess_view;
        if (settings_common.get_boolean ("show-3d"))
            chess_view = new ChessView3D ();
        else
            chess_view = new ChessView2D ();
        chess_view.set_size_request (300, 300);
        /* This connects scene.changed() to a handler that draws view everytime scene changes */
        chess_view.scene = scene;

        (game as Object).set_data<ChessView> ("chess-view", chess_view);

        bool game_needs_saving = false;
        game.start ();

        if (game.result != ChessResult.IN_PROGRESS)
            game_end_cb (game);

        white_time_label.queue_draw ();
        black_time_label.queue_draw ();

        channels.insert_with_values (null, -1,
            ChannelsColumn.OUR_COLOUR_WHITE, our_color_white,
            ChannelsColumn.PGN_GAME, pgn_game,
            ChannelsColumn.HISTORY_MODEL, history_model,
            ChannelsColumn.CHESS_GAME, game,
            ChannelsColumn.CHESS_SCENE, scene,
            ChannelsColumn.INFO_BAR, info_bar,
            ChannelsColumn.INFO_BAR_VISIBILITY, info_bar.visible,
            ChannelsColumn.GAME_NEEDS_SAVING, game_needs_saving);
    }

    private void register_objects (DBusConnection connection,
        TelepathyGLib.DBusTubeChannel tube,
        Gtk.TreeIter channel_iter)
    {
      //  display_game ();
    }

    private void offer_tube (TelepathyGLib.DBusTubeChannel tube,
        Gtk.TreeIter channel_iter)
    {
        DBusConnection? connection = null;
        bool play_as_white = settings_common.get_boolean ("play-as-white");
        Value colour_val = play_as_white;
        int32 duration = settings_common.get_int ("duration");
        Value duration_val = duration;

        HashTable<string, Value?>? tube_params = new HashTable<string, Value?> (str_hash, str_equal);
        tube_params.insert ("offerer-white", colour_val);
        tube_params.insert ("duration", duration_val);

        debug ("Offering tube with offerer-white:%s and duration:%ld",
        colour_val.get_boolean () ? "true" : "false", duration_val.get_int ());
//        debug ("Parameters passed: %s", );
        tube.offer_async.begin ((HashTable<void*, void*>?)tube_params,
            (obj, res)=>{
                try
                {
                    connection = tube.offer_async.end (res);
                }
                catch (Error e)
                {
                    debug ("Failed to offer tube to %s: %s",
                        (tube as TelepathyGLib.Channel).target_contact.identifier,
                        e.message);
                  //  display_channel_error (channel_iter, e, "Failed to offer chess game channel to %s", (tube as TelepathyGLib.Channel).target_contact.identifier);
                    debug ("Closing channel.");
                    (tube as TelepathyGLib.Channel).close_async.begin ((obj, res)=>
                        {
                            try
                            {
                                (tube as TelepathyGLib.Channel).close_async.end (res);
                            }
                            catch (Error e)
                            {
                                debug ("Couldn't close the channel: %s", e.message);
                            }
                        });


                    /* Nothing to do */
                    return;
                }

                assert (connection != null);

                debug ("Tube opened.");
                connection.notify["closed"].connect (
                    (s, p)=>{
                        debug ("Connection to %s closed unexpectedly",
                            (tube as
                            TelepathyGLib.Channel).target_contact.identifier);
                    }
                );
                create_and_add_game (tube, channel_iter);
                register_objects (connection, tube, channel_iter);
              }
            );
    }

    private void accept_tube (TelepathyGLib.DBusTubeChannel tube,
        Gtk.TreeIter channel_iter)
    {
        DBusConnection? connection = null;

        debug ("Accepting tube");
        tube.accept_async.begin (
            (obj, res)=>{
                try
                {
                    connection = tube.accept_async.end (res);
                }
                catch (Error e)
                {
                    debug ("Failed to accept tube from %s: %s",
                        (tube as TelepathyGLib.Channel).target_contact.identifier,
                        e.message);
//                    display_error (channel_iter, e, "Failed to offer chess game channel to %s", (tube as TelepathyGLib.Channel).target_contact.identifer);
                    debug ("Closing channel.");

                    (tube as TelepathyGLib.Channel).close_async.begin ((obj, res)=>
                        {
                            try
                            {
                                (tube as TelepathyGLib.Channel).close_async.end (res);
                            }
                            catch (Error e)
                            {
                                debug ("Couldn't close the channel: %s", e.message);
                            }
                        });

                    /* Nothing to do */
                    return;
                }

                assert (connection != null);

                debug ("Tube opened.");
                connection.notify["closed"].connect (
                    (s, p)=>{
                        debug ("Connection to %s closed unexpectedly",
                            (tube as
                            TelepathyGLib.Channel).target_contact.identifier);
                    }
                );
                create_and_add_game (tube, channel_iter);
              }
            );

    }

    private void tube_invalidated_cb (TelepathyGLib.Proxy tube_channel,
        uint domain,
        int code,
        string message)
    {
        debug ("Tube has been invalidated: %s", message);

        channels.@foreach ((model, path, iter)=>
            {
                TelepathyGLib.Channel stored_tube;
                uint64 user_action_time;
                channels.@get (iter, ChannelsColumn.CHANNEL, out stored_tube,
                    ChannelsColumn.USER_ACTION_TIME, out user_action_time);

                if (stored_tube == tube_channel)
                {
                    debug ("Removing chess dbus-tube channel initiated at timeframe %llu with remote player %s",
                        user_action_time, stored_tube.target_contact.identifier);
                    channels.remove (iter);
                    return true;
                }

                return false;
            }
          );
    }

    public void handle_channels (TelepathyGLib.SimpleHandler handler,
        TelepathyGLib.Account account,
        TelepathyGLib.Connection conn,
        List<TelepathyGLib.Channel> channel_bundle,
        List<TelepathyGLib.ChannelRequest> requests,
        int64 action_time,
        TelepathyGLib.HandleChannelsContext context)
    {
        Error error = new TelepathyGLib.Error.NOT_AVAILABLE (
            "No channel to be handled");

        debug ("Handling channels");

        foreach (TelepathyGLib.Channel tube_channel in channel_bundle)
        {
          Gtk.TreeIter iter;

          if (! (tube_channel is TelepathyGLib.DBusTubeChannel))
            continue;
          if ((tube_channel as TelepathyGLib.DBusTubeChannel).service_name !=
              TelepathyGLib.CLIENT_BUS_NAME_BASE + "Games.Glchess")
            continue;

          /* Add channel to list of currently handled channels */
          channels.insert_with_values (out iter, -1,
              ChannelsColumn.USER_ACTION_TIME, action_time,
              ChannelsColumn.TARGET_CONTACT, tube_channel.target_contact,
              ChannelsColumn.CHANNEL, tube_channel);

          if (tube_channel.requested)
          {
              /* We created this channel. Make a tube offer and wait for
               * approval */
              offer_tube (tube_channel as TelepathyGLib.DBusTubeChannel, iter);
             // display_waiting_for_approval (iter);
          }
          else
          {
              /* This is an incoming channel request */
              accept_tube (tube_channel as TelepathyGLib.DBusTubeChannel, iter);
          }

          (tube_channel as TelepathyGLib.Proxy).invalidated.connect (
              tube_invalidated_cb);

          context.accept ();
          /* Only one of accept() or fail() can be called on context */
          return;
        }

        debug ("Rejecting channels");
        context.fail (error);

        return;
    }

    public override void activate ()
    {
      if (!is_activated)
      {
          /* Create and register our telepathy client handler as a one-off */

          tp_handler =  new TelepathyGLib.SimpleHandler.with_am (
              TelepathyGLib.AccountManager.dup (),
              true,     /* Bypass approval */
              true,    /* Requests */
              "Games.Glchess",
              false,    /* Uniquify name */
              handle_channels);

          var filter = new HashTable<string, Value?> (str_hash, str_equal);
          filter.insert (
              TelepathyGLib.PROP_CHANNEL_CHANNEL_TYPE,
              TelepathyGLib.IFACE_CHANNEL_TYPE_DBUS_TUBE);
          filter.insert (
              TelepathyGLib.PROP_CHANNEL_TARGET_HANDLE_TYPE,
              1);
          filter.insert (
              TelepathyGLib.PROP_CHANNEL_TYPE_DBUS_TUBE_SERVICE_NAME,
                TelepathyGLib.CLIENT_BUS_NAME_BASE + "Games.Glchess");

          (tp_handler as TelepathyGLib.BaseClient).add_handler_filter (filter);

          try {
              (tp_handler as TelepathyGLib.BaseClient).register ();
          }
          catch (Error e) {
              debug ("Failed to register handler: %s", e.message);
          }

          debug ("Waiting for tube offer");

          is_activated = true;
      }

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

