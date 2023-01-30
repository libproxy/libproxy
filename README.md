![build](https://github.com/janbrummer/libproxy2/actions/workflows/build.yml/badge.svg)
[![codecov](https://codecov.io/github/janbrummer/libproxy2/branch/main/graph/badge.svg?token=LS7B1CZKMY)](https://codecov.io/github/janbrummer/libproxy2)
[![Coverage](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/janbrummer/713fa369e20d1c0fdb5896b9a167c3b4/raw/greeter-coverage.json)](https://github.com/janbrummer/repo/actions/workflows/build.yaml)
[![Coverity](https://github.com/janbrummer/libproxy2/actions/workflows/coverity.yml/badge.svg)](https://github.com/janbrummer/libproxy2/actions/workflows/coverity.yml)

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
- no external dependencies within libproxy core
- only 3 functions in the stable-ish external API
- dynamic adjustment to changing network topology
- a standard way of dealing with proxy settings across all scenarios
- a sublime sense of joy and accomplishment
