import itertools
import _winreg as winreg

def _enum_values(hkey, sub_key):
    with winreg.OpenKey(hkey, sub_key) as key:
        for i in itertools.count():
            try:
                yield unicode(winreg.EnumValue(key, i)[1])
            except WindowsError:
                pass            # no more values

def serial_ports():
    sub_key = '\\'.join(['hardware', 'devicemap', 'serialcomm'])
    return list(_enum_values(winreg.HKEY_LOCAL_MACHINE, sub_key))

def serial_port_abs(serial_port):
    if serial_port.startswith(u'\\\\.\\'):
        return serial_port

    try:
        int(serial_port)
    except ValueError:
        pass
    else:
        return serial_port_abs(u'COM' + serial_port)

    return u'\\\\.\\' + serial_port
