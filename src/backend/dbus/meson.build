if build_dbus
  gdbus_codegen = find_program('gdbus-codegen')

  unitdir = ''
  dbus_data_dir = join_paths(get_option('prefix'), get_option('datadir'), 'dbus-1')
  dbus_interfaces_dir = join_paths(dbus_data_dir, 'interfaces')
  dbus_services_dir = join_paths(dbus_data_dir, 'services')
  dbus_sysconf_dir = join_paths(dbus_data_dir, 'session.d')

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
    c_args: px_backend_c_args,
    dependencies: proxyd_deps,
    install_dir: join_paths(px_prefix, get_option('libexecdir')),
    install: true,
  )

  service_data = configuration_data()
  service_data.set('LIBEXECDIR', join_paths(px_prefix, get_option('libexecdir')))

  dbus_service = configure_file(
    input: 'org.libproxy.proxy.service.in',
    output: 'org.libproxy.proxy.service',
    configuration: service_data
  )

  install_data('org.libproxy.proxy.conf', install_dir : dbus_sysconf_dir)
  install_data(dbus_service, install_dir : dbus_services_dir)
  install_data('org.libproxy.proxy.xml', install_dir : dbus_interfaces_dir)

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
