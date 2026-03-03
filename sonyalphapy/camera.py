"""
High-level Python API for Sony Alpha cameras via CrSDK v1.

Typical usage::

    import sonyalphapy as sony

    sony.initialize()
    cameras = sony.enumerate_cameras()
    cam = cameras[0]
    cam.connect()
    cam.capture()
    cam.disconnect()
    sony.shutdown()
"""

import ctypes
import struct
import sys
import threading
import time

from ._sdk import (
    get_lib,
    DeviceProperty,
    ImageDataBlock,
    _dp_ctor,
    _dp_dtor,
    _dp_GetCode,
    _dp_SetCode,
    _dp_GetCurrentValue,
    _dp_SetCurrentValue,
    _dp_GetValueType,
    _dp_SetValueType,
    _dp_IsGetEnable,
    _dp_IsSetEnable,
    _dp_GetValues,
    _dp_GetValueSize,
    _DP_BUF_SIZE,
)
from ._vcall import EnumCameraInfo, CameraObjectInfo
from ._callback import DeviceCallback
from .errors import check_error, SonyConnectionError
from .enums import CommandId, CommandParam, DataType, SdkControlMode, ReconnectingSet


# ---------------------------------------------------------------------------
# SDK lifecycle
# ---------------------------------------------------------------------------


def initialize(log_type: int = 0) -> None:
    """Initialize the Sony Camera Remote SDK.

    Must be called once before any other operation.

    Args:
        log_type: SDK logging level (0 = off).

    Raises:
        RuntimeError: if the SDK fails to initialize.
    """
    lib = get_lib()
    ok = lib.Init(log_type)
    if not ok:
        raise RuntimeError("Failed to initialize Sony Camera Remote SDK.")


def shutdown() -> None:
    """Release the Sony Camera Remote SDK.

    Call once when your application exits.
    """
    get_lib().Release()


def sdk_version() -> str:
    """Return the SDK version string, e.g. ``'1.8.00'``."""
    v = get_lib().GetSDKVersion()
    major = (v >> 24) & 0xFF
    minor = (v >> 16) & 0xFF
    patch = (v >> 8) & 0xFF
    return f"{major}.{minor}.{patch:02d}"


# ---------------------------------------------------------------------------
# Camera enumeration
# ---------------------------------------------------------------------------


def enumerate_cameras(timeout_sec: int = 3) -> list["Camera"]:
    """Discover cameras connected via USB or network.

    Args:
        timeout_sec: seconds to wait for camera discovery (default 3).

    Returns:
        List of :class:`Camera` instances (may be empty).
    """
    lib = get_lib()
    enum_ptr = ctypes.c_void_p(0)
    err = lib.EnumCameraObjects(ctypes.byref(enum_ptr), timeout_sec)
    check_error(err, "EnumCameraObjects")

    if not enum_ptr.value:
        return []

    cameras = []
    # Do NOT release enum_info here — ICrCameraObjectInfo* pointers returned by
    # GetCameraObjectInfo() are internal to the enum and become dangling once the
    # enum is released.  Each Camera holds a reference to keep the enum alive
    # until Connect() completes, after which it drops it.
    enum_info = EnumCameraInfo(enum_ptr.value)
    try:
        for i in range(enum_info.get_count()):
            info = enum_info.get_camera_object_info(i)
            cameras.append(Camera(info, index=i, _enum_ref=enum_info))
    except Exception:
        enum_info.release()
        raise
    return cameras


# ---------------------------------------------------------------------------
# macOS run-loop helper
# ---------------------------------------------------------------------------


def _wait_for_event_pumping_runloop(event: threading.Event, timeout: float) -> bool:
    """Wait for *event* while keeping the CoreFoundation run loop alive.

    On macOS the Sony SDK may dispatch ``OnConnected`` through GCD's main
    queue.  GCD main-queue blocks only execute when the main thread's CF run
    loop is spinning.  A plain ``threading.Event.wait()`` blocks the main
    thread without pumping the run loop, so the callback never fires.

    We work around this by repeatedly calling ``CFRunLoopRunInMode`` for short
    intervals (50 ms), checking the event between each call.
    """
    if sys.platform != "darwin":
        return event.wait(timeout=timeout)
    try:
        CF = ctypes.CDLL(
            "/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation"
        )
        CF.CFRunLoopRunInMode.restype = ctypes.c_int32
        CF.CFRunLoopRunInMode.argtypes = [
            ctypes.c_void_p,  # mode (CFStringRef)
            ctypes.c_double,  # seconds
            ctypes.c_bool,  # returnAfterSourceHandled
        ]
        mode = ctypes.c_void_p.in_dll(CF, "kCFRunLoopDefaultMode")
        deadline = time.monotonic() + timeout
        while not event.is_set():
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                return False
            CF.CFRunLoopRunInMode(mode, min(0.05, remaining), True)
        return True
    except Exception:
        return event.wait(timeout=timeout)


# ---------------------------------------------------------------------------
# Internal callback wrapper used by Camera.connect()
# ---------------------------------------------------------------------------


class _ForwardingCallback(DeviceCallback):
    """Wraps a user DeviceCallback, signals a threading.Event on OnConnected."""

    def __init__(self, event: threading.Event, inner: DeviceCallback | None = None):
        self._event = event
        self._inner = inner
        self.connection_succeeded = False

    def on_connected(self, version: int) -> None:
        self.connection_succeeded = True
        self._event.set()
        if self._inner:
            self._inner.on_connected(version)

    def on_disconnected(self, error: int) -> None:
        self.connection_succeeded = False
        self._event.set()  # unblock connect() if camera rejects
        if self._inner:
            self._inner.on_disconnected(error)

    def on_property_changed(self) -> None:
        if self._inner:
            self._inner.on_property_changed()

    def on_property_changed_codes(self, codes: list[int]) -> None:
        if self._inner:
            self._inner.on_property_changed_codes(codes)

    def on_lv_property_changed(self) -> None:
        if self._inner:
            self._inner.on_lv_property_changed()

    def on_lv_property_changed_codes(self, codes: list[int]) -> None:
        if self._inner:
            self._inner.on_lv_property_changed_codes(codes)

    def on_complete_download(self, filename: str, file_type: int) -> None:
        if self._inner:
            self._inner.on_complete_download(filename, file_type)

    def on_notify_contents_transfer(
        self, notify: int, handle: int, filename: str
    ) -> None:
        if self._inner:
            self._inner.on_notify_contents_transfer(notify, handle, filename)

    def on_warning(self, warning: int) -> None:
        if self._inner:
            self._inner.on_warning(warning)

    def on_error(self, error: int) -> None:
        if self._inner:
            self._inner.on_error(error)


# ---------------------------------------------------------------------------
# Camera class
# ---------------------------------------------------------------------------


class Camera:
    """Represents a single Sony camera device.

    Obtain instances from :func:`enumerate_cameras`.
    """

    def __init__(self, info: CameraObjectInfo, index: int = 0, _enum_ref=None):
        self._info = info
        self._index = index
        self._handle = ctypes.c_int64(0)
        self._callback: DeviceCallback | None = None
        self._connected = False
        # Keep the ICrEnumCameraObjectInfo alive until connect() — its raw pointer
        # owns the ICrCameraObjectInfo* we hold in self._info.
        self._enum_ref = _enum_ref
        # Cache strings now while the vtable pointer is guaranteed valid.
        self._model = info.model
        self._name = info.name
        self._connection_type = info.connection_type_name

    # ------------------------------------------------------------------
    # Connection
    # ------------------------------------------------------------------

    def connect(
        self,
        callback: DeviceCallback | None = None,
        mode: SdkControlMode = SdkControlMode.Remote,
        reconnect: ReconnectingSet = ReconnectingSet.On,
        timeout: float = 10.0,
    ) -> None:
        """Connect to the camera and block until the connection is established.

        The Sony SDK's ``Connect()`` is asynchronous — it returns immediately
        and fires ``OnConnected`` when the camera is actually ready to receive
        commands.  This method waits for that callback before returning.

        Args:
            callback: Optional :class:`DeviceCallback` subclass for camera events.
                      Keep the object alive for the duration of the connection.
            mode: SDK control mode (Remote or ContentsTransfer).
            reconnect: Whether the SDK should auto-reconnect on link loss.
            timeout: Seconds to wait for ``OnConnected`` (default 10).

        Raises:
            SonyConnectionError: if the connection fails or times out.
        """
        connected_event = threading.Event()
        wrapper = _ForwardingCallback(connected_event, callback)
        self._callback = wrapper  # keep alive; SDK holds raw pointer to it

        lib = get_lib()
        err = lib.Connect(
            self._info.raw_ptr,
            wrapper._c_ptr(),
            ctypes.byref(self._handle),
            int(mode),
            int(reconnect),
            None,  # userId
            None,  # userPassword
            None,  # fingerprint
            0,  # fingerprintSize
        )
        check_error(err, "Connect")
        # ICrCameraObjectInfo* is no longer needed; release the enum before waiting.
        self._enum_ref = None

        if not _wait_for_event_pumping_runloop(connected_event, timeout):
            raise SonyConnectionError(f"Camera did not respond within {timeout}s")
        if not wrapper.connection_succeeded:
            raise SonyConnectionError("Camera rejected the connection (OnDisconnected received)")
        self._connected = True

    def disconnect(self) -> None:
        """Disconnect from the camera."""
        if not self._connected:
            return
        lib = get_lib()
        err = lib.Disconnect(self._handle)
        check_error(err, "Disconnect")
        self._connected = False

    def release(self) -> None:
        """Disconnect and release the device handle."""
        if self._connected:
            self.disconnect()
        lib = get_lib()
        lib.ReleaseDevice(self._handle)
        self._handle = ctypes.c_int64(0)

    @property
    def connected(self) -> bool:
        return self._connected

    # ------------------------------------------------------------------
    # Camera info
    # ------------------------------------------------------------------

    @property
    def model(self) -> str:
        return self._model

    @property
    def name(self) -> str:
        return self._name

    @property
    def connection_type(self) -> str:
        return self._connection_type

    @property
    def index(self) -> int:
        return self._index

    def __repr__(self) -> str:
        status = "connected" if self._connected else "disconnected"
        return f"Camera(model={self.model!r}, {status})"

    # ------------------------------------------------------------------
    # Properties
    # ------------------------------------------------------------------

    def get_properties(self) -> dict[int, DeviceProperty]:
        """Retrieve all device properties.

        Returns:
            Dict mapping property code (int) → :class:`DeviceProperty`.

        Raises:
            SonySDKError: on failure.
        """
        lib = get_lib()
        props_ptr = ctypes.c_void_p(0)
        count = ctypes.c_int32(0)
        err = lib.GetDeviceProperties(
            self._handle,
            ctypes.byref(props_ptr),
            ctypes.byref(count),
        )
        check_error(err, "GetDeviceProperties")

        result = {}
        n = count.value
        if not props_ptr.value or n == 0:
            return result

        # The SDK returns a contiguous array of CrDeviceProperty objects.
        # We walk it by reading the code at each position, using the
        # object's own methods (which know its internal layout).
        # We probe the stride by examining address differences.
        stride = _DP_BUF_SIZE  # 128-byte safe upper bound
        base = props_ptr.value

        for i in range(n):
            dp = DeviceProperty.__new__(DeviceProperty)
            dp._buf = None
            dp._ptr = base + i * stride
            code = _dp_GetCode(dp._ptr)
            result[code] = dp

        # Release the SDK-allocated array.
        lib.ReleaseDeviceProperties(self._handle, props_ptr)
        return result

    def get_property(self, code: int) -> DeviceProperty:
        """Retrieve a single device property by *code*.

        Args:
            code: A :class:`~enums.PropertyCode` value or raw integer.

        Returns:
            :class:`DeviceProperty` with current value and metadata.

        Raises:
            SonySDKError: on failure.
        """
        lib = get_lib()
        code_arr = (ctypes.c_uint32 * 1)(int(code))
        props_ptr = ctypes.c_void_p(0)
        count = ctypes.c_int32(0)
        err = lib.GetSelectDeviceProperties(
            self._handle,
            1,
            code_arr,
            ctypes.byref(props_ptr),
            ctypes.byref(count),
        )
        check_error(err, "GetSelectDeviceProperties")

        if not props_ptr.value or count.value == 0:
            raise ValueError(f"Property 0x{code:04X} not found")

        dp = DeviceProperty.__new__(DeviceProperty)
        dp._buf = None
        dp._ptr = props_ptr.value
        # Copy out the values we need before releasing.
        result = DeviceProperty()
        result.code = _dp_GetCode(dp._ptr)
        result.value = _dp_GetCurrentValue(dp._ptr)
        result.value_type = _dp_GetValueType(dp._ptr)

        lib.ReleaseDeviceProperties(self._handle, props_ptr)
        return result

    def set_property(
        self,
        code: int,
        value: int,
        dtype: DataType | int | None = None,
    ) -> None:
        """Set a device property value.

        Args:
            code: Property code (e.g. ``PropertyCode.FNumber``).
            value: New value (cast to uint64 internally).
            dtype: Data type for the property.  If *None*, uses UInt16.

        Raises:
            SonySDKError: on failure.
        """
        if dtype is None:
            dtype = DataType.UInt16

        prop = DeviceProperty()
        prop.code = int(code)
        prop.value = int(value)
        prop.value_type = int(dtype)

        lib = get_lib()
        err = lib.SetDeviceProperty(self._handle, prop._ptr)
        check_error(err, f"SetDeviceProperty(code=0x{code:04X})")

    # ------------------------------------------------------------------
    # Shooting
    # ------------------------------------------------------------------

    def capture(self) -> None:
        """Release the shutter.

        Briefly asserts S1 (half-press) to arm the shutter before firing.
        The Release command alone is ignored by the camera without a prior S1.
        Use ``capture_af()`` if you need AF to lock first (adds a 500 ms S1 hold).
        """
        from .enums import PropertyCode, LockIndicator

        # Brief S1 half-press to arm the shutter mechanism
        self.set_property(PropertyCode.S1, int(LockIndicator.Locked))
        time.sleep(0.1)

        lib = get_lib()
        err = lib.SendCommand(
            self._handle,
            int(CommandId.Release),
            int(CommandParam.Down),
        )
        check_error(err, "SendCommand(Release Down)")
        time.sleep(0.035)
        err = lib.SendCommand(
            self._handle,
            int(CommandId.Release),
            int(CommandParam.Up),
        )
        check_error(err, "SendCommand(Release Up)")
        time.sleep(1.0)  # wait for camera to write the image to card

        self.set_property(PropertyCode.S1, int(LockIndicator.Unlocked))

    def capture_af(self) -> None:
        """Half-press to autofocus, then release the shutter.

        Camera must be in an AF focus mode.
        Matches Sony's sample ``af_shutter()`` exactly.
        """
        from .enums import PropertyCode, LockIndicator

        # S1 half-press — lock focus
        self.set_property(PropertyCode.S1, int(LockIndicator.Locked))
        time.sleep(0.5)  # wait for AF to settle

        lib = get_lib()
        err = lib.SendCommand(
            self._handle,
            int(CommandId.Release),
            int(CommandParam.Down),
        )
        check_error(err, "SendCommand(Release Down)")
        time.sleep(0.035)
        err = lib.SendCommand(
            self._handle,
            int(CommandId.Release),
            int(CommandParam.Up),
        )
        check_error(err, "SendCommand(Release Up)")
        time.sleep(1.0)

        # Release S1 half-press
        self.set_property(PropertyCode.S1, int(LockIndicator.Unlocked))

    def start_movie(self) -> None:
        """Start movie recording."""
        lib = get_lib()
        err = lib.SendCommand(
            self._handle,
            int(CommandId.MovieRecord),
            int(CommandParam.Down),
        )
        check_error(err, "SendCommand(MovieRecord Down)")

    def stop_movie(self) -> None:
        """Stop movie recording."""
        lib = get_lib()
        err = lib.SendCommand(
            self._handle,
            int(CommandId.MovieRecord),
            int(CommandParam.Up),
        )
        check_error(err, "SendCommand(MovieRecord Up)")

    def cancel_shooting(self) -> None:
        """Cancel an in-progress shooting operation."""
        lib = get_lib()
        lib.SendCommand(
            self._handle,
            int(CommandId.CancelShooting),
            int(CommandParam.Down),
        )

    def power_off(self) -> None:
        """Send power-off command to the camera."""
        lib = get_lib()
        lib.SendCommand(
            self._handle,
            int(CommandId.PowerOff),
            int(CommandParam.Down),
        )

    # ------------------------------------------------------------------
    # Live view
    # ------------------------------------------------------------------

    def enable_live_view(self, enabled: bool = True) -> None:
        """Enable or disable live-view streaming.

        Args:
            enabled: True to enable, False to disable.
        """
        lib = get_lib()
        setting_key = 0  # Setting_Key_EnableLiveView
        err = lib.SetDeviceSetting(
            self._handle,
            setting_key,
            1 if enabled else 0,
        )
        check_error(err, "SetDeviceSetting(EnableLiveView)")

    def get_live_view_image(self, buffer_size: int = 1024 * 1024) -> bytes:
        """Capture a single live-view frame.

        Args:
            buffer_size: Size of the image receive buffer in bytes (default 1 MiB).

        Returns:
            Raw JPEG bytes of the live-view frame.  Empty bytes if no frame is
            available yet.

        Raises:
            SonySDKError: on failure.
        """
        ib = ImageDataBlock(buffer_size)
        lib = get_lib()
        err = lib.GetLiveViewImage(self._handle, ib._ptr)
        check_error(err, "GetLiveViewImage")
        return ib.get_image_bytes()

    # ------------------------------------------------------------------
    # Convenience property getters / setters
    # ------------------------------------------------------------------

    def _get_value(self, code: int) -> int:
        return self.get_property(code).value

    def _set_value(
        self, code: int, value: int, dtype: DataType = DataType.UInt16
    ) -> None:
        self.set_property(code, value, dtype)

    # FNumber: value = f-number * 100  (e.g. 280 = f/2.8)
    def get_fnumber(self) -> float:
        from .enums import PropertyCode

        raw = self._get_value(PropertyCode.FNumber)
        return raw / 100.0

    def set_fnumber(self, value: float) -> None:
        from .enums import PropertyCode

        self._set_value(PropertyCode.FNumber, int(value * 100))

    # ISO: bits 24-27 = mode, bits 0-23 = value; 0xFFFFFF = AUTO
    def get_iso(self) -> int:
        from .enums import PropertyCode

        return self._get_value(PropertyCode.IsoSensitivity)

    def set_iso(self, value: int) -> None:
        from .enums import PropertyCode

        self._set_value(PropertyCode.IsoSensitivity, value, DataType.UInt32)

    # ShutterSpeed: UInt32; low-word/high-word fraction (e.g. 1/250 → 0x01_00FA)
    def get_shutter_speed(self) -> int:
        from .enums import PropertyCode

        return self._get_value(PropertyCode.ShutterSpeed)

    def set_shutter_speed(self, value: int) -> None:
        from .enums import PropertyCode

        self._set_value(PropertyCode.ShutterSpeed, value, DataType.UInt32)

    def get_white_balance(self) -> int:
        from .enums import PropertyCode

        return self._get_value(PropertyCode.WhiteBalance)

    def set_white_balance(self, value: int) -> None:
        from .enums import PropertyCode

        self._set_value(PropertyCode.WhiteBalance, value)

    def get_focus_mode(self) -> int:
        from .enums import PropertyCode

        return self._get_value(PropertyCode.FocusMode)

    def set_focus_mode(self, value: int) -> None:
        from .enums import PropertyCode

        self._set_value(PropertyCode.FocusMode, value)

    def get_exposure_program(self) -> int:
        from .enums import PropertyCode

        return self._get_value(PropertyCode.ExposureProgramMode)

    def set_exposure_program(self, value: int) -> None:
        from .enums import PropertyCode

        self._set_value(PropertyCode.ExposureProgramMode, value, DataType.UInt32)

    # ------------------------------------------------------------------
    # File transfer / save settings
    # ------------------------------------------------------------------

    def set_save_info(
        self,
        path: str,
        prefix: str = "",
        file_number: int = 0,
    ) -> None:
        """Set where captured images are saved on the host.

        Args:
            path: Directory path on the host PC.
            prefix: Filename prefix (optional).
            file_number: Starting file number (0 = auto).
        """
        lib = get_lib()
        err = lib.SetSaveInfo(
            self._handle,
            path.encode(),
            prefix.encode(),
            file_number,
        )
        check_error(err, "SetSaveInfo")

    # ------------------------------------------------------------------
    # Context manager support
    # ------------------------------------------------------------------

    def __enter__(self) -> "Camera":
        return self

    def __exit__(self, *_) -> None:
        if self._connected:
            self.release()
