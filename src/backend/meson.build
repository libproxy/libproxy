api_version = '1.0'

px_backend_sources = [
  'px-manager.c',
  'px-manager.h',
  'px-plugin-config.c',
  'px-plugin-config.h',
  'px-plugin-pacrunner.c',
  'px-plugin-pacrunner.h',
]

px_backend_deps = [
  curl_dep,
  gio_dep,
  glib_dep,
  peas_dep,
]

px_backend_c_args = [
  '-DG_LOG_DOMAIN="libpxbackend"',
]

px_backend_inc = include_directories('.')

px_backend_lib = shared_library(
  'pxbackend-@0@'.format(api_version),
  px_backend_sources,
  dependencies: px_backend_deps,
  c_args: px_backend_c_args,
  install: true
)

gnome = import('gnome')
px_backend_gir = gnome.generate_gir(
  px_backend_lib,
  sources: px_backend_sources,
  nsversion: api_version,
  namespace: 'Px',
  symbol_prefix: 'px',
  identifier_prefix: 'Px',
  includes: [ 'Gio-2.0' ],
  install: false,
)

px_backend_dep = declare_dependency(
  include_directories: px_backend_inc,
  link_with: px_backend_lib,
  dependencies: px_backend_deps
)

subdir('dbus')
subdir('plugins')

summary({
  'D-Bus Service' : build_dbus,
}, section: 'Architecture')