#!/bin/bash

set -e


while read file; do
    sudo rm -rf "$file"
    echo "-- Removing: $file" 
done < build/install_manifest.txt

sudo rmdir "/usr/local/include/muduo"
sudo rmdir "/usr/local/lib/cmake/muduo"
echo "rmdir /usr/local/include/muduo"
echo "rmdir /usr/local/lib/cmake/muduo"
