/* We should probably be using the GIR files, but I can't get them to work in
 * Vala.  This works for now but means it needs to be updated when the library
 * changes -- Robert Ancell */

[CCode (cprefix = "Games", lower_case_cprefix = "games_")]
namespace GamesContacts
{
    public const bool ENABLE_NETWORKING;

    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_SCORES;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_PAUSE_GAME;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_RESUME_GAME;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_FULLSCREEN;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_LEAVE_FULLSCREEN;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_NEW_GAME;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_START_NEW_GAME;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_NETWORK_GAME;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_NETWORK_LEAVE;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_PLAYER_LIST;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_RESTART_GAME;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_UNDO_MOVE;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_REDO_MOVE;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_HINT;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_END_GAME;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_CONTENTS;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_RESET;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_TELEPORT;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_RTELEPORT;
    [CCode (cheader_filename = "games-stock.h")]
    public const string STOCK_DEAL_CARDS;

    [CCode (cheader_filename = "games-stock.h")]
    public static void stock_init ();
    [CCode (cheader_filename = "games-stock.h")]
    public static void stock_prepare_for_statusbar_tooltips (Gtk.UIManager ui_manager, Gtk.Widget statusbar);
    [CCode (cheader_filename = "games-stock.h")]
    public static string get_license (string game_name);

    [CCode (cheader_filename = "games-settings.h")]    
    public static void settings_bind_window_state (string path, Gtk.Window window);

    [CCode (cheader_filename = "games-clock.h")]
    public class Clock : Gtk.Label
    {
        public Clock ();
        public void start ();
        public bool is_started ();
        public void stop ();
        public void reset ();
        public time_t get_seconds ();
        public void add_seconds (time_t seconds);
        public void set_updated (bool do_update);
    }
    
    [CCode (cheader_filename = "games-pause-action.h")]
    public class PauseAction : Gtk.Action
    {
        public signal void state_changed ();
        public PauseAction (string name);
        public void set_is_paused (bool is_paused);
        public bool get_is_paused ();
    }

    [CCode (cprefix = "GAMES_FULLSCREEN_ACTION_VISIBLE_")]
    public enum VisiblePolicy
    {
        ALWAYS,
        ON_FULLSCREEN,
        ON_UNFULLSCREEN
    }

    [CCode (cheader_filename = "games-fullscreen-action.h")]
    public class FullscreenAction : Gtk.Action
    {
        public FullscreenAction (string name, Gtk.Window window);
        public void set_visible_policy (VisiblePolicy visible_policy);
        public VisiblePolicy get_visible_policy ();
        public void set_is_fullscreen (bool is_fullscreen);
        public bool get_is_fullscreen ();
    }

    [CCode (cheader_filename = "games-scores.h")]
    public void scores_startup ();

    [CCode (cprefix = "GAMES_SCORES_STYLE_", cheader_filename = "games-score.h")]
    public enum ScoreStyle
    {
        PLAIN_DESCENDING,
        PLAIN_ASCENDING,
        TIME_DESCENDING,
        TIME_ASCENDING
    }
    
    [CCode (cheader_filename = "games-scores.h", destroy_function = "")]
    public struct ScoresCategory
    {
        string key;
        string name;
    }

    [CCode (cheader_filename = "games-score.h")]
    public class Score : GLib.Object
    {
        public Score ();
    }

    [CCode (cheader_filename = "games-scores.h")]
    public class Scores : GLib.Object
    {
        public Scores (string app_name, ScoresCategory[] categories, string? categories_context, string? categories_domain, int default_category_index, ScoreStyle style);
        public void set_category (string category);
        public int add_score (Score score);
        public int add_plain_score (uint32 value);
        public int add_time_score (double value);
        public void update_score (string new_name);
        public void update_score_name (string new_name, string old_name);
        public ScoreStyle get_style ();
        public unowned string get_category ();
        public void add_category (string key, string name);
    }
    
    [CCode (cprefix = "GAMES_SCORES_", cheader_filename = "games-scores-dialog.h")]
    public enum ScoresButtons
    {
        CLOSE_BUTTON,
        NEW_GAME_BUTTON,
        UNDO_BUTTON,
        QUIT_BUTTON
    }

    [CCode (cheader_filename = "games-scores-dialog.h")]
    public class ScoresDialog : Gtk.Dialog
    {
        public ScoresDialog (Gtk.Window parent_window, Scores scores, string title);
        public void set_category_description (string description);
        public void set_hilight (uint pos);
        public void set_message (string? message);
        public void set_buttons (uint buttons);
    }

    [CCode (cheader_filename = "games-preimage.h")]
    public class Preimage : GLib.Object
    {
        public Preimage ();
        public Preimage.from_file (string filename) throws GLib.Error;
        public void set_font_options (Cairo.FontOptions font_options);
        public Gdk.Pixbuf render (int width, int height);
        public void render_cairo (Cairo.Context cr, int width, int height);
        public Gdk.Pixbuf render_sub (string node, int width, int height, double xoffset, double yoffset, double xzoom, double yzoom);
        public void render_cairo_sub (Cairo.Context cr, string? node, int width, int height, double xoffset, double yoffset, double xzoom, double yzoom);
        public bool is_scalable ();
        public int get_width ();
        public int get_height ();
        public Gdk.Pixbuf render_unscaled_pixbuf ();
    }

    [CCode (cheader_filename = "games-file-list.h")]
    public const int FILE_LIST_REMOVE_EXTENSION;
    [CCode (cheader_filename = "games-file-list.h")]
    public const int FILE_LIST_REPLACE_UNDERSCORES;

    [CCode (cheader_filename = "games-file-list.h")]
    public class FileList : GLib.Object
    {
        public FileList (string glob, ...);
        public FileList.images (string path1, ...);
        public void transform_basename ();
        public size_t length ();
        public void for_each (GLib.Func<string> function);
        public string find (GLib.CompareFunc function);
        public unowned string? get_nth (int n);
        public Gtk.Widget create_widget (string selection, uint flags);
    }

    [CCode (cheader_filename = "games-controls.h")]
    public class ControlsList : Gtk.ScrolledWindow
    {
        public ControlsList (GLib.Settings settings);
        public void add_control (string conf_key, string label, uint default_keyval);
        public void add_controls (string first_conf_key, ...);
    }


#if ENABLE_NETWORKING

    [CCode (cheader_filename = "games-contact.h", copy_function = "g_boxed_copy", free_function = "g_boxed_free", type_id = "games_avatar_get_type ()")]
    [Compact]
    public class Avatar {
        public uint8 data;
        public weak string filename;
        public weak string format;
        public size_t len;
        public uint refcount;
        public weak string token;
        [CCode (has_construct_function = false)]
        public Avatar (uint8 data, size_t len, string format, string filename);
        public GamesContacts.Avatar @ref ();
        public bool save_to_file (string filename) throws GLib.Error;
        public void unref ();
    }

    [CCode (cheader_filename = "games-contact.h", type_id = "games_contact_get_type ()")]
    public class Contact : GLib.Object {
        [CCode (has_construct_function = false)]
        protected Contact ();
        public bool can_do_action (GamesContacts.ActionType action_type);
        public bool can_play_glchess ();
        public static bool equal (void* contact1, void* contact2);
        public unowned string get_alias ();
        public GamesContacts.Avatar get_avatar ();
        public GamesContacts.Capabilities get_capabilities ();
        public uint get_handle ();
        public unowned string get_id ();
        public TelepathyGLib.ConnectionPresenceType get_presence ();
        public unowned string get_presence_message ();
        public unowned string get_status ();
        public bool is_online ();
        public void set_alias (string alias);
        public void set_is_user (bool is_user);
        public void set_persona (Folks.Persona persona);
        [NoAccessorMethod]
        public TelepathyGLib.Account account { owned get; construct; }
        public string alias { get; set; }
        public GamesContacts.Avatar avatar { owned get; }
        [NoAccessorMethod]
        public uint handle { get; set; }
        [NoAccessorMethod]
        public string id { owned get; set; }
        [NoAccessorMethod]
        public bool is_user { get; set; }
        [NoAccessorMethod]
        public Folks.Persona persona { owned get; set; }
        [NoAccessorMethod]
        public uint presence { get; set; }
        [NoAccessorMethod]
        public string presence_message { owned get; set; }
        [NoAccessorMethod]
        public TelepathyGLib.Contact tp_contact { owned get; construct; }
        public signal void presence_changed (uint object, uint p0);
    }

    [CCode (cheader_filename = "games-individual-store.h", type_id = "games_individual_store_get_type ()")]
    public class IndividualStore : Gtk.TreeStore, Gtk.Buildable, Gtk.TreeDragDest, Gtk.TreeDragSource, Gtk.TreeModel, Gtk.TreeSortable {
        [CCode (has_construct_function = false)]
        protected IndividualStore ();
        public void add_individual (Folks.Individual individual);
        [CCode (cname = "individual_store_add_individual_and_connect")]
        public void add_individual_and_connect (Folks.Individual individual);
        public void disconnect_individual (Folks.Individual individual);
        public bool get_is_compact ();
        public static string get_parent_group (Gtk.TreeModel model, Gtk.TreePath path, bool path_is_group, bool is_fake_group);
        public bool get_show_avatars ();
        public bool get_show_groups ();
        public bool get_show_protocols ();
        public GamesContacts.IndividualStoreSort get_sort_criterium ();
        [NoWrapper]
        public virtual bool initial_loading ();
        public void refresh_individual (Folks.Individual individual);
        [NoWrapper]
        public virtual void reload_individuals ();
        public void remove_individual (Folks.Individual individual);
        [CCode (cname = "individual_store_remove_individual_and_disconnect")]
        public void remove_individual_and_disconnect (Folks.Individual individual);
        public static bool row_separator_func (Gtk.TreeModel model, Gtk.TreeIter iter, void* data);
        public void set_is_compact (bool is_compact);
        public void set_show_avatars (bool show_avatars);
        public void set_show_groups (bool show_groups);
        public void set_show_protocols (bool show_protocols);
        public void set_sort_criterium (GamesContacts.IndividualStoreSort sort_criterium);
        public bool is_compact { get; set; }
        public bool show_avatars { get; set; }
        public bool show_groups { get; set; }
        public bool show_protocols { get; set; }
    }

    [CCode (cheader_filename = "games-individual-view.h", type_id = "games_individual_view_get_type ()")]
    public class IndividualView : Gtk.TreeView, Atk.Implementor, Gtk.Buildable, Gtk.Scrollable {
        [CCode (has_construct_function = false)]
        public IndividualView (GamesContacts.IndividualStore store, GamesContacts.IndividualViewFeatureFlags view_features);
        [CCode (cname = "individual_view_filter_default")]
        public static bool filter_default (Gtk.TreeModel model, Gtk.TreeIter iter, void* user_data, GamesContacts.ActionType interest);
        public bool get_show_offline ();
        public bool get_show_untrusted ();
        public bool is_searching ();
        public void refilter ();
        public void select_first ();
        public void set_live_search (GamesContacts.LiveSearch search);
        public void set_show_offline (bool show_offline);
        public void set_show_uninteresting (bool show_uninteresting);
        public void set_show_untrusted (bool show_untrusted);
        public void set_store (GamesContacts.IndividualStore store);
        public void start_search ();
        [NoAccessorMethod]
        public bool show_uninteresting { get; set; }
        public bool show_untrusted { get; set; }
        [NoAccessorMethod]
        public GamesContacts.IndividualStore store { owned get; set; }
    }

    [CCode (cheader_filename = "games-live-search.h", type_id = "games_live_search_get_type ()")]
    public class LiveSearch : Gtk.HBox, Atk.Implementor, Gtk.Buildable, Gtk.Orientable {
        [CCode (has_construct_function = false, type = "GtkWidget*")]
        public LiveSearch (Gtk.Widget hook);
        public unowned string get_text ();
        public bool match (string string);
        public static bool match_string (string string, string prefix);
        public static bool match_words (string string, GLib.GenericArray<void*> words);
        public void set_hook_widget (Gtk.Widget hook);
        public void set_text (string text);
        [NoAccessorMethod]
        public Gtk.Widget hook_widget { owned get; set; }
        public string text { get; set; }
        public signal void activate ();
        public signal bool key_navigation (Gdk.Event object);
    }

    [CCode (cheader_filename = "games-contact.h", cprefix = "GAMES_ACTION_")]
    public enum ActionType {
        CHAT,
        PLAY_GLCHESS
    }
    [CCode (cheader_filename = "games-contact.h", cprefix = "GAMES_CAPABILITIES_")]
    [Flags]
    public enum Capabilities {
        NONE,
        TUBE_GLCHESS,
        UNKNOWN
    }
    [CCode (cheader_filename = "games-individual-store.h", cprefix = "GAMES_INDIVIDUAL_STORE_COL_")]
    public enum IndividualStoreCol {
        ICON_STATUS,
        PIXBUF_AVATAR,
        PIXBUF_AVATAR_VISIBLE,
        NAME,
        PRESENCE_TYPE,
        STATUS,
        COMPACT,
        INDIVIDUAL,
        IS_GROUP,
        IS_ACTIVE,
        IS_ONLINE,
        IS_SEPARATOR,
        INDIVIDUAL_CAPABILITIES,
        IS_FAKE_GROUP,
        COUNT
    }
    [CCode (cheader_filename = "games-individual-store.h", cprefix = "GAMES_INDIVIDUAL_STORE_SORT_")]
    public enum IndividualStoreSort {
        STATE,
        NAME
    }
    [CCode (cheader_filename = "games-individual-view.h", cprefix = "GAMES_INDIVIDUAL_VIEW_FEATURE_")]
    [Flags]
    public enum IndividualViewFeatureFlags {
        NONE,
        GROUPS_SAVE
    }

#endif
}

