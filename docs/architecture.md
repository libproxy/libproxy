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

