plugin_name = 'config-sysconfig'

if get_option(plugin_name)

plugin_src = [
  '@0@.c'.format(plugin_name),
]

plugin_data = [
  '@0@.plugin'.format(plugin_name),
]

plugin_lib = shared_module(
  plugin_name,
  plugin_src,
  include_directories: px_backend_inc,
  c_args: px_backend_c_args,
  dependencies: [px_backend_dep],
  install_dir: join_paths(px_plugins_dir, plugin_name),
  install: true,
  name_suffix: module_suffix,
)

# Starting with Meson 0.64 this can be replaced with fs.copyfile
custom_target(
  '@0@-data'.format(plugin_name),
  input: plugin_data,
  output: plugin_data,
  command: ['cp', '@INPUT@', '@OUTDIR@'],
  build_by_default: true,
  install_dir: join_paths(px_plugins_dir, plugin_name),
  install: true,
)

endif
