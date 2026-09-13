'use strict';

return L.view.extend({
    render: function () {
        let m, s, o;

        m = new L.form.Map('ps3netsrv', _('PS3 Net Server'),
            _('ps3netsrv allows you to stream games and ISOs over the network to your CFW PlayStation(R) 3 system.'));

        s = m.section(L.form.TypedSection, 'ps3netsrv', _('Server Settings'));
        s.anonymous = true;
        s.addremove = false;

        // Enabled
        o = s.option(L.form.Flag, 'enabled', _('Enabled'));
        o.rmempty = false;

        // User
        o = s.option(L.form.Value, 'user', _('Run as User'),
            _('The system user to run the service as (e.g., root).'));
        o.default = 'root';
        o.rmempty = false;

        // Directory
        o = s.option(L.form.Value, 'dir', _('Game Directory'),
            _('Path to the folder containing your PS3ISO, GAMES, etc.'));
        o.rmempty = false;
        o.datatype = 'directory';

        // Port
        o = s.option(L.form.Value, 'port', _('Port'),
            _('Network port to listen on (default: 38008).'));
        o.datatype = 'port';
        o.default = '38008';
        o.rmempty = false;

        // Whitelist
        o = s.option(L.form.Value, 'whitelist', _('IP Whitelist'),
            _('Optional: IP whitelist in x.x.x.x format (e.g., 192.168.1.*).'));
        o.placeholder = '*.*.*.*';
        o.rmempty = true;
        o.validate = function (section_id, value) {
            if (!value) return true;
            if (value.match(/^(\*|([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5]))\.(\*|([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5]))\.(\*|([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5]))\.(\*|([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5]))$/)) {
                return true;
            }
            return _('Invalid whitelist format. Expecting x.x.x.x (0-255 or *).');
        };

        return m.render();
    }
});
