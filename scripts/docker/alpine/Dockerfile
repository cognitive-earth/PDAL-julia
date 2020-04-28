FROM fredrikekre/julia-alpine:1.4.1 as julia_builder
FROM pdal/alpinebase:latest as pdal_builder

ENV JULIA_PATH /usr/local/julia
ENV PATH $JULIA_PATH/bin:$PATH

# Copy the Julia binary into the PDAL image
COPY --from=julia_builder /usr/local/julia /usr/local/julia

# May need the headers from here
# RUN git clone --progress https://github.com/JuliaLang/julia.git . && \
#     if [[ "$VERSION" ]]; then \
#         git checkout $VERSION; \
#     fi

# Build the PDAL-julia plugin
RUN    \
    git clone https://github.com/cognitive-earth/PDAL-julia.git --branch master --single-branch ~/PDAL-julia; \
    cd ~/PDAL-julia; \
    julia build_sys.jl; \
    cd pdal; \
    cmake .; \
    make;

# Install PDAL and copy dynamic library into the lib folder
RUN    \
    git clone https://github.com/PDAL/PDAL.git --branch master --single-branch pdal; \
    cd pdal; \
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
    \
    cp ~/PDAL-julia/pdal/libpdal_plugin_filter_julia.so ./lib; \
    cp -R ~/PDAL-julia/jl ../jl; \
    cp ~/PDAL-julia/pdal_jl_sys.so ..; \
    \
    ninja install; \
    cd /; \
    rm -rf pdal; \
    \
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

FROM alpine:3.11

COPY --from=pdal_builder /usr/ /usr/
COPY --from=pdal_builder /lib/ /lib/
