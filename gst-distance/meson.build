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
gst_cuda_plugin = shared_library(
  # library name, sources
  'gstdistance', plugin_sources,
  dependencies: deps,
  include_directories: [plugin_incdir, config_incdir],
  install: true,
  install_dir: plugins_install_dir,
)

# add test subdir
subdir('test')