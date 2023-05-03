proxy_sources = [
  'proxy.c'
]

executable(
  'proxy',
  sources: proxy_sources,
  dependencies: libproxy_dep,
  install: true,
  include_directories: libproxy_inc
)

install_man('proxy.8')