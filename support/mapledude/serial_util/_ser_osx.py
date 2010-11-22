import glob
from os.path import join, basename

def serial_ports():
    return [unicode(basename(p)) for p in glob.glob('/dev/tty.usbmodem*')]

def serial_port_abs(serial_port):
    return join(u'/dev', serial_port)
