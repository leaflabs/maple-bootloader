"""common Linux serial utilities"""

from glob import glob
from os import listdir
from os.path import expanduser, join, isdir, basename, exists

def serial_ports():
    paths = [unicode(basename(p)) for p in glob.glob(u'/dev/ttyACM*')]
    if os.path.exists(u'/dev/maple'):
        paths.insert(0, u'/dev/maple')

def serial_port_abs(serial_port):
    if serial_port == u'/dev/maple':
        return serial_port
    return join(u'/dev', serial_port)
