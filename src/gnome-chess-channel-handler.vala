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

class GnomeChessChannelHandler
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

