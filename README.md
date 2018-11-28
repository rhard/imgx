# imgx

Another ImGui port for X-Plane

[![Build status](https://ci.appveyor.com/api/projects/status/5j47wbf9tibufse8?svg=true)](https://ci.appveyor.com/project/rhard/imgx)
[![Build Status](https://travis-ci.org/rhard/imgx.svg?branch=master)](https://travis-ci.org/rhard/imgx)

## How to build example plugin from this repository on Ubuntu 16.04 LTS. Same recepy could be used on Windows and Mac OS.

You need to have valid Python 2 or 3, Cmake, GCC or Clang compiler. You also need an OpenGL dev package (ie. *mesa-common-dev*).
Next you will need to install **conan** package manager and I used this command:

```pip install conan```

Add my conan channel for X-Plane SDK:

```conan remote add rhard "https://api.bintray.com/conan/rhard/conan"```

Create folder to clone repositories into. I called mine *xp11_imgx_plugin_builder* so use the following command from home:

```mkdir xp11_imgx_plugin_builder```

Change your directory using this command:

```cd xp11_imgx_plugin_builder```

Get the imgx repository:

```git clone --recursive https://github.com/rhard/imgx.git```

Change your directory:

```cd imgx```

Create build directory:

```mkdir build```

Change your directory:

```cd build```

Use the following command to install the conan build files:

```conan install ..```

Then the following comand:

```cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release```

And the next command:

```cmake --build . --target imgx_test```

You then should find imgx_test.xpl in the ~/xp11_imgx_plugin_builder/imgx/build/lib folder

## How to use this library in the final project

*TODO*




