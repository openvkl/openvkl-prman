# Intel® Open VKL plugin for RenderMan*

The Intel® Open VKL plugin for RenderMan* implements the implicit field
interface for Pixar's RenderMan. Through this plugin, applications can load
OpenVDB files as Open VKL volumes, and gain both additional functionality and
improved performance through use of Intel® Open Volume Kernel Library (Intel®
Open VKL).

# Building

Building the plugin requires Pixar's RenderMan, Open VKL, and OpenVDB.

## Building Open VKL and OpenVDB

The Open VKL superbuild can be used to build Open VKL, OpenVDB, and their
dependencies:

```sh
git clone https://github.com/openvkl/openvkl.git ./openvkl

mkdir openvkl_build
cd openvkl_build

cmake -D TBB_VERSION=2020.3 -D TBB_HASH="" \
  -D OPENVKL_EXTRA_OPTIONS="-DBUILD_EXAMPLES=OFF -DBUILD_TESTING=OFF" \
  ../openvkl/superbuild

cmake --build .

cd ..
```

After running the above, the installed components can be found in
`openvkl_build/install/`.

## Building the Plugin

The plugin requires a RenderMan installation to build. Set the environment as
appropriate for your system, e.g.:

```sh
export RMANTREE=/opt/pixar/RenderManProServer-24.4
```

The other dependencies were already installed above. Set the environment so that
CMake may find these via:

```sh
export CMAKE_PREFIX_PATH=${PWD}/openvkl_build/install
```

Now, build the plugin:

```sh
mkdir build
cd build

cmake -D OpenVDB_ROOT=${CMAKE_PREFIX_PATH} ..

cmake --build .
```

The plugins should now be built, including the primary Open VKL plugin
(`libprman_openvkl`) and an auxiliary plugin used for benchmarking / comparison
purposes (`librif_benchmark`).

# Usage

## Volume types

The plugin supports two Open VKL volume types which can be used with any
imported VDB file:

- `structuredRegular`: a dense, regular grid
- `vdb`: a sparse grid, with internal representation similar to the OpenVDB data
  structure

The `structuredRegular` representation may provide improved performance,
especially for small volumes; however its dense representation can require more
memory. Conversely, the `vdb` representation is more memory efficient.

Additionally, the `auto` volume type may be used, which will choose the
`structuredRegular` representation if the memory overhead is sufficiently small,
and otherwise default to `vdb`. This feature is experimental.

Open VKL supports several additional volume types, but these are not exposed in
the plugin at this time.

## RIB file syntax

The plugin supports rendering of OpenVDB data as a volume. Three arguments are
required: the VDB filename, the Open VKL volume type to use, and the grid name
from the OpenVDB file to import. JSON arguments for density scaling
(`densityMult`, `densityRolloff`) are optional. Primvars from additional OpenVDB
grids can also be imported. Below are examples of the RIB file syntax.

### Density field using VDB volume type

```
Volume "blobbydso:../build/plugins/openvkl/libprman_openvkl.so"
    [-40 40 -3 100 -40 40]
    [0 0 0]
    "constant string[3] blobbydso:stringargs" ["smoke.vdb" "vdb" "density"]
    "varying float density" []
```

### Density field using structuredRegular volume type

```
Volume "blobbydso:../build/plugins/openvkl/libprman_openvkl.so"
    [-40 40 -3 100 -40 40]
    [0 0 0]
    "constant string[3] blobbydso:stringargs" ["smoke.vdb" "structuredRegular" "density"]
    "varying float density" []
```

### Density field using VDB volume type, density scaling, and additional primvars

```
Volume "blobbydso:../build/plugins/openvkl/libprman_openvkl.so"
    [-6.881570 12.118430 -1.200000 59.700001 -7.939096 9.760904]
    [0 0 0]
    "constant string[4] blobbydso:stringargs" [
        "smoke2.vdb"
        "vdb"
        "density"
        "{\"densityMult\": 1.0,  \"densityRolloff\": 0.0}"]
    "varying float density" []
    "varying vector v" []
```

## RIB filter plugin: rif_benchmark

A RIB filter plugin is also included, which can dynamically substitute which
volume plugin is used, and for the Open VKL plugin also select the volume type
used. The filter plugin assumes RIB files written for the `impl_openvdb` plugin
syntax. Thus, this filter plugin can be used to conveniently render existing RIB
files written for the `impl_openvdb` plugin with the Open VKL-based plugin, or
for comparison purposes between different plugins and volume types.

Below are examples of how to use the RIB filter plugin, using RIB files included
in the `scenes/` directory. Note that the VDB data sets are not included in this
repository; see the `scenes/README.md` for details.

Note that you may need to set your library search path for the dependencies
built above. This can be done as:

```sh
export LD_LIBRARY_PATH=${PWD}/openvkl_build/install/lib:${LD_LIBRARY_PATH}
```

### Rendering using the Open VKL plugin, VDB volume type

```sh
prman -rif ../build/plugins/rif_benchmark/librif_benchmark.so \
  -rifargs -plugin ../build/plugins/openvkl/libprman_openvkl.so -type vdb \
  -rifend -statsfile smoke_openvkl_vdb.xml \
  smoke.rib
```

### Rendering using the Open VKL plugin, structuredRegular volume type

```sh
prman -rif ../build/plugins/rif_benchmark/librif_benchmark.so \
  -rifargs -plugin ../build/plugins/openvkl/libprman_openvkl.so -type structuredRegular \
  -rifend -statsfile smoke_openvkl_structuredRegular.xml \
  smoke.rib
```

### Display filtering

The `rif_benchmark` plugin can also modify display outputs from the RIB file.
These options, disabled by default, are:

- `-filterDisplay`: redirects display output to TIFF files, named according to
  the current volume plugin and volume type
- `-it`: enables interactive display (via the `it` tool) of rendering in
  progress

These options can be used in conjunction with the other arguments above, e.g.:

```sh
prman -rif ../build/plugins/rif_benchmark/librif_benchmark.so \
  -rifargs -plugin ../build/plugins/openvkl/libprman_openvkl.so -type vdb \
  -it \
  -rifend -statsfile smoke_openvkl_vdb.xml \
  smoke.rib
```

# Known Issues

Below are currently known issues with this plugin. If you encounter any other
issues, please create an issue on GitHub.

- Performance with aggregate volumes: the current
  `OpenVKLImplicitField::Range()` implementation may return value ranges that
  are conservatively large, which can result in more sampling than necessary
  when using RenderMan's aggregate volume functionality. We are working to
  improve this.
