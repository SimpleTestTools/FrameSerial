from collections import namedtuple
import argparse
import struct
import timeit
import binascii

from cobs import cobs


uint16_t = struct.Struct("<H")
uint32_t = struct.Struct("<I")


class Error(Exception):
    "Base class for errors in this module"


class InvalidCRC(Error):
    "Frame has an invalid CRC value"


class InvalidLength(Error):
    "Frame has an invalid length"


class Frame(namedtuple("_frame", ["type", "data"])):
    @classmethod
    def from_bytes(cls, raw_pkt):
        if not raw_pkt:
            raise InvalidLength("Unexpected length: {}".format(len(raw_pkt)))

        if raw_pkt[-1] == 0:
            raw_pkt = raw_pkt[:-1]
        raw = cobs.decode(raw_pkt)

        if len(raw) < 8:
            raise InvalidLength("Need at least 4 header and 4 footer bytes")

        frm_crc32 = uint32_t.unpack(raw[-4:])[0]
        computed_crc32 = binascii.crc32(raw[:-4])
        if frm_crc32 != computed_crc32:
            raise InvalidCRC(
                "Frame CRC does not match! given {} != computed {}".format(
                    frm_crc32, computed_crc32
                )
            )

        frm_type = uint16_t.unpack(raw[0:2])[0]
        frm_len = uint16_t.unpack(raw[2:4])[0]
        frm_data = raw[4 : (4 + frm_len)]

        return cls(type=frm_type, data=frm_data)

    def to_bytes(self):
        frm_type = uint16_t.pack(self.type)
        frm_len = uint16_t.pack(len(self.data))

        raw = frm_type + frm_len + bytes(self.data)
        raw += uint32_t.pack(binascii.crc32(raw))

        pkt = cobs.encode(raw) + b"\0"

        return pkt


class FrameSerial:
    def __init__(self, port):
        self.port = port
        self._buf = bytearray()

    def send(self, type, data):
        frm = Frame(type=type, data=data)
        self.send_frame(frm)

    def send_frame(self, frame):
        raw = frame.to_bytes()
        # print("tx:", raw)
        self.port.write(raw)

    def pop_packet_or_fetch_data(self, fetch=True):
        split = self._buf.split(b"\0", maxsplit=1)

        if len(split) == 2:
            pkt, self._buf = split
            return pkt

        if fetch:
            self._buf = self.port.read(1024)
            return self.pop_packet_or_fetch_data(fetch=False)

        return None

    def read_frame(self):
        raw = self.pop_packet_or_fetch_data()
        try:
            f = Frame.from_bytes(raw)
            # print(f)
            return f
        except (cobs.DecodeError, InvalidCRC, InvalidLength):
            pass

        return None

    def read_frames(self, timeout=None):
        start = timeit.default_timer()
        while True:
            f = self.read_frame()
            if f:
                yield f
            if timeout is not None and ((timeit.default_timer() - start) > timeout):
                return StopIteration()
