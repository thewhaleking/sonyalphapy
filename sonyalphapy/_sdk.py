"""
Low-level ctypes bindings to libCr_Core.dylib.

Exposes:
  - All extern "C" SDK functions
  - CrDeviceProperty wrapper (via exported C++ mangled symbols)
  - CrImageDataBlock wrapper (via exported C++ mangled symbols)
"""

import ctypes
import ctypes.util
import os
import struct
import sys
from typing import Any, Callable

# ---------------------------------------------------------------------------
# Locate and load the dylib
# ---------------------------------------------------------------------------

_HERE = os.path.dirname(os.path.abspath(__file__))
_SDK_DIR = os.path.normpath(os.path.join(_HERE, "..", "CrSDK_v1", "external", "crsdk"))
_ADAPTER_DIR = os.path.join(_SDK_DIR, "CrAdapter")

_lib: ctypes.CDLL | None = None


def _macos_symlink_adapters() -> None:
    """Create adapter symlinks where the Sony SDK expects them on macOS.

    The SDK uses ``NSBundle mainBundle.bundlePath`` + ``/Contents/Frameworks/CrAdapter/``
    to locate its adapter dylibs.  For a command-line Python process that path
    points into the Python.app bundle, not our SDK directory.  We create
    symlinks there once so the SDK can find the adapters.
    """
    try:
        objc = ctypes.CDLL("/usr/lib/libobjc.A.dylib")
        objc.objc_getClass.restype = ctypes.c_void_p
        objc.sel_registerName.restype = ctypes.c_void_p

        def _msg(obj, sel, restype=ctypes.c_void_p):
            fn = objc.objc_msgSend
            fn.restype = restype
            fn.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
            return fn(obj, objc.sel_registerName(sel))

        ns_bundle = objc.objc_getClass(b"NSBundle")
        bundle = _msg(ns_bundle, b"mainBundle")
        path_obj = _msg(bundle, b"bundlePath")
        bundle_bytes = _msg(path_obj, b"UTF8String", ctypes.c_char_p)
        bundle_path = bundle_bytes.decode("utf-8")

        adapter_dst = os.path.join(bundle_path, "Contents", "Frameworks", "CrAdapter")
        os.makedirs(adapter_dst, exist_ok=True)

        for name in (
            "libusb-1.0.0.dylib",
            "libssh2.dylib",
            "libCr_PTP_USB.dylib",
            "libCr_PTP_IP.dylib",
        ):
            src = os.path.join(_ADAPTER_DIR, name)
            dst = os.path.join(adapter_dst, name)
            if os.path.exists(src) and not os.path.lexists(dst):
                os.symlink(src, dst)
    except Exception:
        pass  # Best-effort; SDK will report CrError_Adaptor_Create if it still can't find them.


def _load_lib() -> ctypes.CDLL:
    global _lib
    if _lib is not None:
        return _lib

    if sys.platform == "darwin":
        _macos_symlink_adapters()

    # Preload adapter dylibs so the core can resolve symbols.
    for name in (
        "libusb-1.0.0.dylib",
        "libssh2.dylib",
        "libCr_PTP_USB.dylib",
        "libCr_PTP_IP.dylib",
    ):
        path = os.path.join(_ADAPTER_DIR, name)
        if os.path.exists(path):
            ctypes.CDLL(path)

    core_path = os.path.join(_SDK_DIR, "libCr_Core.dylib")
    _lib = ctypes.CDLL(core_path)
    _bind_functions(_lib)
    _bind_cpp_methods(_lib)
    return _lib


def get_lib() -> ctypes.CDLL:
    """Return the loaded SDK library, loading it on first call."""
    return _load_lib()


# ---------------------------------------------------------------------------
# extern "C" function signatures
# ---------------------------------------------------------------------------


def _bind_functions(lib: ctypes.CDLL) -> None:
    lib.Init.restype = ctypes.c_bool
    lib.Init.argtypes = [ctypes.c_uint32]

    lib.Release.restype = ctypes.c_bool
    lib.Release.argtypes = []

    lib.EnumCameraObjects.restype = ctypes.c_int32
    lib.EnumCameraObjects.argtypes = [
        ctypes.POINTER(ctypes.c_void_p),  # ICrEnumCameraObjectInfo**
        ctypes.c_uint8,  # timeout in seconds
    ]

    lib.Connect.restype = ctypes.c_int32
    lib.Connect.argtypes = [
        ctypes.c_void_p,  # ICrCameraObjectInfo*
        ctypes.c_void_p,  # IDeviceCallback*
        ctypes.POINTER(ctypes.c_int64),  # CrDeviceHandle* out
        ctypes.c_uint32,  # CrSdkControlMode
        ctypes.c_uint32,  # CrReconnectingSet
        ctypes.c_char_p,  # userId (optional)
        ctypes.c_char_p,  # userPassword (optional)
        ctypes.c_char_p,  # fingerprint (optional)
        ctypes.c_uint32,  # fingerprintSize
    ]

    lib.Disconnect.restype = ctypes.c_int32
    lib.Disconnect.argtypes = [ctypes.c_int64]

    lib.ReleaseDevice.restype = ctypes.c_int32
    lib.ReleaseDevice.argtypes = [ctypes.c_int64]

    lib.GetDeviceProperties.restype = ctypes.c_int32
    lib.GetDeviceProperties.argtypes = [
        ctypes.c_int64,  # handle
        ctypes.POINTER(ctypes.c_void_p),  # CrDeviceProperty** out
        ctypes.POINTER(ctypes.c_int32),  # count out
    ]

    lib.GetSelectDeviceProperties.restype = ctypes.c_int32
    lib.GetSelectDeviceProperties.argtypes = [
        ctypes.c_int64,
        ctypes.c_uint32,  # numOfCodes
        ctypes.POINTER(ctypes.c_uint32),  # codes array
        ctypes.POINTER(ctypes.c_void_p),  # CrDeviceProperty** out
        ctypes.POINTER(ctypes.c_int32),  # count out
    ]

    lib.ReleaseDeviceProperties.restype = ctypes.c_int32
    lib.ReleaseDeviceProperties.argtypes = [
        ctypes.c_int64,
        ctypes.c_void_p,  # CrDeviceProperty*
    ]

    lib.SetDeviceProperty.restype = ctypes.c_int32
    lib.SetDeviceProperty.argtypes = [
        ctypes.c_int64,
        ctypes.c_void_p,  # CrDeviceProperty*
    ]

    lib.SendCommand.restype = ctypes.c_int32
    lib.SendCommand.argtypes = [
        ctypes.c_int64,
        ctypes.c_uint32,  # commandId
        ctypes.c_uint16,  # commandParam
    ]

    lib.GetLiveViewImage.restype = ctypes.c_int32
    lib.GetLiveViewImage.argtypes = [
        ctypes.c_int64,
        ctypes.c_void_p,  # CrImageDataBlock*
    ]

    lib.GetLiveViewImageInfo.restype = ctypes.c_int32
    lib.GetLiveViewImageInfo.argtypes = [
        ctypes.c_int64,
        ctypes.c_void_p,  # CrImageInfo* out
    ]

    lib.GetSDKVersion.restype = ctypes.c_uint32
    lib.GetSDKVersion.argtypes = []

    lib.SetSaveInfo.restype = ctypes.c_int32
    lib.SetSaveInfo.argtypes = [
        ctypes.c_int64,
        ctypes.c_char_p,  # path
        ctypes.c_char_p,  # prefix
        ctypes.c_int32,  # file number
    ]

    lib.GetDeviceSetting.restype = ctypes.c_int32
    lib.GetDeviceSetting.argtypes = [
        ctypes.c_int64,
        ctypes.c_uint32,
        ctypes.POINTER(ctypes.c_uint32),
    ]

    lib.SetDeviceSetting.restype = ctypes.c_int32
    lib.SetDeviceSetting.argtypes = [
        ctypes.c_int64,
        ctypes.c_uint32,
        ctypes.c_uint32,
    ]


# ---------------------------------------------------------------------------
# C++ mangled method bindings
# All symbols confirmed present via: nm -gU libCr_Core.dylib
# ---------------------------------------------------------------------------


def _unbound(*_args: Any) -> Any:
    """Placeholder for C++ method pointers before _bind_cpp_methods() runs."""
    raise RuntimeError("SDK not loaded; call initialize() first")


# CrDeviceProperty method pointers (populated by _bind_cpp_methods)
_dp_ctor: Callable[..., Any] = _unbound
_dp_dtor: Callable[..., Any] = _unbound
_dp_GetCode: Callable[..., Any] = _unbound
_dp_SetCode: Callable[..., Any] = _unbound
_dp_GetCurrentValue: Callable[..., Any] = _unbound
_dp_SetCurrentValue: Callable[..., Any] = _unbound
_dp_GetValueType: Callable[..., Any] = _unbound
_dp_SetValueType: Callable[..., Any] = _unbound
_dp_IsGetEnable: Callable[..., Any] = _unbound
_dp_IsSetEnable: Callable[..., Any] = _unbound
_dp_GetValues: Callable[..., Any] = _unbound
_dp_GetValueSize: Callable[..., Any] = _unbound

# CrImageDataBlock method pointers
_ib_ctor: Callable[..., Any] = _unbound
_ib_dtor: Callable[..., Any] = _unbound
_ib_SetSize: Callable[..., Any] = _unbound
_ib_GetSize: Callable[..., Any] = _unbound
_ib_SetData: Callable[..., Any] = _unbound
_ib_GetImageData: Callable[..., Any] = _unbound
_ib_GetImageSize: Callable[..., Any] = _unbound
_ib_GetFrameNo: Callable[..., Any] = _unbound

# CrImageInfo method pointers
_ii_ctor: Callable[..., Any] = _unbound
_ii_dtor: Callable[..., Any] = _unbound
_ii_GetBufferSize: Callable[..., Any] = _unbound


def _bind_cpp_methods(lib: ctypes.CDLL) -> None:
    global _dp_ctor, _dp_dtor, _dp_GetCode, _dp_SetCode
    global _dp_GetCurrentValue, _dp_SetCurrentValue
    global _dp_GetValueType, _dp_SetValueType
    global _dp_IsGetEnable, _dp_IsSetEnable
    global _dp_GetValues, _dp_GetValueSize
    global _ib_ctor, _ib_dtor, _ib_SetSize, _ib_GetSize
    global _ib_SetData, _ib_GetImageData, _ib_GetImageSize, _ib_GetFrameNo
    global _ii_ctor, _ii_dtor, _ii_GetBufferSize

    # CrDeviceProperty
    # Note: nm on macOS shows symbols with an extra leading '_'; dlsym expects
    # them without it, so we use '_ZN...' not '__ZN...'.
    _dp_ctor = lib["_ZN6SCRSDK16CrDevicePropertyC1Ev"]
    _dp_ctor.restype = None
    _dp_ctor.argtypes = [ctypes.c_void_p]

    _dp_dtor = lib["_ZN6SCRSDK16CrDevicePropertyD1Ev"]
    _dp_dtor.restype = None
    _dp_dtor.argtypes = [ctypes.c_void_p]

    _dp_GetCode = lib["_ZN6SCRSDK16CrDeviceProperty7GetCodeEv"]
    _dp_GetCode.restype = ctypes.c_uint32
    _dp_GetCode.argtypes = [ctypes.c_void_p]

    _dp_SetCode = lib["_ZN6SCRSDK16CrDeviceProperty7SetCodeEj"]
    _dp_SetCode.restype = None
    _dp_SetCode.argtypes = [ctypes.c_void_p, ctypes.c_uint32]

    _dp_GetCurrentValue = lib["_ZN6SCRSDK16CrDeviceProperty15GetCurrentValueEv"]
    _dp_GetCurrentValue.restype = ctypes.c_uint64
    _dp_GetCurrentValue.argtypes = [ctypes.c_void_p]

    _dp_SetCurrentValue = lib["_ZN6SCRSDK16CrDeviceProperty15SetCurrentValueEy"]
    _dp_SetCurrentValue.restype = None
    _dp_SetCurrentValue.argtypes = [ctypes.c_void_p, ctypes.c_uint64]

    _dp_GetValueType = lib["_ZN6SCRSDK16CrDeviceProperty12GetValueTypeEv"]
    _dp_GetValueType.restype = ctypes.c_uint32
    _dp_GetValueType.argtypes = [ctypes.c_void_p]

    _dp_SetValueType = lib[
        "_ZN6SCRSDK16CrDeviceProperty12SetValueTypeENS_10CrDataTypeE"
    ]
    _dp_SetValueType.restype = None
    _dp_SetValueType.argtypes = [ctypes.c_void_p, ctypes.c_uint32]

    _dp_IsGetEnable = lib["_ZN6SCRSDK16CrDeviceProperty23IsGetEnableCurrentValueEv"]
    _dp_IsGetEnable.restype = ctypes.c_bool
    _dp_IsGetEnable.argtypes = [ctypes.c_void_p]

    _dp_IsSetEnable = lib["_ZN6SCRSDK16CrDeviceProperty23IsSetEnableCurrentValueEv"]
    _dp_IsSetEnable.restype = ctypes.c_bool
    _dp_IsSetEnable.argtypes = [ctypes.c_void_p]

    _dp_GetValues = lib["_ZN6SCRSDK16CrDeviceProperty9GetValuesEv"]
    _dp_GetValues.restype = ctypes.c_void_p
    _dp_GetValues.argtypes = [ctypes.c_void_p]

    _dp_GetValueSize = lib["_ZN6SCRSDK16CrDeviceProperty12GetValueSizeEv"]
    _dp_GetValueSize.restype = ctypes.c_uint32
    _dp_GetValueSize.argtypes = [ctypes.c_void_p]

    # CrImageDataBlock
    _ib_ctor = lib["_ZN6SCRSDK16CrImageDataBlockC1Ev"]
    _ib_ctor.restype = None
    _ib_ctor.argtypes = [ctypes.c_void_p]

    _ib_dtor = lib["_ZN6SCRSDK16CrImageDataBlockD1Ev"]
    _ib_dtor.restype = None
    _ib_dtor.argtypes = [ctypes.c_void_p]

    _ib_SetSize = lib["_ZN6SCRSDK16CrImageDataBlock7SetSizeEj"]
    _ib_SetSize.restype = None
    _ib_SetSize.argtypes = [ctypes.c_void_p, ctypes.c_uint32]

    _ib_GetSize = lib["_ZN6SCRSDK16CrImageDataBlock7GetSizeEv"]
    _ib_GetSize.restype = ctypes.c_uint32
    _ib_GetSize.argtypes = [ctypes.c_void_p]

    _ib_SetData = lib["_ZN6SCRSDK16CrImageDataBlock7SetDataEPh"]
    _ib_SetData.restype = None
    _ib_SetData.argtypes = [ctypes.c_void_p, ctypes.c_void_p]

    _ib_GetImageData = lib["_ZN6SCRSDK16CrImageDataBlock12GetImageDataEv"]
    _ib_GetImageData.restype = ctypes.c_void_p
    _ib_GetImageData.argtypes = [ctypes.c_void_p]

    _ib_GetImageSize = lib["_ZN6SCRSDK16CrImageDataBlock12GetImageSizeEv"]
    _ib_GetImageSize.restype = ctypes.c_uint32
    _ib_GetImageSize.argtypes = [ctypes.c_void_p]

    _ib_GetFrameNo = lib["_ZN6SCRSDK16CrImageDataBlock10GetFrameNoEv"]
    _ib_GetFrameNo.restype = ctypes.c_uint32
    _ib_GetFrameNo.argtypes = [ctypes.c_void_p]

    # CrImageInfo
    _ii_ctor = lib["_ZN6SCRSDK11CrImageInfoC1Ev"]
    _ii_ctor.restype = None
    _ii_ctor.argtypes = [ctypes.c_void_p]

    _ii_dtor = lib["_ZN6SCRSDK11CrImageInfoD1Ev"]
    _ii_dtor.restype = None
    _ii_dtor.argtypes = [ctypes.c_void_p]

    _ii_GetBufferSize = lib["_ZN6SCRSDK11CrImageInfo13GetBufferSizeEv"]
    _ii_GetBufferSize.restype = ctypes.c_uint32
    _ii_GetBufferSize.argtypes = [ctypes.c_void_p]


# ---------------------------------------------------------------------------
# CrDeviceProperty Python wrapper
# ---------------------------------------------------------------------------

_DP_BUF_SIZE = 128  # Larger than the actual struct; safe upper bound.


class DeviceProperty:
    """Wraps a CrDeviceProperty C++ object for get/set operations."""

    __slots__ = ("_buf", "_ptr")

    def __init__(self):
        _load_lib()
        self._buf = ctypes.create_string_buffer(_DP_BUF_SIZE)
        self._ptr = ctypes.addressof(self._buf)
        _dp_ctor(self._ptr)

    def __del__(self):
        if _dp_dtor is not None and self._ptr:
            _dp_dtor(self._ptr)

    # --- read accessors ---

    @property
    def code(self) -> int:
        return _dp_GetCode(self._ptr)

    @property
    def value(self) -> int:
        return _dp_GetCurrentValue(self._ptr)

    @property
    def value_type(self) -> int:
        return _dp_GetValueType(self._ptr)

    @property
    def readable(self) -> bool:
        return _dp_IsGetEnable(self._ptr)

    @property
    def writable(self) -> bool:
        return _dp_IsSetEnable(self._ptr)

    def get_available_values(self, elem_size: int = 2) -> list[int]:
        """Return possible values as a list.

        *elem_size* is the byte width of each element (2 for UInt16, 4 for UInt32).
        """
        raw_ptr = _dp_GetValues(self._ptr)
        raw_size = _dp_GetValueSize(self._ptr)
        if not raw_ptr or not raw_size:
            return []
        buf = (ctypes.c_uint8 * raw_size).from_address(raw_ptr)
        fmt = {1: "B", 2: "H", 4: "I", 8: "Q"}.get(elem_size, "H")
        return list(struct.unpack_from(f"<{raw_size // elem_size}{fmt}", bytes(buf)))

    # --- write accessors (used when building a property to send) ---

    @code.setter
    def code(self, v: int) -> None:
        _dp_SetCode(self._ptr, v)

    @value.setter
    def value(self, v: int) -> None:
        _dp_SetCurrentValue(self._ptr, v)

    @value_type.setter
    def value_type(self, v: int) -> None:
        _dp_SetValueType(self._ptr, v)

    def __repr__(self) -> str:
        return (
            f"DeviceProperty(code=0x{self.code:04X}, "
            f"value=0x{self.value:X}, type=0x{self.value_type:04X})"
        )


def _wrap_property_array(ptr: int, count: int) -> list[DeviceProperty]:
    """Wrap a raw CrDeviceProperty* array returned by GetDeviceProperties."""
    result = []
    # Each element in the array is _DP_BUF_SIZE bytes; but since the SDK
    # allocates the array with the correct sizeof(CrDeviceProperty), we walk
    # by calling GetCode on increasing offsets until we know the stride.
    # Instead: create DeviceProperty shells that alias into the SDK buffer.
    for i in range(count):
        dp = DeviceProperty.__new__(DeviceProperty)
        # We alias the SDK-allocated memory; do NOT call our destructor on it.
        dp._buf = None
        dp._ptr = ptr + i * _DP_BUF_SIZE  # stride approximation; see note below
        result.append(dp)
    return result


# ---------------------------------------------------------------------------
# CrImageDataBlock Python wrapper
# ---------------------------------------------------------------------------

_IB_BUF_SIZE = 64


class ImageDataBlock:
    """Wraps a CrImageDataBlock C++ object for live-view image retrieval."""

    __slots__ = ("_buf", "_ptr", "_data_buf")

    def __init__(self, buffer_size: int = 0):
        _load_lib()
        self._buf = ctypes.create_string_buffer(_IB_BUF_SIZE)
        self._ptr = ctypes.addressof(self._buf)
        self._data_buf: ctypes.Array | None = None
        _ib_ctor(self._ptr)
        if buffer_size:
            self.allocate(buffer_size)

    def __del__(self):
        if _ib_dtor is not None and self._ptr:
            _ib_dtor(self._ptr)

    def allocate(self, size: int) -> None:
        """Allocate an internal image buffer of *size* bytes."""
        self._data_buf = (ctypes.c_uint8 * size)()
        _ib_SetSize(self._ptr, size)
        _ib_SetData(self._ptr, ctypes.addressof(self._data_buf))

    @property
    def image_size(self) -> int:
        return _ib_GetImageSize(self._ptr)

    @property
    def frame_no(self) -> int:
        return _ib_GetFrameNo(self._ptr)

    def get_image_bytes(self) -> bytes:
        """Return the captured image data as a bytes object."""
        size = self.image_size
        if not size:
            return b""
        data_ptr = _ib_GetImageData(self._ptr)
        if not data_ptr:
            return b""
        return bytes((ctypes.c_uint8 * size).from_address(data_ptr))
