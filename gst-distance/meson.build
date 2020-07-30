# tell the preprocessor we have config.h and build it
subdir('config_h')
config_incdir = include_directories('config_h')

# plugin sources
plugin_incdir = include_directories('include')
plugin_sources = [
  'src/gstdistance.cpp',     # common plugin code
  'src/gstdsdistance.cpp',     # dsdistance Element
  'src/gstdsprotopayload.cpp', # dsprotopayload Element
  'src/gstdspayloadbroker.cpp',  # dspayloadbroker Element
]

# libdistance, libdistanceproto
distance_dep = dependency('distance',
  version: '>=0.1.1',
  required: false,
)
if not distance_dep.found()
  distance_proj = subproject('distance')
  distance_dep = distance_proj.get_variable('distance_dep')
endif


# plugin dependencies
deps = [
  dependency('gstreamer-1.0'),
  dependency('gstreamer-base-1.0'),
  distance_dep,
]

# plugin library target
gst_distance = shared_library(
  # library name, sources
  package_name, plugin_sources,
  dependencies: deps,
  include_directories: [plugin_incdir, config_incdir],
  install: true,
  install_dir: plugins_install_dir,
)

# this is a dependency for subproects, designed for use with
# a dummy target just to make sure it builds and installs.
gst_distance_dep = declare_dependency(
  dependencies: deps,
  link_with: gst_distance,
)

# pkg-config
# this exists mostly for subprojects to check if the plugin
# is installed.
pkg = import('pkgconfig')
distance_pc = pkg.generate(gst_distance,
  version: meson.project_version(),
  description: package_description,
  url: package_uri,
  # for consistency with existing cmake install:
  install_dir: get_option('datadir') / 'pkgconfig'
)

# add test subdir
subdir('test')