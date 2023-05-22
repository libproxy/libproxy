if get_option('tests')
  envs = [
    'G_TEST_SRCDIR=' + meson.current_source_dir(),
    'G_TEST_BUILDDIR=' + meson.current_build_dir(),
    'GSETTINGS_BACKEND=memory',
  ]

  test_cargs = ['-UG_DISABLE_ASSERT']

  libproxy_test = executable('test-libproxy',
    ['libproxy-test.c'],
    include_directories: libproxy_inc,
    dependencies: [libproxy_dep],
  )
  test('Libproxy test',
       libproxy_test,
       env: envs
  )

  if get_option('pacrunner-duktape')
    px_manager_test = executable('test-px-manager',
      ['px-manager-test.c', 'px-manager-helper.c'],
      include_directories: px_backend_inc,
      dependencies: [glib_dep, px_backend_dep],
    )
    test('PX Manager test',
         px_manager_test,
         env: envs
    )
  endif

  if get_option('config-env')
    config_env_test = executable('test-config-env',
      ['config-env-test.c', 'px-manager-helper.c'],
      include_directories: px_backend_inc,
      dependencies: [glib_dep, px_backend_dep],
    )
    test('Config Environment test',
         config_env_test,
         env: envs
    )
  endif

  if get_option('config-sysconfig')
    config_sysconfig_test = executable('test-config-sysconfig',
      ['config-sysconfig-test.c', 'px-manager-helper.c'],
      include_directories: px_backend_inc,
      dependencies: [glib_dep, px_backend_dep],
    )
    test('Config sysconfig test',
         config_sysconfig_test,
         env: envs
    )
  endif

  if get_option('config-gnome')
    config_gnome_test = executable('test-config-gnome',
      ['config-gnome-test.c', 'px-manager-helper.c'],
      include_directories: px_backend_inc,
      dependencies: [glib_dep, px_backend_dep],
    )
    test('Config GNOME test',
         config_gnome_test,
         env: [envs, 'XDG_CURRENT_DESKTOP=GNOME'],
    )
  endif

  if get_option('config-kde')
    config_kde_test = executable('test-config-kde',
      ['config-kde-test.c', 'px-manager-helper.c'],
      include_directories: px_backend_inc,
      dependencies: [glib_dep, px_backend_dep],
    )
    test('Config KDE test',
         config_kde_test,
         env: [envs, 'XDG_CURRENT_DESKTOP=KDE'],
    )
  endif

  if get_option('config-osx') and with_platform_darwin
    config_osx_test = executable('test-config-osx',
      ['config-osx-test.c', 'px-manager-helper.c'],
      include_directories: px_backend_inc,
      dependencies: [glib_dep, px_backend_dep],
    )
    test('Config OSX test',
         config_osx_test,
         env: [envs],
    )
  endif

  if get_option('config-windows') and with_platform_windows
    config_windows_test = executable('test-config-windows',
      ['config-windows-test.c', 'px-manager-helper.c'],
      include_directories: px_backend_inc,
      dependencies: [glib_dep, px_backend_dep],
    )
    test('Config Windows test',
         config_windows_test,
         env: [envs],
    )
  endif
endif
