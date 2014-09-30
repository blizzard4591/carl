Building with CMake {#build_cmake}
===========

We use [CMake](www.cmake.org) to support the building process. [CMake](www.cmake.org) is a command line tool available for all major platforms. 
To simplify the building process on Unix, we suggest using the GUI for [CMake](www.cmake.org), called [CCMake](http://www.vtk.org/Wiki/CCMake_2.8.11_Docs). 

[CMake](www.cmake.org) generates a Makefile likewise to Autotools' configure.
We suggest initiating this procedure from a separate build directory, called 'out-of-source' building. 
This keeps the source directory free from files created during the building process.

CMake Options for building CArL.
------------

Run `ccmake` to obtain a list of all available options or change them.
@code
$ cd build/
$ ccmake ../
@endcode

Using `[t]`, you can enable the _advanced mode_ that shows all options. Most of these should not be changed by the average user.

### General 

- *BUILD_STATIC* [ON, OFF] <br>
  If set to *ON*, carl is built as shared and static library, otherwise only as shared library.

- *CMAKE_BUILD_TYPE* [Release, Debug]
  - *Release*
  - *Debug*
  
- *CMAKE_CXX_COMPILER* \<compiler command\>
  - `/usr/bin/c++`: Default for most linux distributions, will probably be an alias for `g++`.
  - `/usr/bin/g++`: Uses `g++`.
  - `/usr/bin/clang++`: Uses `clang`.

- *USE_COTIRE* [ON, OFF] <br>
  If set to *ON*, `cotire` is used to produce precompiled headers. This can reduce the compile time significantly.


### Debugging

- *DEVELOPER* <br>
  Enables additional compiler warnings.

- *LOGGING* [ON, OFF] <br>
  Setting *LOGGING* to *OFF* disables all logging output. 
  It is recommended if the performance should be maximized, but notice that this also prevents important warnings and error messages to be generated.

- *LOGGING_CARL* [ON, OFF] <br>
  Switches between the CArL logging library (*ON*) and simply logging to `std::cout` and `std::cerr` (*OFF*).


CMake Targets
------------

There are a few important targets in the CArL CMakeLists:

- `doc`: Builds the doxygen documentation.
- `lib_carl`: Builds the shared library.
- `lib_carl_static`: Builds the static library.
- `runXTests`: Builds the tests for the `X` module.
- `test`: Build and run all tests.