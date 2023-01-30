if get_option('dbus')
  gdbus_codegen = find_program('gdbus-codegen')

  unitdir = ''
  systemd = dependency('systemd', required: false)
  if systemd.found()
    unitdir = systemd.get_variable(pkgconfig: 'systemduserunitdir')
  endif

  px_interface = [
    'org.libproxy.proxy.xml'
  ]

  px_interface_h = custom_target(
    'px-interface.h',
    input: px_interface,
    output: ['px-interface.h'],
    command: [
      gdbus_codegen,
      '--interface-info-header',
      '--output', '@OUTPUT@',
      '@INPUT@'
    ]
  )

  px_interface_c = custom_target(
    'libproxy-iface.c',
    input: px_interface,
    output: ['px-interface.c'],
    command: [
      gdbus_codegen,
      '--interface-info-body',
      '--output', '@OUTPUT@',
      '@INPUT@'
    ],
  )

  proxyd_sources = [
    px_interface_c,
    px_interface_h,
    'dbus.c',
  ]

  proxyd_deps = [
    px_backend_dep
  ]

  executable(
    'proxyd',
    proxyd_sources,
    dependencies: proxyd_deps,
    install_dir: join_paths(px_prefix, get_option('libexecdir')),
    install: true,
  )

  service_data = configuration_data()
  service_data.set('LIBEXECDIR', join_paths(px_prefix, get_option('libexecdir')))

  service = configure_file(
    input: 'proxyd.service.in',
    output: 'proxyd.service',
    configuration: service_data
  )

  if unitdir != ''
    install_data(service, install_dir: unitdir)
  endif
endif
