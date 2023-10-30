set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# for Mpp build
add_definitions(-fPIC)
add_definitions(-DARMLINUX)
add_definitions(-Dlinux)

set(target_arch aarch64-linux-gnu)
set(CMAKE_LIBRARY_ARCHITECTURE ${target_arch} CACHE STRING "" FORCE)

# Configure cmake to look for libraries, include directories and
# packages inside the target root prefix.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# setup compiler for cross-compilation
set(CMAKE_CXX_FLAGS           "-fPIC"               CACHE STRING "c++ flags")
set(CMAKE_C_FLAGS             "-fPIC"               CACHE STRING "c flags")
set(CMAKE_SHARED_LINKER_FLAGS ""                    CACHE STRING "shared linker flags")
set(CMAKE_MODULE_LINKER_FLAGS ""                    CACHE STRING "module linker flags")
set(CMAKE_EXE_LINKER_FLAGS    ""                    CACHE STRING "executable linker flags")

# needed to avoid doing some more strict compiler checks that
# are failing when cross-compiling
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# specify the toolchain programs
set(CMAKE_C_COMPILER            ${toolchain_prefix}gcc)
set(CMAKE_CXX_COMPILER          ${toolchain_prefix}g++)
set(CMAKE_AR                    ${toolchain_prefix}ar)
set(CMAKE_AS                    ${toolchain_prefix}as)
set(CMAKE_NM                    ${toolchain_prefix}nm)
set(CMAKE_STRIP                 ${toolchain_prefix}strip)
set(CMAKE_OBJCOPY               ${toolchain_prefix}objcopy)
set(CMAKE_OBJDUMP               ${toolchain_prefix}objdump)
set(CMAKE_RANLIB                ${toolchain_prefix}ranlib)
set(CMAKE_LINKER                ${toolchain_prefix}ld)


# Not all shared libraries dependencies are instaled in host machine.
# Make sure linker doesn't complain.
set(CMAKE_EXE_LINKER_FLAGS_INIT -Wl,--allow-shlib-undefined)
