FROM fredrikekre/julia-alpine:1.4.1 as julia_builder
FROM pdal/alpinebase:latest as pdal_builder

ENV JULIA_PATH /usr/local/julia
ENV PATH $JULIA_PATH/bin:$PATH
ENV PDAL_DRIVER_PATH=/usr/local/lib/

# Copy the Julia binary into the PDAL image
COPY --from=julia_builder /usr/local/julia $JULIA_PATH

# Copy julia dependency libraries into path where the embedded Julia runtime can access them
RUN cp $JULIA_PATH/lib/julia/* $JULIA_PATH/lib;

# Clone and build PDAL 
RUN    \
    git clone https://github.com/PDAL/PDAL.git --branch master --single-branch ~/pdal; \
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

# Build dynamic library and copy into /usr/local/lib for detection by PDAL
RUN    \
    git clone https://github.com/cognitive-earth/PDAL-julia.git --branch master --single-branch ~/PDAL-julia; \
    cd ~/PDAL-julia; \
    julia build_sys.jl; \
    cd ~/PDAL-julia; \
    mkdir pdal/build; \
    cd pdal/build; \
    cmake .. -G Ninja \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DCMAKE_INSTALL_LIBDIR=lib \
        ; \
    ninja install; \
    cp ./libpdal_plugin_filter_julia.so $PDAL_DRIVER_PATH;
    # cp ./pdal_jl_sys.so $PDAL_DRIVER_PATH; \


# Run PDAL-julia test suite
RUN cd ~/PDAL-julia/pdal; ./build/julia_filter_test;

# TODO: So far, we can't run the filter unless we're in the ~/PDAL-julia/pdal folder
#
# This prevents us from just copying the pdal and julia lib/bin folders across to another
# image and using them without all the mess here
RUN    \
  cd ~/PDAL-julia/pdal; \
  cp $JULIA_PATH/lib/julia/* /usr/local/lib; \
  pdal translate /root/PDAL-julia/pdal/test/data/1.2-with-color.las ./julia-out.las julia \
    --filters.julia.script=/root/PDAL-julia/pdal/test/data/test1.jl \
    --filters.julia.module="TestModule" \
    --filters.julia.function="fff";

# RUN    \
#     rm -rf ~/pdal; \
#     rm -rf ~/PDAL-julia; \
#     apk del \
#         git \
#         cmake \
#         ninja \
#         g++ \
#     ; \
#     rm -rf /usr/share/hdf5_examples; \
#     rm /usr/lib/*.a; \
#     rm /usr/bin/gdal*; \
#     rm /usr/bin/ogr*; \
#     rm /usr/bin/h5*;

# Finally, construct a minimal image with the filter available
FROM alpine:3.11

COPY --from=pdal_builder /usr/ /usr/
COPY --from=pdal_builder /lib/ /lib/
COPY --from=pdal_builder /root/PDAL-julia/pdal/test/data/ /tmp/data/

# Fails to find the shared libraries for Julia as we're not in ~/PDAL-julia/pdal (and we dont want it in this image)
RUN    \
  pdal translate /tmp/data/1.2-with-color.las ./julia-out.las julia \
    --filters.julia.script=/tmp/data/test1.jl \
    --filters.julia.module="TestModule" \
    --filters.julia.function="fff";
