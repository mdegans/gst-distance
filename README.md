# gst-distance

Is a simple social distancing plugin for Nvidia DeepStream. It was written prior
to the advent of nvanalytics and feature it's own metadata format.

## Requirements:

### Hardware

* An Nvidia dGPU supported by DeepStream (GeForce works, but is not officially supported)

(or)

* A Nvidia Tegra board of your preference.

### Software

* DeepStream 5.0 (obtaining this on any platform is harder than it should be)

* Other software: on Ubuntu / Debian / Linux for Tegra:
    ```bash
    sudo apt install \
        libglib2.0-dev \
        libgstreamer-plugins-base1.0-dev \
        libgstreamer1.0-dev \
        ninja-build \
        python3-pip
    pip3 install --upgrade meson
    ```

## Building
```bash
git clone --branch (branch) (repo url)
cd (repo folder)
mkdir build && cd build
meson (options) ..
ninja test
(sudo) ninja install
```

A systemwide install is not necessary if you tell meson to configure a `--prefix` like `~/.local` and set GST_PLUGIN_PATH accordingly. `ninja uninstall` can be used to uninstall.