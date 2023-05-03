Title: Architecture of Libproxy
Slug: Architecture


# Architecture

Starting with release 0.5.0 Libproxy is making use of glib. glib has many
advantages and helps to get rid of one of the major issues we have had with
previous versions of Libproxy. To name a few advantages:

- Testing Framework
- Make use of existing plugin loader
- Automatic documentation out of code
- Gobject Introspection bindings for almost every programming language

## Plugin types

Libproxy is using internal plugins to extend it's knowledge about different
configuration options and handling of PAC files. Most of those plugins are
configuration plugins:

- config-env
- config-gnome
- config-kde
- config-osx
- config-sysconfig
- config-windows

Then there is only one pacrunner plugin which handles parsing of JS PAC files:

- pacrunner-duktape

## Configuration plugin priority

Configuration plugins are executed in a specific priority:

- PX_CONFIG_PRIORITY_FIRST (highest)
- PX_CONFIG_PRIORITY_DEFAULT
- PX_CONFIG_PRIORITY_LAST (lowest)

Highest priority is used in `config-env` and lowest priority in
`config-sysconfig`. All the other plugins are running on default priority.

### Configuration plugin enforcement

In case you want to use a specific configuration plugin for a given application,
you are able to overwrite the priority. Therefore set a environment variable
`PX_FORCE_CONFIG=config-name` and libproxy will just use this configuration
plugin.
