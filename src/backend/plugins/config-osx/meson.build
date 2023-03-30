plugin_name = 'config-osx'

if get_option(plugin_name) and with_platform_darwin

foundation_dep = dependency('Foundation')
system_configuration_dep = dependency('SystemConfiguration')
px_backend_deps += [
  foundation_dep,
  system_configuration_dep,
]

px_backend_sources += [
  'plugins/@0@/@0@.c'.format(plugin_name),
]

endif
