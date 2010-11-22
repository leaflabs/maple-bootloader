from __future__ import print_function

import platform
import sys

# supported platforms
OSX   = 'osx'
LINUX = 'linux'
WIN32 = 'win32'

def _die(msg):
    print(msg)
    sys.exit(1)

def _guess_os():
    s = platform.system()
    if s == 'Darwin':
        return OSX
    elif s == 'Linux':
        return LINUX
    elif s == 'Windows':
        # FIXME what to do about 64-bit Windows?  Whether or not
        # _ser_win32 will work is unknown to me [mbolivar].
        return WIN32

# runtime platform
OS = _guess_os()

# pull in platform-specific definitions
if OS == OSX:     from _ser_osx import *
elif OS == WINXP: from _ser_win32 import *
elif OS == LINUX: from _ser_linux import *
else:             die('unknown operating system: {0}'.format(OS))

# derived definitions
def serial_ports_abs():
    return map(serial_port_abs, serial_ports())
