#!/usr/bin/env python

"""Executable module for interacting with the serial bootloader.
Doesn't presently support all of the avrdude command line options, but
uses a similar, STK-500-like communications protocol.

Requires the PySerial library and Python 2.6+.
"""

# This is the host side of the Maple Rev 5 bootloader.  See the
# bootloader documentation for information on the protocol.

from __future__ import print_function

import contextlib
import math
import optparse
import re
import sys
import subprocess
import time
from itertools import izip, cycle, chain

import serial

import serial_util

# Serial config
BAUDRATE      = 115200
BYTESIZE      = serial.EIGHTBITS
PARITY        = serial.PARITY_NONE
STOPBITS      = serial.STOPBITS_ONE
TIMEOUT       = 20              # FIXME
WRITE_TIMEOUT = 20              # FIXME
DSRDTR        = False
BOOTLOADER_TIMEOUT = 2
BOOTLOADER_DIE_TIMEOUT = BOOTLOADER_TIMEOUT  # seconds for comm to die
BOOTLOADER_LIVE_TIMEOUT = BOOTLOADER_TIMEOUT # seconds for comm to be reborn

USE_FAKE_SERIAL = True # debug with FakeSerial objects instead of Serial
RUN_LOCAL = True
HOSTNAME = 'localhost'
TCP_PORT = 5323

# Packet constants
PACKET_START = 0x1B
PACKET_TOKEN = 0x7F
PACKET_MAX_PAYLOAD = 1024       # bytes
LOCATION_FLASH = 0              # for JUMP_TO_USER
LOCATION_RAM = 1                # for JUMP_TO_USER
SEQUENCE_NUM_MAX = 0xFF

# Command values
GET_INFO     = 0
ERASE_PAGE   = 1
WRITE_BYTES  = 2
READ_BYTES   = 3
JUMP_TO_USER = 4
SOFT_RESET   = 5

def command_string(command):
    return {GET_INFO: 'GET_INFO',
            ERASE_PAGE: 'ERASE_PAGE',
            WRITE_BYTES: 'WRITE_BYTES',
            READ_BYTES: 'READ_BYTES',
            JUMP_TO_USER: 'JUMP_TO_USER',
            SOFT_RESET: 'SOFT_RESET'}[command]

# Verbosity
NORMAL = 0
VERBOSE = 1
NOISY = 2
VERBOSITY = NORMAL

# -- Verbosity-controlled printing -------------------------------------------#

def verbose(level=VERBOSE):
    return VERBOSITY >= level

def printv(*args, **kwargs):
    if 'level' in kwargs:
        level = kwargs['level']
        del kwargs['level']
    else:
        level = VERBOSE

    if verbose(level):
        print(*args, **kwargs)

# -- Programmatic uploading interface ----------------------------------------#

def upload_bin(prog_bin, port, memory_location=LOCATION_FLASH):
    """Upload a file in .bin format to a Maple over serial port.

    Raises or uses functions which may raise:
     - IOError: prog_bin cannot be read, Maple isn't cooperating, etc.

     - ValueError: memory_location isn't one of LOCATION_RAM or
       LOCATION_FLASH, binary is too big to fit in flash, some other
       reasons.

     - serial.SerialTimeoutException: a timeout occurs during serial
       communication

     - ResponseError: some response packet is malformed

     - ChecksumError: some packet's checksum is mismatched
    """
    with open(prog_bin, 'rb') as f_in:
        prog_bytes = bytearray(f_in.read())

    if memory_location not in [LOCATION_FLASH, LOCATION_RAM]:
        raise ValueError('memory location must be LOCATION_FLASH or ' +
                         'LOCATION_RAM')

    printv('Opening port {0}'.format(port))
    with contextlib.closing(_open_serial(port)) as comm:
        seqnums = cycle(range(256))

        printv('Querying board info')
        info = get_info(comm, seqnums.next()).fields

        printv('Uploading file', prog_bin,
              '({0}KB)'.format(round(len(prog_bytes) / 1024.0)))
        if memory_location == LOCATION_FLASH:
            upload_flash(comm, seqnums, info, prog_bytes)
        else:
            upload_ram(comm, seqnums, info, prog_bytes)

        printv('Jumping to user code')
        # Once we tell the bootloader to jump to user code, it will
        # intentionally delay a little bit before doing so, in order
        # to allow the comm port to close
        if not jump_to_user(comm, seqnums.next(), location).success:
            raise IOError('failed jumping to user code')

def upload_flash(comm, seqnums, info, prog_bytes):
    nbytes = len(prog_bytes)
    if nbytes > info.available_flash:
        e = 'program is too large ({0}B) to fit in available flash ({1}B)'
        raise ValueError(e.format(nbytes, info.available_flash))

    printv('Uploading using flash target.')

    printv('Clearing any RAM-resident program')
    clear_ram_program(comm, seqnums, info)

    printv('Clearing flash pages before upload')
    clear_pages(comm, seqnums, info, nbytes)

    printv('Uploading program')
    addr = info.start_addr_flash
    for chunk in packet_chunks(prog_bytes):
        if not write_bytes(comm, seqnums.next(), addr, chunk).success:
            e = 'failed writing program to flash. wrote {0}/{1} bytes.'
            raise IOError(e.format(addr - info.start_addr_flash, nbytes))
        addr += len(chunk)

    printv('Done uploading.')

def clear_pages(comm, seqnums, board_info, nbytes):
    start = board_info.start_addr_flash
    page_size = board_info.flash_page_size
    npages = int(math.ceil(float(nbytes) / page_size))
    for addr in range(start, start + npages * page_size, page_size):
        if not erase_page(comm, seqnums.next(), addr).success:
            e = "failed erasing page containing address 0x{0:X}"
            raise IOError(e.format(addr))

def upload_ram(comm, seqnums, prog_bytes, info):
    nbytes = len(prog_bytes)
    if nbytes > info.available_ram:
        e = 'program is too large ({0}B) to fit in available RAM ({1}B)'
        raise ValueError(e.format(nbytes, info.available_ram))

    printv('Uploading using RAM target')
    addr = info.start_addr_ram
    for chunk in packet_chunks(prog_bytes):
        if not write_bytes(comm, seqnums.next(), addr, chunk).success:
            e = 'failed writing program to flash. wrote {0}/{1} bytes.'
            raise IOError(e.format(addr - info.start_addr_flash, nbytes))
        addr += len(chunk)

    printv('Done uploading.')

def clear_ram_program(comm, seqnums, info):
    """Clears some bytes in RAM so that the bootloader won't think a
    program is resident there."""
    # the starting address in RAM is the value to use as the stack
    # pointer.  set this to zero so the bootloader doesn't think
    # there's a program in RAM (say, after a reset).
    if not write_bytes(comm, seqnums.next(),
                       info.start_addr_ram, bytearray(4)).success:
        raise IOError('failed clearing SP')

def _reset_board(port):
    if True:
        raise RuntimeError('BROKEN! DANGEROUS! DO NOT USE!')
    port_abs = serial_util.serial_port_abs(port)

    # first, grab a serial connection to the board.
    printv('Opening serial port', level=NOISY)
    comm = _open_serial(port_abs)

    try:
        # tell the board's bootloader to do a hard reset. the
        # bootloader will delay for a short time before actually
        # performing the reset.
        printv('Telling the bootloader to reset', level=NOISY)
        comm.setRTS(1)
        time.sleep(0.01)
        comm.setDTR(1)
        time.sleep(0.01)
        comm.setDTR(0)
        time.sleep(0.01)
        comm.write("1EAF")
    finally:
        # the bootloader delay allows us to close the comm port on the
        # host side, so we don't leak any file descriptors, etc.  kind
        # of hackish, but it works ok.
        printv('Closing comm port')
        comm.close()

    # now the totally hacky part.  we don't want to re-open the comm
    # until after it's gone down and come back up.  so keep listing
    # available ports until ours disappears (or we time out)...
    start = time.time()
    while port_abs in serial_util.serial_ports_abs():
        printv('Waiting for death...', level=NOISY)
        if time.time() - start > BOOTLOADER_DIE_TIMEOUT:
            raise serial.SerialTimeoutException("board death")
    # ... and reappears again (or we time out).
    start = time.time()
    while port_abs not in serial_util.serial_ports_abs():
        printv('Waiting for rebirth...', level=NOISY)
        if time.time() - start > BOOTLOADER_LIVE_TIMEOUT:
            raise serial.SerialTimeoutException("board resurrection")

    # ok, we're back.  pass the caller a new Serial.
    return _open_serial(port_abs)

def _open_serial(port):
    if USE_FAKE_SERIAL:
        return FakeSerial()

    try:
        return serial.Serial(port, baudrate=BAUDRATE, bytesize=BYTESIZE,
                             parity=PARITY, stopbits=STOPBITS, timeout=TIMEOUT,
                             writeTimeout=WRITE_TIMEOUT, dsrdtr=DSRDTR)
    except serial.SerialException:
        raise ValueError('cannot open port {0}'.format(port))

# -- utility functions -------------------------------------------------------#

def packet_checksum(bs):
    """packet_checksum(iterable_of_ints) -> bytearray checksum

    Each int must be in range(256). Returned bytearray has length 4."""
    csum = 0
    csum_accum = 0
    # pull bytes out four at a time into csum_accum, treated as a
    # big-endian 32 bit int.  each time we read a word, xor into csum
    # and keep going.
    for byte, shift in izip(bs, cycle([24, 16, 8, 0])):
        if not (0 <= byte <= 256):
            raise ValueError('byte {0} not in range(256)'.format(byte))
        csum_accum |= byte << shift
        if shift == 0:
            csum ^= csum_accum
            csum_accum = 0
    # if not aligned on a 4-byte boundary, treat as if padded with zero bytes.
    if shift != 0:
        csum ^= csum_accum

    return int_to_bytearray(csum, 4)

def int_to_bytearray(n, nbytes):
    """Puts nbytes of int n into a bytearray, in network byte order."""
    ret = bytearray()
    while nbytes > 0:
        ret.append((n >> (nbytes - 1) * 8) & 0xFF)
        nbytes -= 1
    return ret

def bytearray_to_int(barr):
    """Converts byte array barr, interpreted as an MSB int, into a
    len(barr)-byte int in host-endianness."""
    ret = 0
    nbytes = len(barr)
    for i in range(nbytes):
        ret |= barr[i] << ((nbytes - 1 - i) * 8)
    return ret

def check_bytearray(barr, nbytes, name='', word_aligned_as_int=False):
    if len(barr) != nbytes:
        e = '{0}={1} has len={2}, must be {3}'
        e.format(name if name else 'bytearray', barr, len(barr), nbytes)
        raise ValueError(e)
    if word_aligned_as_int:
        barr_int = bytearray_to_int(barr)
        if barr_int % 4 != 0:
            e = '{0}={1} as integer is {2}; must be a multiple of 4'
            e.format(name if name else 'bytearray', barr, barr_int)
            raise ValueError(e)

def packet_chunks(barr):
    """Iterator over sub-slices of the argument bytearray, each of
    which fits in a packet payload.  The slices are not backed by the
    argument bytearray."""
    chunk_size = int(PACKET_MAX_PAYLOAD / 2)
    nbytes = len(barr)
    for i in xrange(0, nbytes, chunk_size):
        yield barr[i:i+chunk_size]

# -- Query -------------------------------------------------------------------#

class Query(object):
    """Query superclass.  A Query knows how to send a packet over
    serial, using Query.transmit_over_serial(Serial)."""

    def __init__(self, command, sequence_num, message_size, message_body):
        """Query(command, sequence_num, message_size, message_body)

        - command: bootloader command number
        - sequence_num: from 0--255
        - message_size: in bytes, currently 1KB max.  must be positive.
        - message_body: iterable of ints in range(256), to be sent
          as bytes."""
        self.command = command

        if not (0 <= sequence_num <= 255):
            err = 'sequence num {0} not in range(256)'.format(sequence_num)
            raise ValueError(err)
        self.sequence_num = sequence_num

        if not (0 < message_size <= PACKET_MAX_PAYLOAD):
            err = 'message_size={0} not in allowed 1B--{1}B range'
            raise ValueError(err.format(message_size, PACKET_MAX_PAYLOAD))
        self.message_size = message_size
        ms_msb = (message_size >> 8) & 0xFF
        ms_lsb = message_size & 0xFF

        # pack everything except the checksum into a bytearray
        bs = bytearray(message_size + 5)
        for i, b in enumerate(chain([PACKET_START, self.sequence_num,
                                     ms_msb, ms_lsb, PACKET_TOKEN],
                                    message_body)):
            if not (0 <= b <= 255):
                if i == 0: loc = 'START'
                elif i == 1: loc = 'SEQUENCE_NUM'
                elif i == 2: loc = 'MESSAGE_SIZE MSB'
                elif i == 3: loc = 'MESSAGE_SIZE LSB'
                elif i == 4: loc = 'TOKEN'
                else: loc = 'index {0} of MESSAGE_BODY'.format(i - 5)

                if i == 5 and b != self.command:
                    raise ValueError(('message body command=0x{0:02X}, should '
                                      'be {0:02X}').format(b, self.command))

                raise ValueError('bad byte 0x{0:X} in {1}'.format(b, loc))
            bs[i] = b
        # and tack on a checksum
        bs.extend(packet_checksum(bs))

        self.bytes = bs
        self.size = len(bs)

    def transmit_over_serial(self, serial):
        """Transmits this packet's data over the given Serial object.
        Exceptions from calls to serial.write() propagate upwards."""
        # we could just say serial.write(self.bytes), but calculating
        # the timeout gets trickier etc, so just do it byte-by-byte
        if verbose(NOISY):
            noisy_head = '---------- Sending command ----------'
            print(noisy_head, self.hex_bytes_str(), sep='\n')
        for b in self:
            n = serial.write(chr(b))
            if n != 1: raise IOError("couldn't write byte")
        printv('-' * len(noisy_head), level=NOISY)

    def hex_bytes_str(self):
        return ''.join('{0:02X}'.format(b) for b in self)

    def _short_hex_str(self):
        hstr = self.hex_bytes_str()
        if len(hstr > 11): hstr = hstr[:4] + '...' + hstr[:-4]
        return hstr

    def __iter__(self):
        """Iterator of the packet's bytes as ints in range(256)."""
        return iter(self.bytes)

    def __str__(self):
        return "<Query object '" + self._short_hex_str() + "'>"

    def __repr__(self):
        return str(self)


# -- Response ----------------------------------------------------------------#

class Response(object):

    def __init__(self, command, raw, payload, fields):
        """Response(raw, payload, fields)

        raw: the response packet's raw bytes.
        payload: a map from field names to raw bytes in network order.
        fields: map from field names to integers in host endianness."""
        self.command = command
        self.raw = raw
        self.payload = payload
        self.fields = fields

    def pprint(self):
        print('{0} Response fields:'.format(self.command))
        for k, v in fields.iteritems():
            print('\t{0}={1:X}'.format(k, v))

class ResponseError(RuntimeError):
    pass

class ChecksumError(ResponseError):
    pass

class ResponseParser(object):
    """Used to parse raw bootloader response bytes into a useful form.
    Given a specification for the format of the response packet,
    provides a parse() function for creating a Response object from a
    file-like."""

    # to consume enough bytes for a field to fill out MESSAGE_SIZE.
    # at most one field can have this size.
    FLEX = object()

    def __init__(self, command, sequence_num, payload_spec):
        """ResponseParser(command, sequence_num, payload_spec)

        data_spec must be an iterable of (field, size) pairs
        specifying packet payload, not including command.  size may be
        either a positive integer or (for at most one field)
        ResponseParser.FLEX.  If a field's size is given as FLEX, it will
        consume as many bytes as necessary to consume all MESSAGE_SIZE
        of the packet's MESSAGE_BODY bytes."""
        self.command = command
        self.sequence_num = sequence_num
        self.payload_spec = list(payload_spec)

        flex_field = None
        fixed_size = 0 # number of fixed payload bytes, not including command
        for field, size in payload_spec:
            if size is ResponseParser.FLEX:
                if flex_field is not None:
                    e = 'too many FLEX fields: {0}, {1}'
                    raise ValueError(e.format(flex_field, field))
                flex_field = field
            else:
                fixed_size += size
        self.flex_field = flex_field

        # cmd + payload
        body_size = 1 + fixed_size

        if flex_field is None:
            if body_size > PACKET_MAX_PAYLOAD:
                e = 'given MESSAGE_BODY size {0} exceeds maximum of {1}'
                raise ValueError(e.format(body_size, PACKET_MAX_PAYLOAD))
            self.size = body_size
        else:
            if body_size > PACKET_MAX_PAYLOAD:
                e = "non-FLEX fields' total size {0} exceeds maximum of {1}"
                raise ValueError(e.format(body_size, PACKET_MAX_PAYLOAD))
            self.fixed_size = body_size

    def parse(self, filelike):
        """Parse a response from a file-like object (it just needs to
        support read(nbytes)), returning a Response.

        Will raise ResponseError if the response is not properly
        formatted.  As a special case, ChecksumError will be raised if
        the checksum differs from what is expected."""
        if verbose(NOISY):
            noisy_head = '-------- Parsing {0} response --------'
            print(noisy_head.format(command_string(self.command)))

        barr = bytearray()

        # packet header (START, SEQUENCE_NUM, etc.)
        printv('Parsing packet header', level=NOISY)
        barr += self._get_byte(filelike, PACKET_START, 'START')
        barr += self._get_byte(filelike, self.sequence_num, 'SEQUENCE_NUM')

        msg_size_msb = self._get_byte(filelike, name='MESSAGE_SIZE[0]')
        msg_size_lsb = self._get_byte(filelike, name='MESSAGE_SIZE[1]')
        msg_size_barr = msg_size_msb + msg_size_lsb
        barr += msg_size_barr

        msg_size = bytearray_to_int(msg_size_barr)
        if msg_size > PACKET_MAX_PAYLOAD:
            e = 'MESSAGE_SIZE {0} exceeds maximum of {1}'
            raise ResponseError(e.format(msg_size, PACKET_MAX_PAYLOAD))
        elif hasattr(self, 'size'):
            if msg_size != self.size:
                self._err('MESSAGE_SIZE', msg_size, self.size)
        else:
            assert hasattr(self, 'fixed_size'), "can't happen"
            if msg_size < self.fixed_size:
                e = 'MESSAGE_SIZE {0} is less than total size of ' + \
                    'non-flex fields, {1}'
                raise ResponseError(e.format(msg_size, self.fixed_size))
            flex_size = msg_size - self.fixed_size

        barr += self._get_byte(filelike, PACKET_TOKEN, 'TOKEN')
        printv('Done parsing packet header', level=NOISY)

        # MESSAGE_BODY
        printv('Parsing message body', level=NOISY)
        barr += self._get_byte(filelike, self.command, 'COMMAND')
        payload = {}
        for field, size in self.payload_spec:
            if size is ResponseParser.FLEX:
                size = flex_size
                if size == 0:
                    payload[field] = bytearray(0)
                    continue

            buf = bytearray(size)
            for i in xrange(size):
                n = 'field {0} byte {1}'.format(field, i)
                b = self._get_byte(filelike, name=n)
                assert len(b) == 1, "can't happen"
                buf[i] = b[0]
            payload[field] = buf
            barr.extend(buf)
        printv('Done parsing message body', level=NOISY)

        # CHECKSUM
        printv('Parsing checksum and comparing with expected', level=NOISY)
        expected_checksum = packet_checksum(barr)
        csum = bytearray(4)
        for i in range(4):
                csum[i] = self._get_byte(filelike)[0]
        if csum != expected_checksum:
            e = 'got checksum {0:X}, expected {1:X}. packet data:\n[{2}]'
            raise ChecksumError(e.format(bytearray_to_int(csum),
                                         bytearray_to_int(expected_checksum),
                                         ', '.join(['0x{0:02X}'.format(b) 
                                                    for b in barr])))
        barr += csum
        printv('Done checksumming, things look good', level=NOISY)

        # everything looks good
        fields = object()
        for f, b in payload.iteritems():
            setattr(fields, f, bytearray_to_int(b))
        response = Response(self.command, barr, payload, fields)

        if verbose(NOISY):
            response.pprint()
            print('-' * len(noisy_head))

        return response

    def _get_byte(self, filelike, expected=None, name=None):
        """Pull a byte out of file-like object `filelike'. If
        `expected' is not None, it should be an int in range(256) the
        byte should equal.  ResponseError is raised if that is the
        case and there's a mismatch; `name' is a name for the byte
        (for debugging)."""
        string = filelike.read(size=1)
        if string: printv('read: {0:02X}'.format(ord(string)), level=NOISY)
        barr = bytearray(string)
        if len(barr) != 1:
            self._err(name, received='nothing')

        byte = barr[0]
        if expected is not None and byte != expected:
            self._err(name, byte, expected)

        return barr

    def _err(self, value=None, received=None, expected=None):
        val = 'error on {0}'.format(value) if value is not None else ''
        rec = 'received {0}'.format(received) if received is not None else ''
        exp = 'expected {0}'.format(expected) if expected is not None else ''

        if exp and not rec: rec = 'got nothing'

        exprec = ', '.join(filter(None, [exp, rec]))
        msg = ': '.join(filter(None, [val, exprec]))

        raise ResponseError(msg)

# -- Serial packet transaction -----------------------------------------------#

def call_and_response(query, response_parser, comm):
    query.transmit_over_serial(comm)
    return response_parser.parse(comm)

# -- GET_INFO ----------------------------------------------------------------#

def get_info(comm, sequence_num):
    printv('get_info(sequence_num={0})'.format(sequence_num), level=NOISY)
    return call_and_response(GetInfoQuery(sequence_num),
                             GetInfoResponseParser(sequence_num),
                             comm)

class GetInfoQuery(Query):

    def __init__(self, sequence_num):
        """GetInfoQuery(sequence_num) -> GET_INFO query

        >>> q = GetInfoQuery(127)
        >>> q.hex_bytes_str()
        '1B7F00017F00647F0001'
        >>> bytearray.fromhex(unicode(q.hex_bytes_str())) == q.bytes
        True"""
        Query.__init__(self, GET_INFO, sequence_num, 1, (GET_INFO,))

    def __str__(self):
        return 'GetInfoQuery({0})'.format(self.sequence_num)

class GetInfoResponseParser(ResponseParser):

    def __init__(self, sequence_num):
        ResponseParser.__init__(self, GET_INFO, sequence_num,
                                [('endianness', 1),
                                 ('available_ram', 4),
                                 ('available_flash', 4),
                                 ('flash_page_size', 2),
                                 ('start_addr_flash', 4),
                                 ('start_addr_ram', 4),
                                 ('bootloader_ver', 4)])

# -- ERASE_PAGE --------------------------------------------------------------#

def erase_page(comm, sequence_num, address):
    printv('erase_page(sequence_num={0}, address={1})'.format(
            sequence_num, address), level=NOISY)
    return call_and_response(ErasePageQuery(sequence_num, address),
                             ErasePageResponseParser(sequence_num),
                             comm)

class ErasePageQuery(Query):

    def __init__(self, sequence_num, address):
        """ErasePageQuery(sequence_num, address)

        `address' must be an iterable that returns the 4 bytes of the
        desired address in network order.

        >>> q = ErasePageQuery(80, [0x12, 0x51, 0xA8, 0xF3])
        >>> q.hex_bytes_str()
        '1B5000057F011251A8F3CCA21254'
        >>> bytearray.fromhex(unicode(q.hex_bytes_str())) == q.bytes
        True"""
        addr = bytearray(address)
        check_bytearray(addr, 4, name='address')
        Query.__init__(self, ERASE_PAGE, sequence_num, 5,
                       chain((ERASE_PAGE,), addr))
        self.address = addr

    def __str__(self):
        return 'ErasePageQuery({0}, {1})'.format(self.sequence_num,
                                                 self.address)

class ErasePageResponseParser(ResponseParser):

    def __init__(self, sequence_num):
        ResponseParser.__init__(self, ERASE_PAGE, sequence_num,
                                [('success', 1)])

# -- WRITE_BYTES -------------------------------------------------------------#

def write_bytes(comm, sequence_num, address, data):
    printv('write_bytes(sequence_num={0}, address={1}, data={2})'.format(
            sequence_num, address, repr(data)), level=NOISY)
    return call_and_response(WriteBytesQuery(sequence_num, address, data),
                             WriteBytesResponseParser(sequence_num),
                             comm)

class WriteBytesQuery(Query):

    def __init__(self, sequence_num, starting_address, data):
        addr = bytearray(starting_address)
        check_bytearray(addr, 4, name='address')
        data = bytearray(data)
        Query.__init__(self, WRITE_BYTES, sequence_num,
                       1 + len(addr) + len(data),
                       chain((WRITE_BYTES,), addr, data))
        self.address = addr
        self.data = data

    def __str__(self):
        s = 'WriteBytesQuery({0}, {1}, {2})'
        return s.format(self.sequence_num, self.starting_address, self.data)

class WriteBytesResponseParser(ResponseParser):

    def __init__(self, sequence_num):
        ResponseParser.__init__(self, WRITE_BYTES, sequence_num,
                                [('success', 1)])

# -- READ_BYTES --------------------------------------------------------------#

def read_bytes(comm, sequence_num, address, length):
    printv('read_bytes(sequence_num={0}, address={1}, length={2})'.format(
            sequence_num, address, length), level=NOISY)
    return call_and_response(ReadBytesQuery(sequence_num, address, length),
                             ReadBytesResponseParser(sequence_num),
                             comm)

class ReadBytesQuery(Query):

    def __init__(self, sequence_num, address, length):
        addr = bytearray(address)
        check_bytearray(addr, 4, name='address', word_aligned_as_int=True)

        length = bytearray(length)
        check_bytearray(length, 2, name='length', word_aligned_as_int=True)

        Query.__init__(self, READ_BYTES,
                       sequence_num, 1 + len(addr) + len(length),
                       chain((READ_BYTES,), addr, length))
        self.address = addr
        self.length = length

class ReadBytesResponseParser(ResponseParser):

    def __init__(self, sequence_num):
        ResponseParser.__init__(self, READ_BYTES, sequence_num,
                                [('starting_address', 4),
                                 ('data', ResponseParser.FLEX)])

# -- JUMP_TO_USER ------------------------------------------------------------#

def jump_to_user(comm, sequence_num, location):
    printv('jump_to_user(sequence_num={0}, location={1})'.format(
            sequence_num, 'FLASH' if location == LOCATION_FLASH else 'RAM'),
           level=NOISY)
    return call_and_response(JumpToUserQuery(sequence_num, location),
                             JumpToUserResponseParser(sequence_num),
                             comm)

class JumpToUserQuery(Query):

    def __init__(self, sequence_num, location):
        location = bytearray(location)
        check_bytearray(location, 1, name='location')
        location_int = bytearray_to_int(location, 1)
        if not (location_int == 0 or location_int == 1):
            e = 'location {0} as integer is {1}, must be either ' + \
                'LOCATION_FLASH=0 or LOCATION_RAM=1'
            raise ValueError(e.format(location, location_int))

        Query.__init__(self, JUMP_TO_USER, sequence_num, 1 + len(location),
                       chain((JUMP_TO_USER,), location))

class JumpToUserResponseParser(ResponseParser):

    def __init__(self, sequence_num):
        ResponseParser.__init__(self, JUMP_TO_USER, sequence_num,
                                [('success', 1)])

# -- SOFT_RESET --------------------------------------------------------------#

def soft_reset(comm, sequence_num):
    printv('soft_reset(sequence_num={0})'.format(sequence_num), level=NOISY)
    return call_and_response(SoftResetQuery(sequence_num),
                             SoftResetResponseParser(sequence_num),
                             comm)

class SoftResetQuery(Query):

    def __init__(self, sequence_num):
        Query.__init__(self, SOFT_RESET, sequence_num, 1, (SOFT_RESET,))

class SoftResetResponseParser(ResponseParser):

    def __init__(self, sequence_num):
        ResponseParser.__init__(self, SOFT_RESET, sequence_num,
                                [('success', 1)])

# -- FakeSerial --------------------------------------------------------------#

import socket
class FakeSerial(object):       # for debugging

    def __init__(self):
        global RUN_LOCAL
        global HOSTNAME
        global TCP_PORT

        if (RUN_LOCAL):
            if (HOSTNAME != 'localhost'):
                print("Will not spawn local server since the host is remote")
                RUN_LOCAL = False
            else:
                self.child = subprocess.Popen(['../../test/sp_test'],
                                              stdin=subprocess.PIPE,
                                              stdout=subprocess.PIPE)
        # woefully under error handled socket interface
        self.sock = socket.socket(
            socket.AF_INET, socket.SOCK_STREAM)
        
        print("hostname,port %s %i" % (HOSTNAME,TCP_PORT))
        self.sock.connect((HOSTNAME,TCP_PORT))

    def write(self, data):
        bytesOut = 0;
        while bytesOut < len(data):
            newBytes = self.sock.send(data[bytesOut:])
            if (newBytes == 0):
                raise Exception("Connection to host lost unexpectedly during write")
            bytesOut = bytesOut + newBytes

        return bytesOut

    def read(self, size=1):
        data = ''
        while len(data) < size:
            newBytes = self.sock.recv(size-len(data))
            if newBytes == '':
                raise Exception("Connection to host lost unexpectedly during read")
            data += newBytes
        return data

    def close(self):
        self.sock.close()

        if (RUN_LOCAL):
            sigterm_timeout = 3.0
            self.child.terminate()
            start = time.time()
            while time.time() - start < sigterm_timeout:
                if self.child.returncode is not None:
                    break
            else:
                self.child.kill()

# -- Command-line interface --------------------------------------------------#

opts, args = None, None


def main():
    parser = optparse.OptionParser(usage='usage: %prog [options]')
    parser.add_option('-P', '--port', default=None,
                      help='Connection port (serial only).')
    parser.add_option('-t', '--test', action='store_true', default=False,
                      help=('Run tests. Unless -v is specified, only ' +
                            'failures are reported.'))
    parser.add_option('-U', '--upload',
                      help='(flash|ram):(r|w|v):<program.bin>')
    parser.add_option('-v', '--verbose', action='count',
                      help='Enable verbose output; -v -v for more.')
    parser.add_option('-H', '--hostname', default='localhost',
                      help='Specify the IP or hostname of the remote client. If equal to localhost, then the server will be auto-spawned')
    parser.add_option('-p', '--tcp-port', default=5323,
                      help='Specify the port of the remote client')
    parser.add_option('-S', '--run-local-server', default=True,
                      help='The program will spawn its own test server. Must have hostname set to localhost')

    # TODO lose this ugliness
    global opts
    global args
    opts, args = parser.parse_args()

    global VERBOSITY
    VERBOSITY = min(opts.verbose, NOISY)

    def usage(msg, fatal=True):
        if fatal: print ('Error: ', end='')
        print(msg)
        parser.print_help()
        if fatal: sys.exit(1)

    if opts.test:
        import doctest
        doctest.testmod(verbose=verbose(level=VERBOSE))

    printv('Noisy output enabled', level=NOISY)

    global RUN_LOCAL
    global HOSTNAME
    global TCP_PORT
    if opts.run_local_server:
        RUN_LOCAL = opts.run_local_server
    if opts.hostname:
        HOSTNAME = opts.hostname
    if opts.tcp_port:
        TCP_PORT = opts.tcp_port

    if opts.upload:
        match = re.match('(?P<memtype>flash|ram):(?P<rwv>r|w|v):(?P<file>.*)',
                         opts.upload)
        if not match:
            usage('malformed -U value.')

        if opts.port is None:
            usage('you must specify a connection port.')
            sys.exit(1)

        if match.group('memtype') == 'flash':
            location = LOCATION_FLASH
        else:
            location = LOCATION_RAM

        rwv = match.group('rwv')
        if rwv != 'w':          # FIXME
            e = 'used {0} in argument to -U; only "w" is currently supported.'
            usage(e.format(rwv))

        program = match.group('file')

        if rwv == 'r':          # TODO
            pass
        elif rwv == 'w':
            upload_bin(program, opts.port, location)
        else: # TODO rwv == 'v'
            pass

if __name__ == '__main__':
    main()
