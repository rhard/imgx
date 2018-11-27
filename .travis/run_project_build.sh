#!/bin/bash

set -e
set -x

conan remote add rhard "https://api.bintray.com/conan/rhard/conan"
conan install . 
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release .
cmake --build . --target imgx_test 