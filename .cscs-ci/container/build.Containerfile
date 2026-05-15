ARG DEPS_IMAGE
FROM $DEPS_IMAGE

COPY . /oomph
WORKDIR /oomph

ARG BACKEND
ARG NUM_PROCS
RUN spack -e ci build-env oomph -- \
        cmake -G Ninja -B build \
            -DCMAKE_BUILD_TYPE=Debug \
            -DOOMPH_WITH_TESTING=ON \
            -DOOMPH_WITH_$(echo $BACKEND | tr '[:lower:]' '[:upper:]')=ON \
            -DOOMPH_USE_BUNDLED_LIBS=ON \
            -DOOMPH_USE_BUNDLED_HWMALLOC=OFF \
            -DMPIEXEC_EXECUTABLE="" \
            -DMPIEXEC_NUMPROC_FLAG="" \
            -DMPIEXEC_PREFLAGS="" \
            -DMPIEXEC_POSTFLAGS="" && \
    spack -e ci build-env oomph -- cmake --build build -j$NUM_PROCS
