"""CrError → Python exception mapping."""

from .enums import CrError


class SonySDKError(Exception):
    """Base exception for all Sony SDK errors."""

    def __init__(self, message: str = "", code: int = 0):
        self.code = code
        super().__init__(message or f"Sony SDK error 0x{code:08X}")


class SonyConnectionError(SonySDKError):
    """Camera connection / communication failure."""


class SonyAPIError(SonySDKError):
    """Invalid API usage or unsupported operation."""


class SonyFileError(SonySDKError):
    """File / storage related error."""


class SonyMemoryError(SonySDKError):
    """Memory allocation failure."""


class SonyDeviceError(SonySDKError):
    """Camera device error."""


def check_error(code: int, context: str = "") -> None:
    """Raise an appropriate exception if *code* is not CrError_None."""
    if code == 0:
        return

    prefix = f"{context}: " if context else ""
    msg = f"{prefix}CrError 0x{code:08X}"

    category = code & 0xFF00
    if category == 0x8200:
        raise SonyConnectionError(msg, code)
    if category == 0x8400:
        raise SonyAPIError(msg, code)
    if category == 0x8100:
        raise SonyFileError(msg, code)
    if category == 0x8300:
        raise SonyMemoryError(msg, code)
    if category == 0x8800:
        raise SonyDeviceError(msg, code)
    raise SonySDKError(msg, code)
