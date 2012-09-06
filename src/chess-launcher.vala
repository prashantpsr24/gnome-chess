/* chess-launcher.vala: This is a GtkWindow that helps you
 * launch a new game chess and remembers your last launch settings.
 *
 * Copyright © 2012 Chandni Verma
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

public class ChessLauncher : Gtk.Window
{
    private Gtk.Builder builder;
    private Settings settings;
    public signal void start_game ();
    public signal void load_game (File file, bool from_history);

    /* Chess welcome screen widgets */
    private Gtk.Widget grid_welcome;
    private Gtk.Widget grid_select_game;
    private Gtk.Widget grid_select_opponent;
    private Gtk.Widget grid_select_remote_player;
    private Gtk.Widget grid_game_options;
    private Gtk.Widget grid_preferences;

    private Gtk.Action radioaction_previous_game;
    private Gtk.Action radioaction_new_game;

    private Gtk.Widget togglebutton_robot;
    private Gtk.Widget togglebutton_remote_player;
    private Gtk.Action radioaction_opponent_robot;
    private Gtk.Action radioaction_opponent_local_player;
    private Gtk.Action radioaction_opponent_remote_player;

    private Gtk.Widget contacts_box;
    private Gtk.Widget scrolledwindow_remote_player;
    private GamesContacts.IndividualStore? individual_store = null;
    private GamesContacts.IndividualManager? individual_manager = null;
    private GamesContacts.IndividualView? individual_view = null;
    private GamesContacts.LiveSearch? search_widget;
    private GamesContacts.Contact? contact = null;

    private Gtk.Widget togglebutton_easy;
    private Gtk.Widget togglebutton_normal;
    private Gtk.Widget togglebutton_difficult;
    private Gtk.Action radioaction_easy;
    private Gtk.Action radioaction_normal;
    private Gtk.Action radioaction_difficult;
    private Gtk.Action radioaction_white;
    private Gtk.Action radioaction_black;
    private Gtk.ComboBox duration_combo;
    private Gtk.Adjustment duration_adjustment;
    private Gtk.Container custom_duration_box;
    private Gtk.ComboBox custom_duration_units_combo;

    private Gtk.TreeView treeview_robots;
    private Gtk.ListStore robot_list_model;
    private Gtk.Button done_button;
    private Gtk.Widget grid_installable_robots;
    private Gtk.Label label_install_robots;

    private uint save_duration_timeout = 0;
    private History history;

    public ChessLauncher (string engines_file,
        List<AIProfile>? ai_profiles,
        History history)
    {
        this.history = history;

        settings = new Settings ("org.gnome.gnome-chess.Settings");
        builder = new Gtk.Builder ();
        try
        {
          builder.add_from_file (Path.build_filename (Config.PKGDATADIR,
              "new-game-launcher.ui", null));
        }
        catch (Error e)
        {
          warning ("Could not load UI: %s", e.message);
        }

        Gtk.Window ui_window = (Gtk.Window) builder.get_object ("gnome_chess_app");

        grid_welcome = (Gtk.Widget) builder.get_object ("grid_welcome");

        /* Make this window hold all widgets of UI description's window
           container */
        grid_welcome.reparent (this);
        ui_window.destroy ();

        grid_select_game = (Gtk.Widget) builder.get_object ("grid_select_game");
        grid_select_opponent = (Gtk.Widget) builder.get_object ("grid_select_opponent");
        grid_select_remote_player = (Gtk.Widget) builder.get_object ("grid_select_remote_player");
        grid_game_options = (Gtk.Widget) builder.get_object ("grid_game_options");
        grid_preferences = (Gtk.Widget) builder.get_object ("grid_preferences");

        radioaction_previous_game = (Gtk.Action) builder.get_object ("radioaction_previous_game");
        radioaction_new_game = (Gtk.Action) builder.get_object ("radioaction_new_game");

        togglebutton_robot = (Gtk.Widget) builder.get_object ("togglebutton_robot");
        togglebutton_remote_player = (Gtk.Widget) builder.get_object
        ("togglebutton_remote_player");

        radioaction_opponent_robot = (Gtk.Action) builder.get_object ("radioaction_opponent_robot");
        radioaction_opponent_local_player = (Gtk.Action) builder.get_object ("radioaction_opponent_local_player");
        radioaction_opponent_remote_player = (Gtk.Action) builder.get_object ("radioaction_opponent_remote_player");

        scrolledwindow_remote_player = (Gtk.ScrolledWindow) builder.get_object ("scrolledwindow_remote_player");
        contacts_box = (Gtk.Widget) builder.get_object ("contacts_box");

        togglebutton_easy = (Gtk.Widget) builder.get_object ("togglebutton_easy");
        togglebutton_normal = (Gtk.Widget) builder.get_object ("togglebutton_normal");
        togglebutton_difficult = (Gtk.Widget) builder.get_object ("togglebutton_difficult");
        radioaction_easy = (Gtk.Action) builder.get_object ("radioaction_easy");
        radioaction_normal = (Gtk.Action) builder.get_object ("radioaction_normal");
        radioaction_difficult = (Gtk.Action) builder.get_object ("radioaction_difficult");
        radioaction_white = (Gtk.Action) builder.get_object ("radioaction_white");
        radioaction_black = (Gtk.Action) builder.get_object ("radioaction_black");
        duration_combo = (Gtk.ComboBox) builder.get_object ("duration_combo");
        duration_adjustment = (Gtk.Adjustment) builder.get_object ("duration_adjustment");
        custom_duration_box = (Gtk.Container) builder.get_object ("custom_duration_box");
        custom_duration_units_combo = (Gtk.ComboBox) builder.get_object ("custom_duration_units_combo");


        treeview_robots = (Gtk.TreeView) builder.get_object ("treeview_robots");
        robot_list_model = (Gtk.ListStore) treeview_robots.model;
        done_button = (Gtk.Button) builder.get_object ("button_preferences_done");
        grid_installable_robots = (Gtk.Widget) builder.get_object ("grid_installable_robots");
        label_install_robots = (Gtk.Label) builder.get_object ("label_install_robots");

        /* Connect signals of all objects to their respective handlers */
        builder.connect_signals (this);

        this.title = _("Chess");

        grid_welcome.show ();

        (this as Gtk.Widget).configure_event.connect
            (launcher_configure_event_cb);

        /* Populate robots/ prepare installation message */

        if (ai_profiles == null)
        {
            togglebutton_robot.hide ();
            show_robot_opponent_widgets (false);

            StringBuilder engine_names = get_supported_engines (engines_file);
            engine_names.prepend ("No robot is currently installed.\n" +
                "Here is a list of suported robots. Install atleast one for " +
                "advanced options.\n\n");

            label_install_robots.set_label (engine_names.str);
        }
        else
        {
            /* Populate list of robots */

            Gtk.TreeIter iter;
            robot_list_model.clear ();

            foreach (var profile in ai_profiles)
            {
                robot_list_model.append (out iter);
                robot_list_model.set (iter, 0, profile.name, -1);
            }

            message ("No. of robots: %d", robot_list_model.iter_n_children (null));
            treeview_robots.insert_column_with_attributes (-1, "Robot Name", new
                Gtk.CellRendererText (), "text", 0);
            treeview_robots.set_headers_visible (false);

            var robot_selection = treeview_robots.get_selection ();
            if (robot_list_model.get_iter_first (out iter))
                robot_selection.select_iter (iter);
        }

        /* Check if networking is enabled */
        if (!Config.ENABLE_NETWORKING)
          togglebutton_remote_player.hide ();

        /* Remote players list */
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

        /* Add filter for contacts capable of playing glchess */
        individual_view.set_custom_filter ((model, iter, data)=>{
              return (data as GamesContacts.IndividualView).filter_default (
                  model, iter, GamesContacts.ActionType.PLAY_GLCHESS);
            });

        /* Evaluate contacts' presence using filter */
        individual_view.refilter ();

        /* Add to scrolled window and show */
        (scrolledwindow_remote_player as Gtk.Container).add (individual_view);
        scrolledwindow_remote_player.show ();
        individual_view.show ();

        (contacts_box as Gtk.Box).set_orientation (Gtk.Orientation.VERTICAL);
        (contacts_box as Gtk.Container).add (search_widget);
    }

    ~ChessLauncher ()
    {
        if (save_duration_timeout != 0)
            save_duration_cb ();

        settings.sync ();
    }

    private bool launcher_configure_event_cb (Gtk.Widget widget,
        Gdk.EventConfigure event)
    {
        if (grid_preferences.visible)
        {
            settings.set_int ("preferences-screen-width", event.width);
            settings.set_int ("preferences-screen-height",
            event.height);
        }
        else
        {
            if (grid_select_remote_player.visible)
            {
                settings.set_int ("remote-player-selector-width",
                event.width);
                settings.set_int ("remote-player-selector-height",
                event.height);
            }
            else
            {
                settings.set_int ("welcome-screen-width", event.width);
                settings.set_int ("welcome-screen-height", event.height);
            }
        }

        return false;
    }

    private StringBuilder get_supported_engines (string filename)
    {
       var file = new KeyFile ();
       StringBuilder names = new StringBuilder ();

       debug ("Loading AI names");
       try
       {
           file.load_from_file (filename, KeyFileFlags.NONE);
       }
       catch (KeyFileError e)
       {
           warning ("Failed to load AI names: %s", e.message);
           return names;
       }
       catch (FileError e)
       {
           warning ("Failed to load AI names: %s", e.message);
           return names;
       }

       foreach (string name in file.get_groups ())
       {
           if (name != null)
           {
               names.append (name);
               names.append_c ('\n');
           }
       }

       return names;
    }

    public void show_game_selector ()
    {
        this.resize (settings.get_int ("welcome-screen-width"),
            settings.get_int ("welcome-screen-height"));

        grid_select_game.show ();
        grid_select_opponent.hide ();
        grid_select_remote_player.hide ();
        grid_game_options.hide ();
        grid_preferences.hide ();
    }

    public void show_opponent_selector ()
    {
        this.resize (settings.get_int ("welcome-screen-width"),
            settings.get_int ("welcome-screen-height"));

        grid_select_game.hide ();
        grid_select_opponent.show ();
        grid_select_remote_player.hide ();
        grid_game_options.hide ();
        grid_preferences.hide ();
    }

    private void show_remote_player_selector ()
    {
        this.resize (settings.get_int ("remote-player-selector-width"),
            settings.get_int ("remote-player-selector-height"));

        grid_select_game.hide ();
        grid_select_opponent.hide ();

        grid_select_remote_player.show ();
        (contacts_box as Gtk.Widget).grab_focus ();

        grid_game_options.hide ();
        grid_preferences.hide ();
    }

    public void show_game_options ()
    {
        this.resize (settings.get_int ("welcome-screen-width"),
            settings.get_int ("welcome-screen-height"));

        grid_select_game.hide ();
        grid_select_opponent.hide ();
        grid_select_remote_player.hide ();
        grid_game_options.show ();
        grid_preferences.hide ();

        /* Present settings */
        string difficulty = settings.get_string ("difficulty");
        if (difficulty == "easy")
            radioaction_easy.activate ();
        else
            if (difficulty == "normal")
                radioaction_normal.activate ();
            else
                radioaction_difficult.activate ();

        bool play_as_white = settings.get_boolean ("play-as-white");
        if (play_as_white)
            radioaction_white.activate ();
        else
            radioaction_black.activate ();

        set_duration (settings.get_int ("duration"));
    }

    private void show_robot_installation_choice (bool install)
    {
        if (install)
        {
            treeview_robots.hide ();
            done_button.hide ();
            grid_installable_robots.show ();
        }
        else
        {
            treeview_robots.show ();
            done_button.show ();
            grid_installable_robots.hide ();
        }
    }

    private void show_preferences ()
    {
        this.resize (settings.get_int ("preferences-screen-width"),
            settings.get_int ("preferences-screen-height"));

        grid_select_game.hide ();
        grid_select_opponent.hide ();
        grid_select_remote_player.hide ();
        grid_game_options.hide ();
        grid_preferences.show ();

        if (((Gtk.TreeModel)robot_list_model).iter_n_children (null) != 0)
        {
            Gtk.TreeIter iter;

            show_robot_installation_choice (false);

            for (bool valid_iter = robot_list_model.get_iter_first (out iter);
                 valid_iter;
                 valid_iter = robot_list_model.iter_next (ref iter))
            {
                string robot = null;
                robot_list_model.get (iter, 0, &robot, -1);

                if (settings.get_string ("opponent") == robot)
                {
                    var robot_selection = treeview_robots.get_selection ();
                    robot_selection.select_iter (iter);
                }
                break;
            }

            if (settings.get_string ("opponent-type") == "robot")
            {
                treeview_robots.sensitive = true;
                done_button.label = _("_Done");
            }
            else
            {
                treeview_robots.sensitive = false;
                done_button.label = _("_Back");
            }
        }
        else
        {
            /* Present an option to install chess engines */
            show_robot_installation_choice (true);
        }
    }

    [CCode (cname = "G_MODULE_EXPORT game_selected_cb", instance_pos = 1.1)]
    public void game_selected_cb (Gtk.Action action) throws Error
    {
        if (action == radioaction_previous_game)
        {
            var unfinished = history.get_unfinished ();
            var file = unfinished.data;

            /* Signal load_game */
            load_game (file, true);
        }
        else
        {
            show_opponent_selector ();
        }
    }

    private void show_robot_opponent_widgets (bool robot_selected)
    {
        if (robot_selected)
        {
            togglebutton_easy.show ();
            togglebutton_normal.show ();
            togglebutton_difficult.show ();
        }
        else
        {
            togglebutton_easy.hide ();
            togglebutton_normal.hide ();
            togglebutton_difficult.hide ();
        }
    }

    [CCode (cname = "G_MODULE_EXPORT opponent_changed_cb", instance_pos = -1)]
    public void opponent_changed_cb (Gtk.Action action)
    {
        if (action == radioaction_opponent_robot)
        {
            settings.set_string ("opponent-type", "robot");

            show_robot_opponent_widgets (true);
            show_game_options ();
        }
        else
        {
            show_robot_opponent_widgets (false);
            if (action == radioaction_opponent_local_player)
            {
                settings.set_string ("opponent-type", "local-player");
                show_game_options ();
            }
            else
            {
                settings.set_string ("opponent-type", "remote-player");
                show_remote_player_selector ();
            }
        }
    }

    [CCode (cname = "G_MODULE_EXPORT chat_clicked_cb", instance_pos = -1)]
    public void chat_clicked_cb (Gtk.Button button)
    {
      unowned Gtk.TreeModel individual_store;
      Gtk.TreeIter iter;
      Folks.Individual? individual = null;
      GamesContacts.Contact contact;
      int64 timestamp = TelepathyGLib.user_action_time_from_x11 (0);

      var selection = individual_view.get_selection ();
      if (selection.get_selected (out individual_store, out iter))
      {
          (individual_store).@get (iter,
              GamesContacts.IndividualStoreCol.INDIVIDUAL, out individual);

          if (individual != null)
          {
              contact = GamesContacts.Contact.dup_best_for_action (
                  individual, GamesContacts.ActionType.PLAY_GLCHESS);

              GamesContacts.chat_with_contact (contact, timestamp);
          }
      }
    }

    [CCode (cname = "G_MODULE_EXPORT remote_player_selected_cb", instance_pos = -1)]
    public void remote_player_selected_cb (Gtk.Button button)
    {
        Gtk.TreeModel model;
        Gtk.TreeIter iter;

        /* Cache player details */
        var selection = individual_view.get_selection ();
        if (selection.get_selected (out model, out iter))
        {
            Folks.Individual? individual = null;
            model.@get (iter,
                  GamesContacts.IndividualStoreCol.INDIVIDUAL,
                  out individual);
            if (individual != null)
            {
                contact = GamesContacts.Contact.dup_best_for_action (
                    individual, GamesContacts.ActionType.PLAY_GLCHESS);
                string opponent;

                settings.set_string ("opponent", contact.id);

                opponent = settings.get_string ("opponent");
                debug ("opponent selected: %s\n", opponent);

                show_game_options ();
            }
        }
    }

    [CCode (cname = "G_MODULE_EXPORT remote_player_selection_cancelled_cb", instance_pos = -1)]
    public void remote_player_selection_cancelled_cb (Gtk.Button button)
    {
        show_opponent_selector ();
    }

    [CCode (cname = "G_MODULE_EXPORT color_selection_changed_cb", instance_pos = -1)]
    public void color_selection_changed_cb (Gtk.Action action)
    {
        bool play_as_white = (action == radioaction_white) ? true : false;

        settings.set_boolean ("play-as-white", play_as_white);
    }

    [CCode (cname = "G_MODULE_EXPORT difficulty_changed_cb", instance_pos = -1)]
    public void difficulty_changed_cb (Gtk.Action action)
    {
        if (action == radioaction_easy)
            settings.set_string ("difficulty", "easy");
        else
            if (action == radioaction_normal)
                settings.set_string ("difficulty", "normal");
            else
                settings.set_string ("difficulty", "hard");
    }

    private void ensure_legacy_channel_async ()
    {
        TelepathyGLib.AccountChannelRequest req;
        var ht = new HashTable<string, Variant> (str_hash, str_equal);

        req = new TelepathyGLib.AccountChannelRequest (
            contact.account, ht,
            TelepathyGLib.user_action_time_from_x11 (0));

        req.set_request_property (
            TelepathyGLib.PROP_CHANNEL_CHANNEL_TYPE,
            new Variant.string (TelepathyGLib.IFACE_CHANNEL_TYPE_TUBES));
        req.set_request_property (
            TelepathyGLib.PROP_CHANNEL_TARGET_HANDLE_TYPE,
            new Variant.uint16 (TelepathyGLib.HandleType.CONTACT));
        req.set_request_property (
            TelepathyGLib.PROP_CHANNEL_TARGET_ID,
            new Variant.string (contact.id));

        req.ensure_channel_async.begin ("", null);
    }

    private void create_dbus_tube_channel ()
    {
        TelepathyGLib.AccountChannelRequest req;
        var ht = new HashTable<string, Variant> (str_hash, str_equal);
        bool channel_dispatched = true;
        string service = TelepathyGLib.CLIENT_BUS_NAME_BASE + "Gnome.Chess";

        /* Workaround for https://bugs.freedesktop.org/show_bug.cgi?id=47760 */
        ensure_legacy_channel_async ();

        req = new TelepathyGLib.AccountChannelRequest (
            contact.account, ht,
            TelepathyGLib.user_action_time_from_x11 (0));

        req.set_request_property (
            TelepathyGLib.PROP_CHANNEL_CHANNEL_TYPE,
            new Variant.string (TelepathyGLib.IFACE_CHANNEL_TYPE_DBUS_TUBE));
        req.set_request_property (
            TelepathyGLib.PROP_CHANNEL_TARGET_HANDLE_TYPE,
            new Variant.uint16 (TelepathyGLib.HandleType.CONTACT));
        req.set_request_property (
            TelepathyGLib.PROP_CHANNEL_TARGET_ID,
            new Variant.string (contact.id));
        req.set_request_property (
            TelepathyGLib.PROP_CHANNEL_TYPE_DBUS_TUBE_SERVICE_NAME,
            new Variant.string (service));

        req.create_channel_async.begin (service, null,
            (obj, res)=>{
                try
                {
                     channel_dispatched = req.create_channel_async.end (res);
                }
                catch (Error e)
                {
                     Gtk.MessageDialog error_dialog =
                      new Gtk.MessageDialog (
                          this,
                          Gtk.DialogFlags.DESTROY_WITH_PARENT|
                          Gtk.DialogFlags.MODAL,
                          Gtk.MessageType.INFO,
                          Gtk.ButtonsType.OK,
                          _("Failed to create channel"));
                     error_dialog.format_secondary_text (
                      _("The error is: %s"), e.message);

                     error_dialog.run ();
                     error_dialog.destroy ();

                     show_game_selector ();

                     /* Not yet done launching. Don't quit */
                     return;
                }

                if (channel_dispatched)
                  debug ("DBus channel with %s successfully dispatched.",
                      contact.id);
                else
                  debug ("Unsuccessful in creating and dispatching DBus channel with %s.",
                      contact.id);
                /* signal start_game which eventually destroys this launcher */
                start_game ();
              }
            );
    }

    [CCode (cname = "G_MODULE_EXPORT start_game_clicked_cb", instance_pos = -1)]
    public void start_game_clicked_cb (Gtk.Button button)
    {
        /* Create game channel with the selected remote contact if any;
           eventually destroy */
        if (contact != null)
            create_dbus_tube_channel ();
        else
            /* signal start_game which eventually destroys this launcher */
            start_game ();
    }

    [CCode (cname = "G_MODULE_EXPORT game_options_preferences_clicked_cb", instance_pos = -1)]
    public void game_options_preferences_clicked_cb (Gtk.Button button)
    {
        show_preferences ();
    }

    [CCode (cname = "G_MODULE_EXPORT preferences_back_clicked_cb", instance_pos = -1)]
    public void preferences_back_clicked_cb (Gtk.Button button)
    {
        show_game_options ();
    }

    [CCode (cname = "G_MODULE_EXPORT game_options_go_back_clicked_cb", instance_pos = -1)]
    public void game_options_go_back_clicked_cb (Gtk.Button button)
    {
        show_opponent_selector ();
    }

    [CCode (cname = "G_MODULE_EXPORT preferences_done_cb", instance_pos = -1)]
    public void preferences_done_cb (Gtk.Button button)
    {
        var robot_selection = treeview_robots.get_selection ();
        Gtk.TreeIter iter;
        robot_selection.get_selected (null, out iter);

        string selected_robot_opponent = null;
        treeview_robots.model.get (iter, 0, &selected_robot_opponent);
        settings.set_string ("opponent", selected_robot_opponent);

        show_game_options ();
    }

    private void set_duration (int duration, bool simplify = true)
    {
        var model = custom_duration_units_combo.model;
        Gtk.TreeIter iter, max_iter = {};

        /* Find the largest units that can be used for this value */
        int max_multiplier = 0;
        if (model.get_iter_first (out iter))
        {
            do
            {
                int multiplier;
                model.get (iter, 1, out multiplier, -1);
                if (multiplier > max_multiplier && duration % multiplier == 0)
                {
                    max_multiplier = multiplier;
                    max_iter = iter;
                }
            } while (model.iter_next (ref iter));
        }

        /* Set the spin button to the value with the chosen units */
        var value = 0;
        if (max_multiplier > 0)
        {
            value = duration / max_multiplier;
            duration_adjustment.value = value;
            custom_duration_units_combo.set_active_iter (max_iter);
        }

        if (!simplify)
            return;

        model = duration_combo.model;
        if (!model.get_iter_first (out iter))
            return;
        do
        {
            int v;
            model.get (iter, 1, out v, -1);
            if (v == duration || v == -1)
            {
                duration_combo.set_active_iter (iter);
                custom_duration_box.visible = v == -1;
                return;
            }
        } while (model.iter_next (ref iter));
    }

    private int get_duration ()
    {
        Gtk.TreeIter iter;
        if (duration_combo.get_active_iter (out iter))
        {
            int duration;
            duration_combo.model.get (iter, 1, out duration, -1);
            if (duration >= 0)
                return duration;
        }
    
        var magnitude = (int) duration_adjustment.value;
        int multiplier = 1;
        if (custom_duration_units_combo.get_active_iter (out iter))
            custom_duration_units_combo.model.get (iter, 1, out multiplier, -1);
        return magnitude * multiplier;
    }

    private bool save_duration_cb ()
    {
        settings.set_int ("duration", get_duration ());
        Source.remove (save_duration_timeout);
        save_duration_timeout = 0;
        return false;
    }

    [CCode (cname = "G_MODULE_EXPORT duration_changed_cb", instance_pos = -1)]
    public void duration_changed_cb (Gtk.Adjustment adjustment)
    {
        var model = (Gtk.ListStore) custom_duration_units_combo.model;
        Gtk.TreeIter iter;
        /* Set the unit labels to the correct plural form */
        if (model.get_iter_first (out iter))
        {
            do
            {
                int multiplier;
                model.get (iter, 1, out multiplier, -1);
                switch (multiplier)
                {
                case 1:
                    model.set (iter, 0, ngettext (/* Preferences Dialog: Combo box entry for a custom game timer set in seconds */
                                                  "second", "seconds", (ulong) adjustment.value), -1);
                    break;
                case 60:
                    model.set (iter, 0, ngettext (/* Preferences Dialog: Combo box entry for a custom game timer set in minutes */
                                                  "minute", "minutes", (ulong) adjustment.value), -1);
                    break;
                case 3600:
                    model.set (iter, 0, ngettext (/* Preferences Dialog: Combo box entry for a custom game timer set in hours */
                                                  "hour", "hours", (ulong) adjustment.value), -1);
                    break;
                }
            } while (model.iter_next (ref iter));
        }

        save_duration ();
    }

    [CCode (cname = "G_MODULE_EXPORT duration_units_changed_cb", instance_pos = -1)]
    public void duration_units_changed_cb (Gtk.Widget widget)
    {
        save_duration ();
    }

    private void save_duration ()
    {
        /* Delay writing the value as it this event will be generated a lot spinning through the value */
        if (save_duration_timeout != 0)
            Source.remove (save_duration_timeout);
        save_duration_timeout = Timeout.add (100, save_duration_cb);
    }

    [CCode (cname = "G_MODULE_EXPORT duration_combo_changed_cb", instance_pos = -1)]
    public void duration_combo_changed_cb (Gtk.ComboBox combo)
    {
        Gtk.TreeIter iter;
        if (!combo.get_active_iter (out iter))
            return;
        int duration;
        combo.model.get (iter, 1, out duration, -1);
        custom_duration_box.visible = duration < 0;

        if (duration >= 0)
            set_duration (duration, false);
        /* Default to 5 minutes when setting custom duration */
        else if (get_duration () <= 0)
            set_duration (5 * 60, false);

        save_duration ();
    }



}
