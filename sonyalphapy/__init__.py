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
    The underlying SDK library (libCr_Core.dylib) is x86_64 only.
    On Apple Silicon (M1/M2/M3/M4) run Python under Rosetta 2.
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
