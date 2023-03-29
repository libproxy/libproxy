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
  '-DG_LOG_DOMAIN="pxbackend"',
]

px_backend_inc = include_directories('.')

px_backend = shared_library(
  'pxbackend-@0@'.format(api_version),
  px_backend_sources,
  dependencies: px_backend_deps,
  c_args: px_backend_c_args,
  install: true
)

px_backend_dep = declare_dependency(
  include_directories: px_backend_inc,
  link_with: px_backend,
  dependencies: px_backend_deps
)

subdir('plugins')