plugin_name = 'pacrunner-duktape'

if get_option(plugin_name)

duktape_dep = dependency('duktape')
m_dep = cc.find_library('m', required : false)
socket_dep = cc.find_library('socket', required: false)

px_backend_sources += [
  'plugins/@0@/@0@.c'.format(plugin_name),
]

px_backend_deps += [
  duktape_dep,
  m_dep,
  socket_dep
]

endif
