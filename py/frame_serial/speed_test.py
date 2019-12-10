from __future__ import absolute_import, print_function
from enum import IntEnum
import argparse
import timeit

import serial

from . import FrameSerial


class FrameType(IntEnum):
    CMD = 1
    CMD_RESP = 2
    DATA = 3


def send_cmd(frame_serial, cmd, timeout=1):
    print(cmd)
    raw_cmd = cmd.encode("ascii")
    frame_serial.send(FrameType.CMD, raw_cmd)

    ret = None
    for f in frame_serial.read_frames(timeout):
        if f.type == FrameType.CMD_RESP:
            ret = f
            break

    if ret:
        resp = ret.data.decode("ascii")
        print(resp)
        return resp
    else:
        raise ValueError("Did not receive packet back!")


def main():
    parser = argparse.ArgumentParser(description="Run a FrameSerial speed test")
    parser.add_argument(
        "--baud",
        type=int,
        default=115200,
        help="The baud rate of the specified serial port",
    )
    parser.add_argument(
        "--timeout", type=float, default=5.0, help="The test time (seconds)",
    )
    parser.add_argument("port", help="The serial port to use")
    parser.add_argument(
        "frequency", type=int, help="The frequency to send frames at (Hz)",
    )

    args = parser.parse_args()

    with serial.Serial(port=args.port, baudrate=args.baud, timeout=0.1) as port:
        conn = FrameSerial(port)
        if send_cmd(conn, "x") != "$NONE":
            raise ValueError("Invalid response")

        if not send_cmd(conn, "t{}".format(args.frequency)).startswith("$TRANSMIT"):
            raise ValueError("Invalid response")

        start = timeit.default_timer()
        recv_len = 0
        for f in conn.read_frames(args.timeout):
            if f.type != FrameType.DATA:
                raise ValueError("Only expected data frames!")

            recv_len += len(f.data)

            if (timeit.default_timer() - start) > args.timeout:
                break

        end = timeit.default_timer()

        if send_cmd(conn, "x", 3) != "$NONE":
            raise ValueError("Invalid response")

        elapsed_seconds = end - start
        speed_bytes_per_second = recv_len / elapsed_seconds
        speed_Mbps = speed_bytes_per_second * 8 / 1e6
        print(
            f"Received {recv_len} bytes in {elapsed_seconds} seconds: {speed_bytes_per_second:.2f} B/s ({speed_Mbps:.2f} Mbps)"
        )


if __name__ == "__main__":
    main()
