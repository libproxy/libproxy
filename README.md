![build](https://github.com/libproxy/libproxy/actions/workflows/build.yml/badge.svg)
[![codecov](https://codecov.io/github/libproxy/libproxy/branch/main/graph/badge.svg?token=UDbFtICyin)](https://codecov.io/github/libproxy/libproxy)
[![Coverity](https://github.com/libproxy/libproxy/actions/workflows/coverity.yml/badge.svg)](https://scan.coverity.com/projects/libproxy)
[![CodeQL](https://github.com/libproxy/libproxy/workflows/CodeQL/badge.svg)](https://github.com/libproxy/libproxy/actions?query=workflow%3ACodeQL)

# Libproxy
libproxy is a library that provides automatic proxy configuration management.

## The Problem
Problem statement: Applications suck at correctly and consistently supporting proxy configuration.

Proxy configuration is problematic for a number of reasons:

- There are a variety of places to get configuration information
- There are a variety of proxy types
- Proxy auto-configuration (PAC) requires Javascript (which most applications don't have)
- Automatically determining PAC location requires an implementation of the WPAD protocol

These issues make programming with support for proxies hard. Application developers only want to answer the question: Given a network resource, how do I reach it? Because this is their concern, most applications just give up and try to read the proxy from an environment variable. This is problematic because:

- Given the increased use of mobile computing, network switching frequently occurs during the lifetime of an application
- Each application is required to implement the (non-trivial) WPAD and PAC protocols, including a Javascript engine
- It prevents a network administrator from locking down settings on a particular host or user
- In most cases, the environmental variable is almost never correct by default

## The Solution
libproxy exists to answer the question: Given a network resource, how do I reach it? It handles all the details, enabling you to get back to programming.

GNOME? KDE? Command line? WPAD? PAC? Network changed? 
It doesn't matter! Just ask libproxy what proxy to use: you get simple code and your users get correct, consistant behavior and broad infrastructure compatibility. Why use libproxy?

## libproxy offers the following features:
- support for all major platforms: Windows, Mac and Linux/UNIX
- extremely small core footprint
- minimal dependencies within libproxy core
- only 4 functions in the stable-ish external API
- dynamic adjustment to changing network topology
- a standard way of dealing with proxy settings across all scenarios
- a sublime sense of joy and accomplishment

## Repology

[![Arch package](https://repology.org/badge/version-for-repo/arch/libproxy.svg)](https://repology.org/project/libproxy/versions)
[![Debian 13 package](https://repology.org/badge/version-for-repo/debian_13/libproxy.svg)](https://repology.org/project/libproxy/versions)
[![Fedora 40 package](https://repology.org/badge/version-for-repo/fedora_40/libproxy.svg)](https://repology.org/project/libproxy/versions)
[![Fedora Rawhide package](https://repology.org/badge/version-for-repo/fedora_rawhide/libproxy.svg)](https://repology.org/project/libproxy/versions)
[![FreeBSD port](https://repology.org/badge/version-for-repo/freebsd/libproxy.svg)](https://repology.org/project/libproxy/versions)
[![Gentoo package](https://repology.org/badge/version-for-repo/gentoo/libproxy.svg)](https://repology.org/project/libproxy/versions)
[![Homebrew package](https://repology.org/badge/version-for-repo/homebrew/libproxy.svg)](https://repology.org/project/libproxy/versions)
[![Manjaro Stable package](https://repology.org/badge/version-for-repo/manjaro_stable/libproxy.svg)](https://repology.org/project/libproxy/versions)
[![MSYS2 mingw package](https://repology.org/badge/version-for-repo/msys2_mingw/libproxy.svg)](https://repology.org/project/libproxy/versions)
[![openSUSE Leap 15.6 package](https://repology.org/badge/version-for-repo/opensuse_leap_15_6/libproxy.svg)](https://repology.org/project/libproxy/versions)
[![openSUSE Tumbleweed package](https://repology.org/badge/version-for-repo/opensuse_tumbleweed/libproxy.svg)](https://repology.org/project/libproxy/versions)
[![Ubuntu 24.04 package](https://repology.org/badge/version-for-repo/ubuntu_24_04/libproxy.svg)](https://repology.org/project/libproxy/versions)

