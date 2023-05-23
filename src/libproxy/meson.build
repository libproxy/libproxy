libproxy_inc = include_directories('.')

libproxy_sources = []

libproxy_sources = [
  'proxy.c',
]

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

libproxy = shared_library(
  'proxy',
  libproxy_sources,
  include_directories: px_backend_inc,
  dependencies: libproxy_deps,
  link_args : vflag,
  link_depends : mapfile,
  soversion: '1',
  install: true,
  install_rpath: pkglibdir,
)

libproxy_dep = declare_dependency (
  include_directories: libproxy_inc,
  link_with: libproxy,
  dependencies: libproxy_deps
)

install_headers(libproxy_headers, subdir: 'libproxy')

pkg = import('pkgconfig')
pkg.generate(
  libraries: [libproxy],
  subdirs: 'libproxy',
  version: meson.project_version(),
  name: 'libproxy',
  filebase: package_api_name,
  description: 'libproxy',
  requires: 'gio-2.0',
  install_dir: join_paths(get_option('libdir'), 'pkgconfig')
)

if get_option('introspection')
  gnome = import('gnome')

  libproxy_gir_extra_args = [
    '--c-include=proxy.h',
    '--quiet',
  ]

  libproxy_gir = gnome.generate_gir(
    libproxy,
    sources: libproxy_headers + libproxy_sources,
    nsversion: api_version,
    namespace: 'Libproxy',
    export_packages: package_api_name,
    symbol_prefix: 'px',
    identifier_prefix: 'px',
    link_with: libproxy,
    includes: [ 'Gio-2.0' ],
    install: true,
    install_dir_gir: girdir,
    install_dir_typelib: typelibdir,
    extra_args: libproxy_gir_extra_args,
  )

  if get_option('vapi')
    libproxy_vapi = gnome.generate_vapi(package_api_name,
         sources: libproxy_gir[0],
        packages: [ 'gio-2.0' ],
         install: true,
     install_dir: vapidir,
    metadata_dirs: [ meson.current_source_dir() ],
    )
  endif
endif