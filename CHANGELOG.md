# Release Notes

## [Unreleased]

### Changed

- Total rewrite in order to address current pain points:
  - No documentation
  - ABI crashes
  - No tests
  - No CI
  - Missing async API

## [0.4.18]

### Changed

- build: Allow configuration of sysconfig module
- config_envvar: Add environment variable for pacrunner debugging
- build: disable mozjs by default
- python: Support Python 3.10 and above
- Add Duktape pacrunner module
- config_kde: Compute list of config file locations ourselves
- cpmfog_gnome3: Add gnome-wayland to permitted DESKTOP_SESSION

## [0.4.17]

## Changed

-  python bindings: fix "TypeError: _argtypes_ must be a sequence of types" (#125)

## [0.4.16]

### Changed

- Port to, and require, SpiderMonkey 68
- Use closesocket() instead of close() on Windows 
- Add symbol versions - be ready to introduce new APIs as needed
- Add public px_proxy_factory_free_proxies function
- Add PacRunner config backend (largely untested; feedback welcome!)
- Small performance improvements
- pxgsettings: use the correct syntax to connect to the changed signal (silences annoying output on console)
- Support python3 up to version 3.9
- Fix buffer overflow when PAC is enabled (CVE-2020-26154)
- Rewrite url::recvline to be nonrecursive (CVE-2020-25219)
- Remove nonfunctional and crashy pacrunner caching
- Never use system libmodman (no other consumers, not maintained)

## [0.4.15]

### Changed

- Port to, and require, SpiderMonkey 38.
- Fix "NetworkManager plugin not being built" (gh#libproxy/libproxy#53).
- Fix "networkmanager plugin not working (gh#libproxy/libproxy#58).
- Fix "Invalid read after free" (gh#libproxy/libproxy#59).
- Fix intermittent unit test failures.

## [0.4.14]

### Changed

- Parallel build support for python2 and python3.
  -DWITH_PYTHON has been replaced with -DWITH_PYTHON2 and
  -DWITH_PYTHON3 to have full control over this. Default is
  ON for both (issue#22)
- Minor fixes to the PAC retriever code (issue#40)
- Fallback to mcs instead of gmcs for the mono bindings (issue#37)
- Fix build using cmake 3.7
- Fix deprecation warnings of pxgsettings with glib 2.46
- Improve the get-pac test suite (issue#47)

## [0.4.13]

### Changed

- Allow linking webkit pacrunner against javascriptcore-4.0
  (webkit2).
- Allow to disable building of the KDE module (-DWITH_KDE=ON/OFF).
- Fix compilation errors with CLang on MacOSX.
- bindings: perl: Add an option to explicitly link against libperl.so
  Some distributions want to do it, other prefer not to, the library
  is anyway in context of perl.
- config_kde: Add a basic cache and invalidation: performance improvement
  for the KDE module.

## [0.4.12]

### Changed

- Move development to github.com/libproxy/libproxy
- Fix fd leak in get_pac (Bug #185)
- Detect running MATE session (Bug #186, Part1).
- Fix linking of perl bindings to pthread (Bug #182)
- Correctly detect spidermonky (mozjs185) (Bug #188)
- Stop pxgsettings from segfaulting on exit (Bug #192)
- Fix test #10 (Bug #189)
- Fix build on Mac OS X (Bug #183)
- Add a generic KDE Config module (fix crashes of Qt5 based
  apps) (issue#4)

## [0.4.11]

### Changed

- Build fixes with cmake 2.8.10+
- Quick release without built binaries / files (Address Bug #184)

## [0.4.10]

### Changed

- Fix http chunk encoded PAC that was broken in previous release
- Add HTTP client unit test
- Fix more coding style issues

## [0.4.9]

### Changed

- CVE-2012-4504 Fixed buffer overflow when downloading PAC
- Fix infinit loop uppon network errors

## [0.4.8]

### Changed

- Only support standalone mozjs185 as mozilla js engine.
  xulrunner being part of the now lightning fast moving firefox
  is impossible to be tracked as a dependency and it is not
  supported by Mozilla to be used in this scenario.
- Support building with javascritpcoregtk 1.5
  (got split out of webkitgtk).
- Support sending multiple results.
- Issues fixed:
  - #166: Libproxy does not parse NO_PROXY correct when the line
          contains spaces
  - #164: If gconf's value is an empty list, pxgconf will make
          /usr/bin/proxy wait forever
  - #60: use lib js for embedded solutions
  - #160: strdup and gethostbyname not declared on OSX 10.7
  - #168: .pc file should be installed under OSX as well.
  - #170: Also check for "Transfer-Encoding: chunked".
  - #171: mozjs pacrunner: Fix parameters of dnsResolve_()
  - #172: Allow to forcibly build pacrunner as module (-DBIPR={ON,OFF})
  - #173: Libproxy doesn't build with gcc 4.7
  - #147: Use ${CMAKE_DL_LIBS} instead of assuming libdl is correct.
  - #176: python bindings: guard the destructor.
  - #177: Speed up importing of libproxy in python.
  - #179: CMAKE 2.8.8 does not define PKG_CONFIG_FOUND

## [0.4.7]

### Changed

- Support/require xulrunner 2.0+
- Support linking againgst libwebkit-gtk3 (-DWITH_WEBKIT3=ON)
- Port to gsettings for gnome3. (-DWITH_GNOME3=ON[default])
- Issues closed:
  - #149: always test for the right python noarch module path
  - #155: Cannot compile with Firefox 4
  - #156: libproxy should build against webkitgtk-3.0
  - #158: Won't compile w/ xulrunner 2.0 final
  - #159: libproxy fails with autoconfiguration "http://proxy.domain.com"
  - #131: GSettings-based GNOME plugin
  - #150: SUSE sysconfig/proxy config support

## [0.4.6]

### Changed

- Fixed a crash in the URL parser
- Fixed build issues with Visual Studio
- Updated the INSTALL file
- Install Python binding in prefix path if site-packages exists
- Fixed compilation with Visual Studio


## [0.4.5]

### Changed

- C# bindings are installable (-DWITH_DOTNET=ON)
- C# bindings installation path can be changed using -DGAC_DIR=
- Internal libmodman build fixed
- Installation dirs are now all relative to CMAKE_INSTALL_PREFIX
- Fixed test while using --as-needed linker flag
- Fixed generation of libproxy-1.0.pc
- Basic support for Mingw added (not yet 100% functional)
- Ruby binding implemented (not yet in the build system)
- Fixed modules not being found caused by relative LIBEXEC_INSTALL_DIR
- Fixed bug with builtin plugins (Issue 133)
- Vala bindings installation path can be changed using -DVAPI_DIR=
- Python bindings installation path can be changed using -DPYTHON_SITEPKG_DIR=
- Perl bindings can be installed in vendor directory (-DPERL_VENDORARCH=ON)
- Perl bindings installation path can be change using -DPX_PERL_ARCH=
- Unit test now builds on OSX

## [0.4.4]

### Changed

- Add support for optionally building using a system libmodman
- Rework build system to be cleaner
- Fix two major build system bugs: 127, 128

## [0.4.3]

### Changed

- Test can now be out-compiled using BUILD_TESTING=OFF
- Fixed python binding not handling NULL pointer
- Pyhton binding now support Python version 3
- Rewrote URL parser to comply with unit test
- Username and password are now URL encoded
- Scheme comparison is now non-case sensitive
- Fixed deadlock using WebKit has PAC runner
- Fixed OS X compilation of Perl bindings

## [0.4.2]

### Changed

- Fixed python binding that failed on missing px_free symbole
- Workaround cmake bug with dynamic libraries in non-standard folders

## [0.4.1]

### Changed

- Perl bindings have been integrated into the CMake Build System
- Vala bindings are installed if -DWITH_VALA=yes is passed to cmake
- All extensions can be disabled using WITH_*=OFF cmake options
- socks5:// and socks4:// can now be returned
- Many bugfixes

## [0.4.0]

### Changed

- C++ rewrite
- Small API change (px_proxy_factory_get_proxy() can now return NULL)
- SOVERSION bump 
- libmodman is now a seperate library
- Migrate to cmake
- Windows support (config_w32reg, ignore_hostname; VC++ support)
- MacOSX support (config_macosx, ignore_hostname)
- Built-in modules support
- Support for chunked encoding
- Move to hidden visibility by default
- KDE's KConfig symantics are fully supported
- Removeal of all PX_* env variables (no longer needed)
- Symbol based detection of relevant pacrunner
- Reworked config_gnome to not suck (its *much* faster)
- Many other things I can't remember

## [0.3.1]

### Changed

- Bugfixes
  + config file parser reads all sections
  + KDE session detection based on environment varibales,
    as suggested by KDE upstream.
- KDE configuration module is the first module in C++ and
  now links to libkdecore4 in order to properly detect the
  configuration folder for kde.
- At the moment we're not compatible with KDE3. Sorry.
- .NET bindings can now properly be installed and it should
  be possible for packagers to provide them.

## [0.3.0]

### Changed

- WARNING!!! Slight API change!!! see docs 
  for px_proxy_factory_get_proxies()
- Credentials support (see API change above)
- A complete rewrite of the module manager
- file:// as valid PAC URLs
- Sample Mono application
- Automake 1.11 shaved output
- gnome backend rewrite (now w/o thread issues)
- Test suite base functionality exists
- Many solaris build fixes
- Seamonkey support as JS pacrunner
- Bugfixes
- Compiles for MS Windows using Mingw
