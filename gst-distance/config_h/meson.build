# this file builds config.h

cdata = configuration_data()
cdata.set_quoted('GST_API_VERSION', gst_api_version)
cdata.set_quoted('GST_LICENSE', meson.project_license())
cdata.set_quoted('GST_PACKAGE_NAME', package_long_name)
cdata.set_quoted('GST_PACKAGE_ORIGIN', package_uri)
cdata.set_quoted('PACKAGE_AUTHOR', package_author)
cdata.set_quoted('PACKAGE_DESCRIPTION', package_description)
cdata.set_quoted('PACKAGE_EMAIL', package_email)
cdata.set_quoted('PACKAGE_VERSION', package_version)
cdata.set_quoted('PACKAGE', package_name)
configure_file(output : 'config_meson.h', configuration : cdata)

add_project_arguments('-DHAVE_CONFIG_MESON_H', language : 'cpp')
