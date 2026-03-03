"""
Pure-Python implementation of SCRSDK::IDeviceCallback using a manually
constructed C++ vtable (Itanium ABI, macOS/Linux with Clang).

In the Itanium ABI a polymorphic object's first field is the vptr, which
points to an array of function pointers (the vtable).  IDeviceCallback has
no virtual destructor, so the vtable slots map exactly to the methods
declared in IDeviceCallback.h, in declaration order:

  slot 0:  OnConnected(this, DeviceConnectionVersioin version)
  slot 1:  OnDisconnected(this, CrInt32u error)
  slot 2:  OnPropertyChanged(this)
  slot 3:  OnPropertyChangedCodes(this, CrInt32u num, CrInt32u* codes)
  slot 4:  OnLvPropertyChanged(this)
  slot 5:  OnLvPropertyChangedCodes(this, CrInt32u num, CrInt32u* codes)
  slot 6:  OnCompleteDownload(this, CrChar* filename, CrInt32u type)
  slot 7:  OnNotifyContentsTransfer(this, CrInt32u notify,
                                    CrContentHandle handle, CrChar* filename)
  slot 8:  OnWarning(this, CrInt32u warning)
  slot 9:  OnError(this, CrInt32u error)

Usage::

    class MyCallback(DeviceCallback):
        def on_connected(self, version):
            print(f"Connected! version={version}")
        def on_property_changed(self):
            print("Properties updated")

    cb = MyCallback()
    # pass cb._c_ptr() to SDK Connect()
"""

import ctypes


# ---------------------------------------------------------------------------
# CFUNCTYPE definitions
# 'this' pointer is the first argument (Itanium calling convention).
# ---------------------------------------------------------------------------

_FnOnConnected = ctypes.CFUNCTYPE(
    None,
    ctypes.c_void_p,  # this
    ctypes.c_int32,  # DeviceConnectionVersioin (int32)
)
_FnOnDisconnected = ctypes.CFUNCTYPE(
    None,
    ctypes.c_void_p,  # this
    ctypes.c_uint32,  # error
)
_FnOnPropertyChanged = ctypes.CFUNCTYPE(
    None,
    ctypes.c_void_p,  # this
)
_FnOnPropertyChangedCodes = ctypes.CFUNCTYPE(
    None,
    ctypes.c_void_p,  # this
    ctypes.c_uint32,  # num
    ctypes.POINTER(ctypes.c_uint32),  # codes
)
_FnOnLvPropertyChanged = ctypes.CFUNCTYPE(
    None,
    ctypes.c_void_p,
)
_FnOnLvPropertyChangedCodes = ctypes.CFUNCTYPE(
    None,
    ctypes.c_void_p,
    ctypes.c_uint32,
    ctypes.POINTER(ctypes.c_uint32),
)
_FnOnCompleteDownload = ctypes.CFUNCTYPE(
    None,
    ctypes.c_void_p,  # this
    ctypes.c_char_p,  # filename (CrChar* = char* in non-unicode build)
    ctypes.c_uint32,  # type
)
_FnOnNotifyContentsTransfer = ctypes.CFUNCTYPE(
    None,
    ctypes.c_void_p,  # this
    ctypes.c_uint32,  # notify
    ctypes.c_uint32,  # CrContentHandle (CrInt32u)
    ctypes.c_char_p,  # filename
)
_FnOnWarning = ctypes.CFUNCTYPE(
    None,
    ctypes.c_void_p,
    ctypes.c_uint32,
)
_FnOnError = ctypes.CFUNCTYPE(
    None,
    ctypes.c_void_p,
    ctypes.c_uint32,
)


# ---------------------------------------------------------------------------
# VTable structure — must match the exact vtable slot order above
# ---------------------------------------------------------------------------


class _VTable(ctypes.Structure):
    _fields_ = [
        ("OnConnected", _FnOnConnected),
        ("OnDisconnected", _FnOnDisconnected),
        ("OnPropertyChanged", _FnOnPropertyChanged),
        ("OnPropertyChangedCodes", _FnOnPropertyChangedCodes),
        ("OnLvPropertyChanged", _FnOnLvPropertyChanged),
        ("OnLvPropertyChangedCodes", _FnOnLvPropertyChangedCodes),
        ("OnCompleteDownload", _FnOnCompleteDownload),
        ("OnNotifyContentsTransfer", _FnOnNotifyContentsTransfer),
        ("OnWarning", _FnOnWarning),
        ("OnError", _FnOnError),
    ]


# The "object": its only field is a pointer to the vtable (the vptr).
class _CallbackObject(ctypes.Structure):
    _fields_ = [
        ("vptr", ctypes.POINTER(_VTable)),
    ]


# ---------------------------------------------------------------------------
# Public DeviceCallback class
# ---------------------------------------------------------------------------


class DeviceCallback:
    """
    Base class for Sony camera event callbacks.

    Subclass this and override the ``on_*`` methods you care about.
    Pass the instance to :func:`Camera.connect`; the SDK will call your
    methods when camera events occur.

    The instance **must** remain alive for as long as the camera is connected,
    because the SDK holds a raw pointer to the underlying C object.
    """

    def on_connected(self, version: int) -> None:
        """Called when the camera connection is established."""

    def on_disconnected(self, error: int) -> None:
        """Called when the camera disconnects.  *error* is a CrError code."""

    def on_property_changed(self) -> None:
        """Called when one or more device properties have changed."""

    def on_property_changed_codes(self, codes: list[int]) -> None:
        """Called with the list of changed property codes."""

    def on_lv_property_changed(self) -> None:
        """Called when live-view properties change."""

    def on_lv_property_changed_codes(self, codes: list[int]) -> None:
        """Called with the list of changed live-view property codes."""

    def on_complete_download(self, filename: str, file_type: int) -> None:
        """Called when a file download from the camera completes."""

    def on_notify_contents_transfer(
        self, notify: int, handle: int, filename: str
    ) -> None:
        """Called for contents-transfer progress notifications."""

    def on_warning(self, warning: int) -> None:
        """Called when the SDK issues a warning (CrWarning_* code)."""

    def on_error(self, error: int) -> None:
        """Called when the SDK reports an error (CrError_* code)."""

    # ------------------------------------------------------------------
    # Internal plumbing — do not call directly
    # ------------------------------------------------------------------

    def _build_c_object(self) -> None:
        """Construct the vtable and C object.  Called once before Connect()."""
        self_ = self  # capture for closures

        # Build CFUNCTYPE thunks that forward to Python methods.
        # Store on self so they are not garbage-collected.
        self._fn_on_connected = _FnOnConnected(lambda _this, v: self_.on_connected(v))
        self._fn_on_disconnected = _FnOnDisconnected(
            lambda _this, e: self_.on_disconnected(e)
        )
        self._fn_on_property_changed = _FnOnPropertyChanged(
            lambda _this: self_.on_property_changed()
        )
        self._fn_on_property_changed_codes = _FnOnPropertyChangedCodes(
            lambda _this, n, codes: self_.on_property_changed_codes(
                [codes[i] for i in range(n)]
            )
        )
        self._fn_on_lv_property_changed = _FnOnLvPropertyChanged(
            lambda _this: self_.on_lv_property_changed()
        )
        self._fn_on_lv_property_changed_codes = _FnOnLvPropertyChangedCodes(
            lambda _this, n, codes: self_.on_lv_property_changed_codes(
                [codes[i] for i in range(n)]
            )
        )
        self._fn_on_complete_download = _FnOnCompleteDownload(
            lambda _this, fname, ftype: self_.on_complete_download(
                fname.decode("utf-8", errors="replace") if fname else "",
                ftype,
            )
        )
        self._fn_on_notify_contents_transfer = _FnOnNotifyContentsTransfer(
            lambda _this, notify, handle, fname: self_.on_notify_contents_transfer(
                notify,
                handle,
                fname.decode("utf-8", errors="replace") if fname else "",
            )
        )
        self._fn_on_warning = _FnOnWarning(lambda _this, w: self_.on_warning(w))
        self._fn_on_error = _FnOnError(lambda _this, e: self_.on_error(e))

        self._vtable = _VTable(
            self._fn_on_connected,
            self._fn_on_disconnected,
            self._fn_on_property_changed,
            self._fn_on_property_changed_codes,
            self._fn_on_lv_property_changed,
            self._fn_on_lv_property_changed_codes,
            self._fn_on_complete_download,
            self._fn_on_notify_contents_transfer,
            self._fn_on_warning,
            self._fn_on_error,
        )
        self._c_obj = _CallbackObject()
        self._c_obj.vptr = ctypes.pointer(self._vtable)

    def _c_ptr(self) -> int:
        """Return the address of the C callback object for passing to SDK."""
        if not hasattr(self, "_c_obj"):
            self._build_c_object()
        return ctypes.addressof(self._c_obj)
