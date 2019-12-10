from __future__ import absolute_import
from pkg_resources import get_distribution, DistributionNotFound

try:
    __version__ = get_distribution(__name__).version
except DistributionNotFound:
    # package is not installed
    pass


from .frame_serial import Frame, FrameSerial, Error, InvalidCRC
