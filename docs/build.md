# Building Solace kdb+ library from source

## Requirements

-   [Solace Messaging C API](https://solace.com/downloads/)(please select for your relevant machine type).
-   C++11 or later [^1] 
-   CMake ≥ 3.1 [^1]

[^1]: Required when building from source

## Linux/MacOSX

1.  Download the [Solace C Messaging API](https://solace.com/downloads/?fwp_downloads_types=messaging-apis-and-protocols).
2.  Set `SOLACE_INSTALL_DIR` to the instalation directory of the Solace Messaging C API where `include` and `lib` exist. This environmental variable will be used to link the library to Solace-kdb+ interface.
3.  Add `SOLACE_INSTALL_DIR/lib` to `LD_LIBRARY_PATH` (Linux) or `DYLD_LIBRARY_PATH` (MacOSX).

```bash
]$ mkdir solclient
]$ tar xzf solclient_[machine and version].tar.gz -C ./solclient --strip-components=1
]$ cd solclient
solclient]$ export SOLACE_INSTALL_DIR=$(pwd)
```

4. Clone Solace-kdb+ repository and build with `cmake`.

```bash
]$ git clone https://github.com/KxSystems/solace.git
]$ cd solace
solace]$ mkdir build && cd build
build]$ cmake ../
build]$ cmake --build . --target install
```

**Note:** `cmake --build . --target install` as used in the Linux/MacOS builds installs the required share object and q files to the `QHOME/[ml]64` and `QHOME` directories respectively. If you do not wish to install these files directly, you can execute `cmake --build .` instead of `cmake --build . --target install` and move the files from their build location at `build/kdbsolace`.

## Windows

1.  Download the [Solace C Messaging API](https://solace.com/downloads/?fwp_downloads_types=messaging-apis-and-protocols).
2.  Set `SOLACE_INSTALL_DIR` to the installation directory of the Solace Messaging C API where `include` and `lib` exist. This environmental variable will be used to link the library to Solace-kdb+ interface.
3.  Add the `libsolclient.dll` to the kdb+ lib directory e.g. `C:\q\w64` for 64-bit

     ```bat
     > 7z e solclient_[machine and version].tar.gz && 7z x solclient_[machine and version].tar -spe
     > rename solclient-[version] solclient
     > cd solclient
     solclient> set SOLACE_INSTALL_DIR=%cd%
     solclient> cd %QHOME%\w64
     w64> MKLINK libsolclient.dll %SOLACE_INSTALL_DIR%\bin\Win64\libsolclient.dll
     ```

4.  Clone Solace-kdb+ repository and build with `cmake`. Building the interface from source requires Visual Studio (assuming `-G "Visual Studio 15 2017 Win64"` is not necessary).

     ```bat
     > git clone https://github.com/KxSystems/solace.git
     > cd solace
     solace> mkdir build && build
     build> cmake --config Release ..
     build> cmake --build . --config Release --target install
     ```

**Notes:** 

1. `cmake --build . --config Release --target install` installs the required share object and q files to the `QHOME\w64` and `QHOME` directories respectively. If you do not wish to install these files directly, you can execute `cmake --build . --config Release` instead of `cmake --build . --config Release --target install` and move the files from their build location at `build/kdbsolace`.
2.  You can use flag `cmake -G "Visual Studio 16 2019" -A Win32` if building 32-bit version.
