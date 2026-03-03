"""
vtable helpers for calling virtual methods on SDK-returned C++ objects.

In the Itanium ABI (macOS/Linux with Clang/GCC), every polymorphic C++ object
starts with a vptr — a pointer to the vtable.  The vtable is an array of
function pointers, indexed from 0 for the first declared virtual method.

This module provides:
  - _vcall()         — raw vtable dispatch
  - EnumCameraInfo   — wrapper for ICrEnumCameraObjectInfo*
  - CameraObjectInfo — wrapper for ICrCameraObjectInfo*
"""

import ctypes


def _vcall(obj_ptr: int, slot: int, restype, argtypes, *args):
    """Call virtual method at *slot* on the C++ object at *obj_ptr*.

    The object's first field is a void* (the vptr) pointing to the vtable.
    vtable[slot] is the function pointer for the requested method.
    All C++ methods on the Itanium ABI receive *this* as the first argument.
    """
    # Read the vptr from the object's first field.
    vptr = ctypes.cast(obj_ptr, ctypes.POINTER(ctypes.c_void_p))[0]
    # Index the vtable to get the function pointer.
    vtable = ctypes.cast(vptr, ctypes.POINTER(ctypes.c_void_p))
    fn_addr = vtable[slot]
    fn_type = ctypes.CFUNCTYPE(restype, ctypes.c_void_p, *argtypes)
    fn = fn_type(fn_addr)
    return fn(obj_ptr, *args)


# ---------------------------------------------------------------------------
# ICrEnumCameraObjectInfo wrapper
#
# vtable layout (declaration order from ICrCameraObjectInfo.h):
#   slot 0: GetCount() -> CrInt32u
#   slot 1: GetCameraObjectInfo(index: CrInt32u) -> ICrCameraObjectInfo*
#   slot 2: Release() -> void
# ---------------------------------------------------------------------------


class EnumCameraInfo:
    """Wraps a raw ICrEnumCameraObjectInfo* returned by EnumCameraObjects()."""

    def __init__(self, ptr: int):
        self._ptr = ptr

    def get_count(self) -> int:
        return _vcall(self._ptr, 0, ctypes.c_uint32, [])

    def get_camera_object_info(self, index: int) -> "CameraObjectInfo":
        raw = _vcall(self._ptr, 1, ctypes.c_void_p, [ctypes.c_uint32], index)
        return CameraObjectInfo(raw)

    def release(self) -> None:
        _vcall(self._ptr, 2, None, [])
        self._ptr = 0

    def __len__(self) -> int:
        return self.get_count()

    def __getitem__(self, index: int) -> "CameraObjectInfo":
        n = self.get_count()
        if index < 0:
            index += n
        if not (0 <= index < n):
            raise IndexError(index)
        return self.get_camera_object_info(index)

    def __iter__(self):
        for i in range(self.get_count()):
            yield self.get_camera_object_info(i)

    def __del__(self):
        if self._ptr:
            self.release()

    def __enter__(self):
        return self

    def __exit__(self, *_):
        if self._ptr:
            self.release()


# ---------------------------------------------------------------------------
# ICrCameraObjectInfo wrapper
#
# vtable layout (declaration order from ICrCameraObjectInfo.h):
#   slot  0: Release()               -> void
#   slot  1: GetName()               -> char*
#   slot  2: GetNameSize()           -> CrInt32u
#   slot  3: GetModel()              -> char*
#   slot  4: GetModelSize()          -> CrInt32u
#   slot  5: GetUsbPid()             -> CrInt16
#   slot  6: GetId()                 -> CrInt8u*
#   slot  7: GetIdSize()             -> CrInt32u
#   slot  8: GetIdType()             -> CrInt32u
#   slot  9: GetConnectionStatus()   -> CrInt32u
#   slot 10: GetConnectionTypeName() -> char*
#   slot 11: GetAdaptorName()        -> char*
#   slot 12: GetGuid()               -> char*
#   slot 13: GetPairingNecessity()   -> char*
#   slot 14: GetAuthenticationState()-> CrInt16u
#   slot 15: GetSSHsupport()         -> CrInt32u
# ---------------------------------------------------------------------------


class CameraObjectInfo:
    """Wraps a raw ICrCameraObjectInfo* obtained from EnumCameraInfo."""

    def __init__(self, ptr: int):
        self._ptr = ptr

    def _str(self, slot: int) -> str:
        raw = _vcall(self._ptr, slot, ctypes.c_char_p, [])
        return raw.decode("utf-8", errors="replace") if raw else ""

    @property
    def name(self) -> str:
        return self._str(1)

    @property
    def model(self) -> str:
        return self._str(3)

    @property
    def usb_pid(self) -> int:
        return _vcall(self._ptr, 5, ctypes.c_int16, [])

    @property
    def id_bytes(self) -> bytes:
        ptr = _vcall(self._ptr, 6, ctypes.c_void_p, [])
        size = _vcall(self._ptr, 7, ctypes.c_uint32, [])
        if not ptr or not size:
            return b""
        return bytes((ctypes.c_uint8 * size).from_address(ptr))

    @property
    def id_type(self) -> int:
        return _vcall(self._ptr, 8, ctypes.c_uint32, [])

    @property
    def connection_status(self) -> int:
        return _vcall(self._ptr, 9, ctypes.c_uint32, [])

    @property
    def connection_type_name(self) -> str:
        return self._str(10)

    @property
    def adaptor_name(self) -> str:
        return self._str(11)

    @property
    def guid(self) -> str:
        return self._str(12)

    @property
    def pairing_necessity(self) -> str:
        return self._str(13)

    @property
    def authentication_state(self) -> int:
        return _vcall(self._ptr, 14, ctypes.c_uint16, [])

    @property
    def ssh_support(self) -> int:
        return _vcall(self._ptr, 15, ctypes.c_uint32, [])

    @property
    def raw_ptr(self) -> int:
        return self._ptr

    def __repr__(self) -> str:
        return (
            f"CameraObjectInfo(model={self.model!r}, "
            f"connection={self.connection_type_name!r})"
        )
