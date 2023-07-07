project('libproxy', 'c',
          version: '0.5.3',
    meson_version: '>= 0.59.0',
  default_options: [ 'warning_level=2', 'werror=false', 'c_std=gnu11', ],
)

version_arr = meson.project_version().split('-')[0].split('.')
libproxy_version_major = version_arr[0].to_int()
libproxy_version_minor = version_arr[1].to_int()

root_dir = include_directories('.')

px_prefix = get_option('prefix')
datadir = get_option('datadir')
pkglibdir = join_paths(px_prefix, get_option('libdir'), 'libproxy')
girdir     = get_option('datadir') / 'gir-1.0'
typelibdir = get_option('libdir')  / 'girepository-1.0'
vapidir = get_option('datadir') / 'vala' / 'vapi'

add_project_arguments(['-I' + meson.project_build_root()], language: 'c')

# The major api version as encoded in the libraries name
api_version = '1.0'
package_api_name = '@0@-@1@'.format(meson.project_name(), api_version)

cc = meson.get_compiler('c')

project_c_args = []
test_c_args = [
  '-Wcast-align',
  '-Wdeclaration-after-statement',
  '-Werror=address',
  '-Werror=array-bounds',
  '-Werror=empty-body',
  '-Werror=implicit',
  '-Werror=implicit-function-declaration',
  '-Werror=incompatible-pointer-types',
  '-Werror=init-self',
  '-Werror=int-conversion',
  '-Werror=int-to-pointer-cast',
  '-Werror=main',
  '-Werror=misleading-indentation',
  '-Werror=missing-braces',
  '-Werror=missing-include-dirs',
  '-Werror=nonnull',
  '-Werror=overflow',
  '-Werror=parenthesis',
  '-Werror=pointer-arith',
  '-Werror=pointer-to-int-cast',
  '-Werror=redundant-decls',
  '-Werror=return-type',
  '-Werror=sequence-point',
  '-Werror=shadow',
  '-Werror=strict-prototypes',
  '-Werror=trigraphs',
  '-Werror=undef',
  '-Werror=write-strings',
  '-Wformat-nonliteral',
  '-Wignored-qualifiers',
  '-Wimplicit-function-declaration',
  '-Wlogical-op',
  '-Wmissing-declarations',
  '-Wmissing-format-attribute',
  '-Wmissing-include-dirs',
  '-Wmissing-noreturn',
  '-Wnested-externs',
  '-Wno-cast-function-type',
  '-Wno-dangling-pointer',
  '-Wno-missing-field-initializers',
  '-Wno-sign-compare',
  '-Wno-unused-parameter',
  '-Wold-style-definition',
  '-Wpointer-arith',
  '-Wredundant-decls',
  '-Wstrict-prototypes',
  '-Wswitch-default',
  '-Wswitch-enum',
  '-Wundef',
  '-Wuninitialized',
  '-Wunused',
  '-fno-strict-aliasing',
  ['-Werror=format-security', '-Werror=format=2'],
]

foreach arg: test_c_args
  if cc.has_multi_arguments(arg)
    project_c_args += arg
  endif
endforeach
add_project_arguments(project_c_args, language: 'c')

_platforms = []
host_system = host_machine.system()
if host_machine.system() == 'windows'
    _platforms = ['windows']
endif
with_platform_windows = _platforms.contains('windows')
with_platform_darwin = ['darwin', 'ios'].contains(host_system)

module_suffix = []
# Keep the autotools convention for shared module suffix because GModule
# depends on it.
if with_platform_darwin
  module_suffix = 'so'
endif

glib_dep = dependency('glib-2.0', version: '>= 2.71.3')
gio_dep = dependency('gio-2.0', version: '>= 2.71.3')
curl_dep = dependency('libcurl', required: get_option('curl'))
ws2_32_dep = cc.find_library('ws2_32', required : with_platform_windows)
gsettings_desktop_schema = dependency('gsettings-desktop-schemas', required: get_option('config-gnome'))

config_h = configuration_data()
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set('HAVE_CURL', get_option('curl'))
configure_file(output: 'config.h', configuration: config_h)

subdir('src')
subdir('tests')
subdir('docs')

summary({
  'host cpu' : host_machine.cpu_family(),
  'host endian' : host_machine.endian(),
  'host system' : host_system,
  'C Compiler' : cc.get_id(),
}, section: 'Build environment')

summary({
  'Vapi' : get_option('vapi'),
}, section: 'Options')

# Install pre-commit hook
hook = run_command(join_paths(meson.project_source_root(), 'data/install-git-hook.sh'), check: false)
if hook.returncode() == 0
  message(hook.stdout().strip())
endif
