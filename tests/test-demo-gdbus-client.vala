/* A simple test client to show how to do RMI over DBus */

[DBus (name = "org.example.Demo")]
interface Demo : Object {
    public abstract int ping (string msg) throws IOError;
    public abstract int ping_with_sender (string msg) throws IOError;
    public abstract int ping_with_signal (string msg) throws IOError;
    public signal void pong (int count, string msg);
}

void main () {
    /* Needed only if your client is listening to signals; you can omit it
 * otherwise */
    var loop = new MainLoop();

    /* Important: keep demo variable out of try/catch scope not lose signals! */
    Demo demo = null;

    try {
        demo = Bus.get_proxy_sync (BusType.SESSION, "org.example.Demo",
                                                    "/org/example/demo");

        /* Connecting to signal pong! */
        demo.pong.connect((c, m) => {
            stdout.printf ("Got pong %d for msg '%s'\n", c, m);
            loop.quit ();
        });

        int pong = demo.ping ("Hello from Vala");
        stdout.printf ("%d\n", pong);

        pong = demo.ping_with_sender ("Hello from Vala with sender");
        stdout.printf ("%d\n", pong);

        pong = demo.ping_with_signal ("Hello from Vala with signal");
        stdout.printf ("%d\n", pong);

    } catch (IOError e) {
        stderr.printf ("%s\n", e.message);
    }
    loop.run();
}

/* $ valac --pkg gio-2.0 gdbus-demo-client.vala */
