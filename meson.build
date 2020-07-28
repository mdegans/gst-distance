# source of truth for package info 
# (changing these changes config_meson.h and many other things)
project('gst-distance', ['cpp'],
  version: run_command(
    'head', '-n', '1', files('VERSION'),
  ).stdout().strip(),
  meson_version: '>= 0.54.0',
  license : 'LGPL',
  default_options: [
    'warning_level=2',  # -Wall
  ]
)

gst_api_version = '1.0'
package_name = 'gstdistance'
package_long_name = package_name + '-' + gst_api_version
package_description = 'Social distancing plugins for DeepStream.'
package_version = meson.project_version()
package_author = 'Michael de Gans'
package_email = 'michael.john.degans@gmail.com'
package_uri = 'https://github.com/mdegans/gst-distance'

# get the cpp compiler
cc = meson.get_compiler('cpp')

# extra cflags:
add_project_arguments(
  # '-Wall',  # implied by warning_level above
  '-Wshadow',
  '-Werror',
  '-fno-exceptions',
  language : 'cpp'
)

# calculate and print the plugin install dir
plugins_install_dir = join_paths(get_option('libdir'), 'gstreamer-1.0')
message('gst plugin dir: ' + plugins_install_dir)

# include subdirs
subdir('gst-distance')
