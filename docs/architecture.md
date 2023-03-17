Title: Architecture - Difference between library and D-Bus service
Slug: Architecture


# Architecture

Starting with release 0.5.0 Libproxy is making use of glib. glib has many
advantages and helps to get rid of one of the major issues we have had with
previous versions of Libproxy. To name a few advantages:

- Testing Framework
- Make use of existing plugin loader
- Automatic documentation out of code
- Gobject Introspection bindings for almost every programming language
- D-Bus Service

Using D-Bus as a service on Linux we can elimnate the previous ABI clashes we
have had in the past. mozjs in different versions on one system could lead to
crashes in cases where the application using libproxy favours a different mozjs
version.

## Building Libproxy as simple library

On non D-Bus system you still have the option to build Libproxy as a self
contained library using `-Ddbus=disabled`. In this case you can still run into
an ABI issue, but with it's current plugins it is unlikely at the moment.

## Building Libproxy as D-Bus Service
On D-Bus system Libproxy will be build using a D-Bus service and a simple
helper library. The main logic part (previous simple library) is done in the
D-Bus service and so can never crash an application.
The application itself will be linked with the helper library which does the
necessary D-Bus calls and wait's for answer.
