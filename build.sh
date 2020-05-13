export JULIA_PATH=/usr/bin/julia
export PDAL_DRIVER_PATH=/usr/local/lib
export PDAL_JULIA_RUNTIME_PATH=/usr/local/lib
# export JULIA_DEPOT_PATH=

# julia build_sys.jl
sudo cp ./pdal/pdal_jl_sys.so $PDAL_DRIVER_PATH
sudo cp ./jl/PdalJulia.jl $PDAL_JULIA_RUNTIME_PATH

# rm -rf pdal/build
# mkdir pdal/build
pushd pdal/build

cmake .. -G Ninja
ninja install

sudo cp ./libpdal_plugin_filter_julia.so $PDAL_DRIVER_PATH

popd

