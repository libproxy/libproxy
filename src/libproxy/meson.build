api_version = '1.0'

libproxy_inc = include_directories('.')

libproxy_sources = []

if build_dbus
libproxy_sources += [
  'proxy-dbus.c',
]

else
libproxy_sources += [
  'proxy.c',
]
endif

libproxy_headers = [
  'proxy.h',
]

libproxy_deps = [
  px_backend_dep,
]

mapfile = 'libproxy.map'
vscript = '-Wl,--version-script,@0@/@1@'.format(meson.current_source_dir(), mapfile)
vflag = []
if cc.has_multi_link_arguments(vscript)
  vflag += vscript
endif

libproxy_lib = shared_library(
  'proxy',
  libproxy_sources,
  include_directories: px_backend_inc,
  dependencies: libproxy_deps,
  link_args : vflag,
  link_depends : mapfile,
  soversion: '1',
  install: true,
)

libproxy_dep = declare_dependency (
  include_directories: libproxy_inc,
  link_with: libproxy_lib,
  dependencies: libproxy_deps
)

install_headers(libproxy_headers, subdir: 'libproxy')

pkg = import('pkgconfig')
pkg.generate(
  description: 'A shared library for libproxy',
  libraries: libproxy_lib,
  name: 'libproxy',
  filebase: 'libproxy-' + api_version,
  version: meson.project_version(),
  subdirs: 'libproxy',
  requires: 'gio-2.0',
  install_dir: join_paths(get_option('libdir'), 'pkgconfig')
)

gnome = import('gnome')
libproxy_gir = gnome.generate_gir(
  libproxy_lib,
  sources: libproxy_headers + libproxy_sources,
  nsversion: api_version,
  namespace: 'Libproxy',
  symbol_prefix: 'libproxy',
  identifier_prefix: 'Libproxy',
  includes: [ 'Gio-2.0' ],
  install: true,
)
