/* A test app. to show performing DBus-Tube communication in Vala */

[DBus (name = "org.gnome.RemoteGame.OfferedObject")]
public interface RemoteObjectIface : Object {
    public abstract string word {owned get; set;}
    public signal bool do_move_remote (string verb);
    public abstract void remote_yell () throws GLib.IOError;
}

[DBus (name = "org.gnome.RemoteGame.OfferedObject")]
public class LocalObject : Object
{
    public string word {get; construct;}
    public signal bool do_move ();

    public LocalObject (string word)
    {
        Object (word:word);
    }
}

public class RemoteObject : LocalObject, RemoteObjectIface
{
    public RemoteObject (string word)
    {
        base (word);
    }

    public void remote_yell ()
    {
        stdout.printf ("I am happy!");
    }
}

public class RemoteGameHandler : Application
{
    public string my_account {get; construct;}
    public string remote_id {get; construct;}
    string service;
    TelepathyGLib.SimpleHandler tp_handler;

    public RemoteGameHandler (string my_account, string remote_id)
    {
        Object (my_account:my_account, remote_id:remote_id);
        service = TelepathyGLib.CLIENT_BUS_NAME_BASE + "Remote.Game";
    }

    private void register_objects (DBusConnection connection, TelepathyGLib.DBusTubeChannel tube, string side)
    {
        debug ("Registering objects over dbus connection");

        Variant tube_params = tube.dup_parameters_vardict ();
        string say = (tube_params.lookup_value ("say", VariantType.STRING)).get_string ();

        RemoteObject my_object = new RemoteObject (say);
        my_object.do_move_remote.connect ((obj, verb)=>{stdout.printf (verb + "ing the " + obj.word); return true;});

        try {
                connection.register_object<RemoteObjectIface> ("/org/freedesktop/Telepathy/Client/RemoteGame/OfferedObject", my_object);

                debug ("Object registered successfully");
        } catch (IOError e) {
            debug ("Could not register object");
        }
    }

    private void use_object (RemoteObject object)
    {
        stdout.printf ("I got a " + object.word);
        stdout.printf ("Using fetched objects.");
        object.do_move_remote ("bake");
        stdout.printf ("Hurting fetched objects.");
        try {
            object.remote_yell ();
        } catch (IOError e)
        {
            stdout.printf ("Couldn't hurt.. : %s", e.message);
        }
    }

    private void fetch_objects (DBusConnection conn, TelepathyGLib.DBusTubeChannel tube)
    {
        RemoteObject close_object = null;

        conn.get_proxy.begin<RemoteObjectIface> (null, "/org/freedesktop/Telepathy/Client/RemoteGame/OfferedObject", 0, null,
            (obj, res)=>{

                try {
                    close_object = (RemoteObject) conn.get_proxy.end<RemoteObjectIface> (res);
                    if (close_object != null)
                    {
                        use_object (close_object);
                    }
                }
                catch (IOError e)
                {
                   debug ("couldn't fetch the white player: %s\n", e.message);
                }
              }
            );
    }

    private void offer_tube (TelepathyGLib.DBusTubeChannel tube)
    {
        DBusConnection? connection = null;
        string say = "Yay!";
        Value say_val = say;

        HashTable<string, Value?>? tube_params = new HashTable<string, Value?> (str_hash, str_equal);
        tube_params.insert ("say", say_val);

        /* This hashtable is purely offerer defined and cast here is the secret gotcha */
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

                    return;
                }

                assert (connection != null);

                debug ("Offered tube opened.");
                connection.notify["closed"].connect (
                    (s, p)=>{
                        debug ("Connection to %s closed unexpectedly",
                            (tube as
                            TelepathyGLib.Channel).target_contact.identifier);
                    }
                );

                /* First load player objects. Then wait on them for creating Game */
                register_objects (connection, tube, "offerer");
                fetch_objects (connection, tube);
              }
            );
    }

    private void accept_tube (TelepathyGLib.DBusTubeChannel tube)
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
                        (tube as TelepathyGLib.Channel).target_contact.identifier, e.message);

                    return;
                }

                assert (connection != null);

                debug ("Accepted tube opened.");
                connection.notify["closed"].connect (
                    (s, p)=>{
                        debug ("Connection to %s closed unexpectedly",
                            (tube as
                            TelepathyGLib.Channel).target_contact.identifier);
                    }
                );

                /* First load player objects. Then wait on them for creating ChessGame */
                register_objects (connection, tube, "accepter");
                fetch_objects (connection, tube);
              }
            );
    }

    private void tube_invalidated_cb (TelepathyGLib.Proxy tube_channel,
        uint domain,
        int code,
        string message)
    {
        debug ("Tube has been invalidated: %s", message);
    }

    public void handle_channels (TelepathyGLib.SimpleHandler handler,
        TelepathyGLib.Account account,
        TelepathyGLib.Connection conn,
        List<TelepathyGLib.Channel> channel_bundle,
        List<TelepathyGLib.ChannelRequest> requests,
        int64 action_time,
        TelepathyGLib.HandleChannelsContext context)
    {
        Error error = new TelepathyGLib.Error.NOT_AVAILABLE ("No channel to be handled");

        debug ("Handling channels");

        foreach (TelepathyGLib.Channel tube_channel in channel_bundle)
        {
          if (! (tube_channel is TelepathyGLib.DBusTubeChannel))
            continue;
          if ((tube_channel as TelepathyGLib.DBusTubeChannel).service_name !=
              TelepathyGLib.CLIENT_BUS_NAME_BASE + "Remote.Game")
            continue;

          if (tube_channel.requested)
          {
              /* We created this channel. Make a tube offer and wait for approval */
              offer_tube (tube_channel as TelepathyGLib.DBusTubeChannel);
          }
          else
          {
              /* This is an incoming channel request */
              accept_tube (tube_channel as TelepathyGLib.DBusTubeChannel);
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

    private void ensure_legacy_channel_async (TelepathyGLib.Account account)
    {
        TelepathyGLib.AccountChannelRequest req;
        var ht = new HashTable<string, Variant> (str_hash, str_equal);

        req = new TelepathyGLib.AccountChannelRequest (
            account, ht,
            TelepathyGLib.user_action_time_from_x11 (0));

        req.set_request_property (
            TelepathyGLib.PROP_CHANNEL_CHANNEL_TYPE,
            new Variant.string (TelepathyGLib.IFACE_CHANNEL_TYPE_TUBES));
        req.set_request_property (
            TelepathyGLib.PROP_CHANNEL_TARGET_HANDLE_TYPE,
            new Variant.uint16 (TelepathyGLib.HandleType.CONTACT));
        req.set_request_property (
            TelepathyGLib.PROP_CHANNEL_TARGET_ID,
            new Variant.string (remote_id));

        req.ensure_channel_async.begin ("", null);
    }

    public override void activate ()
    {
    }

    public override void startup ()
    {
        base.startup ();

        string account_path = TelepathyGLib.ACCOUNT_OBJECT_PATH_BASE + my_account;
        TelepathyGLib.Account account = null;
        TelepathyGLib.AutomaticClientFactory factory = new TelepathyGLib.AutomaticClientFactory (null);

        var immutable_acc_props = new HashTable<string, Value?> (str_hash, str_equal);
        Quark[] features = {};
        features += TelepathyGLib.Account.get_feature_quark_connection ();
        factory.add_account_features (features);

        try {
            account = factory.ensure_account (account_path, immutable_acc_props);
        }
        catch (Error e)
        {
            stderr.printf ("Unable to create account: %s", e.message);
        }

        tp_handler =  new TelepathyGLib.SimpleHandler.with_am (
            TelepathyGLib.AccountManager.dup (),
            true,     /* Bypass approval */
            true,     /* Requests */
            "Remote.Game",
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
              TelepathyGLib.CLIENT_BUS_NAME_BASE + "Remote.Game");

        (tp_handler as TelepathyGLib.BaseClient).add_handler_filter (filter);

        try {
            (tp_handler as TelepathyGLib.BaseClient).register ();
        }
        catch (Error e) {
            debug ("Failed to register handler: %s", e.message);
        }

        debug ("Waiting for tube offer");

        /* Create a DBusTube Channel */

        /* Workaround for https://bugs.freedesktop.org/show_bug.cgi?id=47760 */
        ensure_legacy_channel_async (account);

        var ht = new HashTable<string, Variant> (str_hash, str_equal);

        var req = new TelepathyGLib.AccountChannelRequest (
            account, ht,
            TelepathyGLib.user_action_time_from_x11 (0));

        req.set_request_property (
            TelepathyGLib.PROP_CHANNEL_CHANNEL_TYPE,
            new Variant.string (TelepathyGLib.IFACE_CHANNEL_TYPE_DBUS_TUBE));
        req.set_request_property (
            TelepathyGLib.PROP_CHANNEL_TARGET_HANDLE_TYPE,
            new Variant.uint16 (TelepathyGLib.HandleType.CONTACT));
        req.set_request_property (
            TelepathyGLib.PROP_CHANNEL_TARGET_ID,
            new Variant.string (remote_id));
        req.set_request_property (
            TelepathyGLib.PROP_CHANNEL_TYPE_DBUS_TUBE_SERVICE_NAME,
            new Variant.string (service));

        bool channel_dispatched = false;
        req.create_channel_async.begin (service, null,
            (obj, res)=>{
                try
                {
                     channel_dispatched = req.create_channel_async.end (res);
                }
                catch (Error e)
                {
                     debug ("Failed to create channel: %s", e.message);
                }

                if (channel_dispatched)
                  debug ("DBus channel with %s successfully dispatched.",
                      remote_id);
                else
                  debug ("Unsuccessful in creating and dispatching DBus channel with %s.",
                      remote_id);
              }
            );

    }
}

class TestTubeApp
{
    public static int main (string[] args)
    {
        if (args.length < 3)
        {
            stdout.printf ("Usage: test-tube-app <account> <remote-player-id>\n");
            return 0;
        }

        RemoteGameHandler app = new RemoteGameHandler (args[1],  args[2]);
        var result = app.run ();

        return result;
    }
}
