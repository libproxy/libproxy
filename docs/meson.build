if get_option('docs')

expand_content_md_files = [
  'architecture.md',
  'applications.md',
  'build-steps.md',
  'configuration-logic.md',
  'perl.md',
  'python.md',
  'ruby.md',
  'vala.md',
]

toml_data = configuration_data()
toml_data.set('VERSION', meson.project_version())

px_toml = configure_file(
  input: 'px.toml.in',
  output: 'px.toml',
  configuration: toml_data
)

gidocgen = find_program('gi-docgen')

docs_dir = datadir / 'doc'

custom_target('px-doc',
  input: [ px_toml, libproxy_gir[0] ],
  output: 'libproxy-@0@'.format(api_version),
  command: [
    gidocgen,
    'generate',
    '--quiet',
    '--add-include-path=@0@'.format(meson.current_source_dir() + '/src'),
    '--config=@INPUT0@',
    '--output-dir=@OUTPUT@',
    '--no-namespace-dir',
    '--content-dir=@0@'.format(meson.current_source_dir()),
    '@INPUT1@',
  ],
  depend_files: [ expand_content_md_files ],
  build_by_default: true,
  install: true,
  install_dir: docs_dir,
)

endif
