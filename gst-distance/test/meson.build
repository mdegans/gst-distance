
# a list of dicts describing tests
tests = [
  {
    'description': 'Test dsdistance element using GstCheck     ',
    'filename': 'test_gstdsdistance',
    'sources': ['test_gstdsdistance.cpp'],
  },
  {
    'description': 'Test dsprotopayload element using GstCheck ',
    'filename': 'test_gstdsprotopayload',
    'sources': ['test_gstdsprotopayload.cpp'],
  },
  {
    'description': 'Test payloadbroker element using GstCheck  ',
    'filename': 'test_gstdspayloadbroker',
    'sources': ['test_gstdspayloadbroker.cpp'],
  },
]

# check for check (outside the loop to avoid printing twice)
gst_check_dep = dependency('gstreamer-check-1.0')

# build and run tests (on ninja test)
foreach t: tests
  exe = executable(t['filename'], t['sources'],
    dependencies: [deps, gst_check_dep],
    link_with: gst_distance,
    include_directories: plugin_incdir,
  )
  test(t['description'], exe,
    is_parallel: false,
    # TODO(mdegans): research if there is a dedicated parent directory function
    env: [
      'GST_DEBUG=4',
      'GST_PLUGIN_PATH=' + meson.current_build_dir() + '/../',
      'GST_REGISTRY_UPDATE=yes',
    ],
  )
endforeach