#!/bin/bash

set -e

path="$(pwd)/build"

case "$path" in
    "/"|"/*"|"/bin"|"/bin/"*|"/etc"|"/etc/"*|"/usr"|"/usr/"*|"/home"|"~")
    echo "✴ 拒绝删除系统目录:$path"
    exit 1
    ;;
esac

if [ -d "$path" ]; then
    echo "✴ 删除旧的构建目录:$path"
    rm -rf "$path"
fi

echo "✴ 配置CMake项目..."
cmake -B "$(pwd)/build"
echo "✴ 编译项目..."
cmake --build "$(pwd)/build" -j16
echo "✴ 构建完成"
echo "✴ 安装..."
sudo cmake --install build
echo "✴ 安装完毕"

sudo ldconfig