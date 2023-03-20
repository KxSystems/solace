# Solace kdb+ library installation

## Requirements

- kdb+ ≥ 3.5 64-bit (Linux/macOS/Windows)
- [Solace Messaging C API](https://solace.com/downloads/) (select for your relevant machine type).

## Installing a release

1.  Ensure [Solace Messaging C API](https://solace.com/downloads/?fwp_downloads_types=messaging-apis-and-protocols) is installed.

2.  Make the Solace library available from kdb+:

    -   Linux: Add the lib directory which includes `include` and `lib` to the `LD_LIBRARY_PATH` environment variable e.g. if unzipped to `/usr/local/solaceapi/`, run:

        ```bash
        export LD_LIBRARY_PATH=/usr/local/solaceapi/lib/:$LD_LIBRARY_PATH
        ```
    -   macOS: Add the lib directory which includes `include` and `lib`to the `DYLD_LIBRARY_PATH` environment variable e.g. if unzipped to `/Users/jim/solaceapi/`, run:

        ```bash
        export DYLD_LIBRARY_PATH=/Users/jim/solaceapi/lib/:$DYLD_LIBRARY_PATH
        ```

    -   Windows: Add the `libsolclient.dll` to the kdb+ lib directory e.g. `C:\q\w64` for 64-bit
  
3.  Download the latest release of Solace-kdb+ interface from our [releases page](https://github.com/KxSystems/solaceses). To install shared library and q files, use:

        # Linux/MacOS
        ./install.sh
        
        # Windows
        install.bat

    or copy `solacekdb.so` or `solacekdb.dll` into `QHOME/[l|m|w]64`
