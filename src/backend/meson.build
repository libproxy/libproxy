backend_config_h = configuration_data()
backend_config_h.set('HAVE_CONFIG_ENV', get_option('config-env'))
backend_config_h.set('HAVE_CONFIG_GNOME', get_option('config-gnome'))
backend_config_h.set('HAVE_CONFIG_KDE', get_option('config-kde'))
backend_config_h.set('HAVE_CONFIG_OSX', get_option('config-osx') and with_platform_darwin)
backend_config_h.set('HAVE_CONFIG_SYSCONFIG', get_option('config-sysconfig'))
backend_config_h.set('HAVE_CONFIG_WINDOWS', get_option('config-windows') and with_platform_windows)
backend_config_h.set('HAVE_CONFIG_XDP', get_option('config-xdp'))
backend_config_h.set('HAVE_PACRUNNER_DUKTAPE', get_option('pacrunner-duktape'))
backend_config_h.set('HAVE_CURL', get_option('curl'))
configure_file(output: 'px-backend-config.h', configuration: backend_config_h)

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
  ws2_32_dep,
]

px_backend_c_args = [
  '-DG_LOG_DOMAIN="pxbackend"',
]

px_backend_inc = include_directories('.')

subdir('plugins')

px_backend = shared_library(
  'pxbackend-@0@'.format(api_version),
  px_backend_sources,
  dependencies: px_backend_deps,
  c_args: px_backend_c_args,
  install: true,
  install_dir: pkglibdir,
  install_rpath: pkglibdir
)

px_backend_dep = declare_dependency(
  include_directories: px_backend_inc,
  link_with: px_backend,
  dependencies: px_backend_deps
)
