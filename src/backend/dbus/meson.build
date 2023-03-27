if build_dbus
  gdbus_codegen = find_program('gdbus-codegen')

  unitdir = ''
  dbus_data_dir = join_paths(get_option('prefix'), get_option('datadir'), 'dbus-1')
  dbus_interfaces_dir = join_paths(dbus_data_dir, 'interfaces')
  dbus_user_services_dir = join_paths(dbus_data_dir, 'services')
  dbus_system_services_dir = join_paths(dbus_data_dir, 'system-services')
  dbus_system_conf_dir = join_paths(dbus_data_dir, 'system.d')

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

  # D-Bus Interface
  install_data('org.libproxy.proxy.xml', install_dir : dbus_interfaces_dir)

  # D-Bus User Service
  user_service_data = configuration_data()
  user_service_data.set('LIBEXECDIR', join_paths(px_prefix, get_option('libexecdir')))
  configure_file(
    input: 'org.libproxy.proxy.service.in',
    output: 'org.libproxy.proxy.service',
    configuration: user_service_data,
    install: true,
    install_dir: dbus_user_services_dir
  )

  # D-Bus System Service
  system_service_data = configuration_data()
  system_service_data.set('LIBEXECDIR', join_paths(px_prefix, get_option('libexecdir')))
  system_service_data.set('USER', get_option('dbus-system-user'))
  dbus_system_service = configure_file(
    input: 'org.libproxy.proxy-system.service.in',
    output: 'org.libproxy.proxy-system.service',
    configuration: system_service_data
  )
  install_data(dbus_system_service, rename : 'org.libproxy.proxy.service', install_dir : dbus_system_services_dir)

  dbus_config_data = configuration_data()
  dbus_config_data.set('daemon_user', get_option('dbus-system-user'))
  configure_file(
    input: 'org.libproxy.proxy.conf.in',
    output: 'org.libproxy.proxy.conf',
    configuration: dbus_config_data,
    install: true,
    install_dir: dbus_system_conf_dir
  )
endif
