FROM fredrikekre/julia-alpine:1.4.1 as julia_builder
FROM pdal/alpinebase:latest as pdal_builder

RUN apk add gdal-dev=3.1.0-r2

ENV JULIA_PATH /usr/local/julia
ENV PDAL_DRIVER_PATH /usr/local/lib
ENV PDAL_JULIA_RUNTIME_PATH $JULIA_PATH/lib
ENV PATH $JULIA_PATH/bin:$PATH
ENV JULIA_DEPOT_PATH $JULIA_PATH/packages

# Copy the Julia binary into the PDAL image
COPY --from=julia_builder /usr/local/julia $JULIA_PATH
RUN mkdir $JULIA_PATH/packages
RUN ln -s $JULIA_PATH/lib/julia/* $JULIA_PATH/lib;

# Clone and build PDAL 
RUN    \
    git clone https://github.com/jtfell/PDAL.git --branch jtfell-release --single-branch ~/pdal; \
    cd ~/pdal; \
    mkdir build; \
    cd build; \
    cmake .. \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_PLUGIN_NITF=ON \
        -DBUILD_PLUGIN_GREYHOUND=ON \
        -DBUILD_PLUGIN_CPD=ON \
        -DBUILD_PLUGIN_ICEBRIDGE=ON \
        -DBUILD_PLUGIN_PGPOINTCLOUD=ON \
        -DBUILD_PLUGIN_SQLITE=ON \
        -DBUILD_PLUGIN_I3S=ON \
        -DBUILD_PLUGIN_E57=ON \
        -DWITH_LASZIP=ON \
        -DWITH_LAZPERF=ON \
        -DWITH_LAZPERF=ON \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DCMAKE_INSTALL_LIBDIR=lib \
        ; \
    ninja install;

# Build dynamic library and copy relevant artifacts into /usr/local/lib for detection by PDAL
RUN    \
    git clone https://github.com/cognitive-earth/PDAL-julia.git --branch master --single-branch ~/PDAL-julia; \
    cd ~/PDAL-julia; \
    julia build_sys.jl;

RUN    \
    cd ~/PDAL-julia/pdal; \
    cmake . -G Ninja \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DCMAKE_INSTALL_LIBDIR=lib \
        ; \
    ninja install;

RUN    \
    cd ~/PDAL-julia/pdal; \
    cp ./libpdal_plugin_filter_julia.so $PDAL_DRIVER_PATH; \
    cp ./pdal_jl_sys.so $PDAL_DRIVER_PATH; \
    cp ../jl/PdalJulia.jl $PDAL_JULIA_RUNTIME_PATH; \
    ./julia_filter_test;

# Symlink julia libs into PDAL_DRIVER_PATH so PDAL can access them
RUN ln -s $JULIA_PATH/lib/julia/* $PDAL_DRIVER_PATH;

# Remove all the extra cruft from /usr and /lib before they are copied into the final image
RUN    \
    apk del \
        git \
        cmake \
        ninja \
        g++ \
    ; \
    rm -rf /usr/share/hdf5_examples; \
    rm /usr/lib/*.a; \
    rm /usr/bin/gdal*; \
    rm /usr/bin/ogr*; \
    rm /usr/bin/h5*;

RUN    \
  pdal translate /root/PDAL-julia/pdal/test/data/1.2-with-color.las ./julia-out.las julia \
    --filters.julia.script=/root/PDAL-julia/pdal/test/data/test1.jl \
    --filters.julia.module="TestModule" \
    --filters.julia.function="fff";
# RUN    \
#   pdal translate /root/PDAL-julia/pdal/test/data/1.2-with-color.las ./julia-out.las julia \
#     --filters.julia.script=/root/PDAL-julia/examples/Example2.jl \
#     --filters.julia.add_dimension="Num_Points_In_Radius" \
#     --filters.julia.module="TestModule" \
#     --filters.julia.function="runFilter";


# Finally, construct a minimal image with the filter available
FROM alpine:latest

COPY --from=pdal_builder /usr/ /usr/
COPY --from=pdal_builder /lib/ /lib/
# COPY --from=pdal_builder /root/PDAL-julia/pdal/test/data/ /tmp/data/

ENV JULIA_PATH /usr/local/julia
ENV PDAL_DRIVER_PATH /usr/local/lib
ENV PDAL_JULIA_RUNTIME_PATH $JULIA_PATH/lib
ENV LD_LIBRARY_PATH $PDAL_DRIVER_PATH:$LD_LIBRARY_PATH
ENV JULIA_DEPOT_PATH $JULIA_PATH/packages

# RUN    \
#   pdal translate /tmp/data/1.2-with-color.las ./julia-out.las julia \
#     --filters.julia.script=/tmp/data/test1.jl \
#     --filters.julia.module="TestModule" \
#     --filters.julia.function="fff";
