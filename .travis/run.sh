#!/bin/bash

set -e
set -x

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
    # Run with native
    .travis/run_project_build.sh
else
    # Run with docker
    docker run -v$(pwd):/home/conan $DOCKER_IMAGE bash -c "pip install conan --upgrade && sudo apt-get update && sudo apt-get -y install mesa-common-dev && .travis/run_project_build.sh"
fi