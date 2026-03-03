# sonyalphapy
High-level Python API for Sony Alpha cameras via CrSDK v1.

### Installation
You can install via pip with:
```
pip install sonyalphapy
```

### Typical usage:

1. Connect camera via USB
2. Select "Remote Shooting" (or "PC Remote", depending on your model/firmware)
3. Run:

    ```python
    import sonyalphapy as sony
    
    sony.initialize()
    cameras = sony.enumerate_cameras()
    cam = cameras[0]
    cam.connect()
    cam.capture_af()
    cam.disconnect()
    sony.shutdown()
    ```

### Notes:
 - This is still quite fragile. See `CrSDK_v1/Sony_CameraRemoteSDK_API-Reference_v1.08.00.pdf` PDF to get connected.
 - I have only tested it on macOS 26 with the Sony ILCE-7M4 (Sony a7IV)
 - The `CrSDK_v1/` directory includes the original C++ SDK from Sony directly.
 - A good sanity check is to make sure the [Imaging Edge Desktop](https://imagingedge.sony.net/en/ie-desktop.html) Remote application works correctly.
