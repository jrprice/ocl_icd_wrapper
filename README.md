===============
ocl_icd_wrapper
===============

About
-----

This project provides an OpenCL ICD wrapper for implementations that
don't support the ICD extension. For example, Apple's OpenCL
implementation currently doesn't support the ICD, which means an
OpenCL application running on OS X has to be rebuilt when switching
between Apple's OpenCL implementation and any third-party OpenCL
implementations on the sytem. This project exposes such
implementations to applications using the OpenCL ICD.


Usage
-----

These instructions assume that you have a working OpenCL ICD on your
system. The OpenCL ICD can be downloaded from the Khronos registry:
http://www.khronos.org/registry/cl/

After configuring the build system, you should run 'make', and
indicate the library you wish to wrap by specifying arguments to
LDFLAGS. For example:

make LDFLAGS+="-framework OpenCL"

Copy the resulting library to your desired location and then register
the library with the ICD. For example:

echo "libocl_icd_wrapper.dylib" >/etc/OpenCL/vendor/apple_wrapper.icd

You should ensure that this library is on the {DY}LD_LIBRARY_PATH, or
alternatively use the full path to the library. You may wish to rename
the library if you are planning to wrap more than one implementation.
