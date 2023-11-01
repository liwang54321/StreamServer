#!/bin/bash
set -e

top_dir=$(cd $(dirname $0); pwd)
script_name=$(basename $0)

toolchain_prefix=${top_dir}/toolchain/gcc-arm-11.2-2022.02-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-

function build_depends() {
    if [[ ! "$(ls -A ${top_dir}/thirdparty/ZLMediaKit)" || ! "$(ls -A ${top_dir}/thirdparty/mpp)" ]]; then
        pushd ${top_dir} >/dev/null 2>&1
        git submodule update --init --recursive
        popd >/dev/null
    fi

    sed "s#\${toolchain_prefix}#${toolchain_prefix}#" ${top_dir}/tools/conan_profile_aarch64 >${top_dir}/build/conan_profile_aarch64

    echo "Start build mpp"
    conan remove -c mpp
    conan create \
        ${top_dir}/tools/conanfile_mpp.py \
        -pr:b ${top_dir}/tools/conan_profile_x86_64 \
        -pr:h ${top_dir}/build/conan_profile_aarch64 \
        --build missing

    echo "Start build ZLMediaKit"
    conan remove -c zlmeidiakit
    conan create \
        ${top_dir}/tools/conanfile_zlmediakit.py \
        -pr:b ${top_dir}/tools/conan_profile_x86_64 \
        -pr:h ${top_dir}/build/conan_profile_aarch64 \
        --build missing

    #  Build for local
    # sed "s#\${toolchain_prefix}#${toolchain_prefix}#" ${top_dir}/scripts/toolchain_aarch64.cmake > ${top_dir}/build/toolchain_aarch64.cmake
    # cmake -DCMAKE_BUILD_TYPE=Release \
    #     -B ${top_dir}/build/mpp \
    #     -S ${top_dir}/thirdparty/mpp \
    #     -DCMAKE_TOOLCHAIN_FILE=${top_dir}/build/toolchain_aarch64.cmake \
    #     -DCMAKE_INSTALL_PREFIX=${top_dir}/packages/mpp \
    #     -DHAVE_DRM=ON \
    #     -DBUILD_TEST=OFF \
    #     ${top_dir}/thirdparty/mpp

    # cmake --build ${top_dir}/build/mpp -j$(nproc)
    # cmake --build ${top_dir}/build/mpp --target install

    #  Build for local
    # pushd ${top_dir}/build/ZLMediaKit >/dev/null 2>&1
    # source ${top_dir}/build/ZLMediaKit/conanbuild.sh
    # cmake -S ${top_dir}/thirdparty/ZLMediaKit \
    #     -B . \
    #     -DCMAKE_INSTALL_PREFIX=${top_dir}/packages/ZLMediaKit \
    #     -DCMAKE_BUILD_TYPE=Release \
    #     -DCMAKE_TOOLCHAIN_FILE=${top_dir}/build/ZLMediaKit/conan_toolchain.cmake

    # cmake --build ${top_dir}/build/ZLMediaKit -j$(nproc)
    # cmake --build ${top_dir}/build/ZLMediaKit --target install

    # popd > /dev/null
}

function install_thirdparty()
{
    [ -d ${top_dir}/build ] || mkdir -p ${top_dir}/build
    sed "s#\${toolchain_prefix}#${toolchain_prefix}#" ${top_dir}/tools/conan_profile_aarch64 >${top_dir}/build/conan_profile_aarch64

    echo "Start Gen Conan Graph Info"
    conan graph info ${top_dir}/conanfile.py \
        -pr:b ${top_dir}/tools/conan_profile_x86_64 \
        -pr:h ${top_dir}/build/conan_profile_aarch64 \
        -f json >${top_dir}/scripts/app_graph_info.json > /dev/null 2>&1

    echo "Start Gen Conan Build Order"
    conan graph build-order ${top_dir}/conanfile.py \
        -pr:b ${top_dir}/tools/conan_profile_x86_64 \
        -pr:h ${top_dir}/build/conan_profile_aarch64 \
        -f json >${top_dir}/scripts/app_graph_build_order.json > /dev/null 2>&1

    conan install \
        ${top_dir}/conanfile.py \
        -pr:b ${top_dir}/tools/conan_profile_x86_64 \
        -pr:h ${top_dir}/build/conan_profile_aarch64 \
        -of ${top_dir}/build/ \
        -b missing
}

function configure() {
    local build_type=$1
    source ${top_dir}/build/conanbuild.sh
    cmake -B ${top_dir}/build/ \
        -S ${top_dir} \
        -DCMAKE_TOOLCHAIN_FILE=${top_dir}/build/conan_toolchain.cmake \
        -DCMAKE_INSTALL_PREFIX=${top_dir}/install \
        -DCMAKE_BUILD_TYPE=${build_type}
}

function build() {
    echo "Building..."
    [ $# -gt 0 ] && verbose='-v'
    cmake --build ${top_dir}/build/ \
        -j$(nproc) \
        ${verbose}
}

function install() {
    cmake --install ${top_dir}/build/
}

function setup_env() {
    if [[ ! -d ${top_dir}/toolchain && $(uname -m) == "x86_64" ]]; then
        echo "Install Toolchain"
        mkdir -p ${top_dir}/toolchain
        [ ! -e ${top_dir}/packages/gcc-arm-11.2-2022.02-x86_64-aarch64-none-linux-gnu.tar.xz ] &&
            wget -N https://repo.jing.rocks/armbian-dl/_toolchain/gcc-arm-11.2-2022.02-x86_64-aarch64-none-linux-gnu.tar.xz -P ${top_dir}/packages

        tar -xf ${top_dir}/packages/gcc-arm-11.2-2022.02-x86_64-aarch64-none-linux-gnu.tar.xz -C ${top_dir}/toolchain
    fi

    if [ ! command -v conan ] &>/dev/null; then
        echo "Install python requirements.txt"
        pip3 config set global.index-url https://mirrors.aliyun.com/pypi/simple/
        sudo pip3 config set global.index-url https://mirrors.aliyun.com/pypi/simple/
        sudo pip3 install -r ${top_dir}/requirements.txt
        conan profile detect --force
    fi
}

function help() {
    echo "Usage: $0 [OPTIONS]"
cat <<EOF
    Options:"
        [ -h, --help ]                          Show this help message and exit
        [ -s, --setup_env ]                     Setup environment
        [ --build_depends ]                     Build depends
        [ --install_thirdparty ]                Install thirdparty
        [ -c, --configure [Release, Debug] ]    Configure 
        [ -b, --build [-v] ]                    Build 
        [ -i, --install ]                       Install
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
    -h | --help)
        help
        exit 0
        ;;
    -s | --setup_env)
        setup_env
        exit 0
        ;;
    --install_thirdparty)
        install_thirdparty
        exit 0
        ;;
    -c | --configure)
        configure $2
        shift
        exit 0
        ;;
    --build_depends)
        build_depends
        exit 0
        ;;
    -b | --build)
        build $2
        exit 0
        ;;
    -i | --install)
        install
        exit 0
        ;;
    *)
        echo "Unknown option: $1"
        help
        exit -1
        ;;
    esac
done
