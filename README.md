# New libpurple plugin for Telegram

## Motivation

telegram-purple seems to miss incoming messages a lot, thus writing new plugin using latest tdlib.

## Functionality

Missing features:
* Removing contacts
* Receiving files other than photos
* Sending any files
* Joining new groups (groups we got added to, or which we joined in some other client, will be shown)
* Leaving groups
* List of members in a group
* User pictures

## Installation

RPM packages for Fedora and openSUSE are available at https://download.opensuse.org/repositories/home:/ars3niy/ .

Alternatively, build from source (see below).

## Debugging vs. privacy

It's good to have debug log at hand whenever a glitch is observed, or to be able to reproduce the glitch with loggin turned on. With pidgin, debug log can be turn on like this:
```
pidgin -d >&~/pidgin.log
```

The debug log contains a lot of private information such as names and phone numbers of all contacts, list of all channels you've participated in or text of all sent and received messages. Be mindful of that before posting debug log on the internets. Even just saving debug log to a file can be a questionable idea if there are multiple users on the system (since permissions will be 0644 by default). Such is the nature of debugging instant messaging software.

## Building

TDLib should be prebuilt and installed somewhere (requires C++14):
```
cd <path to TDLib sources>
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
make install DESTDIR=/path/to/tdlib
```
Also see [building](https://github.com/tdlib/td#building) for additional details on TDLib building.

Building this plugin:
```
mkdir build
cd build
cmake -DTd_DIR=/path/to/tdlib/usr/local/lib/cmake/Td ..
make
```

To install, copy the .so to libpurple plugins directory, or run `make install`.

## Regression test

Build google test library and `make install` it somewhere

Run cmake with '-DGTEST_PATH=/path/to/gtest'

`make run-tests` or `test/tests` or `valgrind test/tests`

## GPL compatibility: building tdlib with OpenSSL 3.0

OpenSSL versions prior to 3.0 branch have license with advertisement clause, making it incompatible with GPL. If this is a concern, a possible solution is to build with OpenSSL 3.0 which uses Apache 2.0 license.

### Building OpenSSL

Remove quotes and spaces from `RELEASE_DATE` in VERSION.

Replace `OPENSSL_VERSION_NUMBER` definition with `#define OPENSSL_VERSION_NUMBER 0x30000000L` in include/openssl/opensslv.h.in (not always necessary, depending on cmake version).

```
./config --prefix=/path/to/openssl
make
make install
rm /path/to/openssl/lib/*.so*
```

### Building tdlib

Same as usual, but with additional cmake argument `-DOPENSSL_ROOT_DIR=/path/to/openssl`

If build fails due to linker errors with dlopen etc. not found then

```
sed 's/tdnet/tdcore tdnet/' -i benchmark/CMakeLists.txt
```
