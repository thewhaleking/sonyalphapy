"""
sonyalphapy — Python bindings for the Sony Camera Remote SDK v1.

Example usage::

    import sonyalphapy as sony

    sony.initialize()
    cameras = sony.enumerate_cameras()
    if not cameras:
        print("No cameras found")
    else:
        cam = cameras[0]
        print(cam.model)
        with cam:
            cam.connect()
            cam.capture()
    sony.shutdown()

Note:
    All bundled SDK dylibs are universal binaries (x86_64 + arm64) and run
    natively on both Intel and Apple Silicon Macs.
"""

from .camera import Camera, initialize, shutdown, sdk_version, enumerate_cameras
from ._callback import DeviceCallback
from ._sdk import DeviceProperty, ImageDataBlock
from . import enums
from .errors import SonySDKError, SonyConnectionError, SonyAPIError, SonyFileError

__all__ = [
    "Camera",
    "DeviceCallback",
    "DeviceProperty",
    "ImageDataBlock",
    "initialize",
    "shutdown",
    "sdk_version",
    "enumerate_cameras",
    "enums",
    "SonySDKError",
    "SonyConnectionError",
    "SonyAPIError",
    "SonyFileError",
]
