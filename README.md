# HsQML

[![Tests](https://github.com/prolic/HsQML/workflows/Tests/badge.svg)](https://github.com/prolic/HsQML/actions?query=workflow%3ATests)
[![Build status](https://ci.appveyor.com/api/projects/status/github/prolic/HsQML?svg=true)](https://ci.appveyor.com/project/prolic/hsqml)

HsQML is a Haskell binding to Qt Quick, a cross-platform framework for creating
graphical user interfaces. For further information on installing and using it,
please see the project's web site.

**Home Page:** http://www.gekkou.co.uk/software/hsqml/
**Original Darcs Repository (outdated):** http://hub.darcs.net/komadori/HsQML

## Notes
- I made some changes, so this library works on newer GHC / cabal versions and has been tested on Qt5 and Qt6.
- This library has been tested with cabal 3.10.3.0, due to its custom Setup.hs, not all versions of cabal are supported.
- **Current Status:** This is a maintained fork of the original HsQML project.

## Building
- Qt5 build on Linux:
  - `PATH=/usr/lib64/qt5/bin:$PATH cabal-3.10.3.0 build --enable-tests`
- Qt6 build on Linux:
  - `PATH=/usr/lib64/qt6/libexec:$PATH cabal-3.10.3.0 build --enable-tests -fuseqt6`
- Qt6 support is enabled with the Cabal flag `useqt6`.
- The custom `Setup.hs` will prefer the matching `moc` executable and set `QT_SELECT` automatically when it is not already set.
