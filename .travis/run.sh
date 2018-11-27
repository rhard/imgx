#!/bin/bash

set -e
set -x

if [[ "$(uname -s)" == 'Darwin' ]]; then
    if which pyenv > /dev/null; then
        eval "$(pyenv init -)"
    fi
    pyenv activate conan
fi

conan remote add rhard "https://api.bintray.com/conan/rhard/conan"
conan install .
cmake .
cmake --build . --target imgx_test
