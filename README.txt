TvRQ:  Test vector RaptorQ
==========================

This is the RaptorQ version that's based on simple Gaussian Elimination;
used to generate test data. TvRQ is a test vector implementation of RaptorQ 
that is meant to test for compliance (correct encoding and decoding) and is 
not implemented to be fast, and in fact it is a quite slow implementation 
of RaptorQ. 

For a compliant and faster implementation please refer to BitRipple Rq
available here: https://www.bitripple.com

Configuration & Building
------------------------

TvRQ uses the "cmake" build system generator; so apart from the usual
suspects needed to build, such as a C compiler, make (on UNIX), etc.,
you need cmake.  If the python module _tvrq is to be built, a python
installation including the development libraries as well as the cffi
module is required.

The code might use some C99 features, so I'm not sure it will compile
out of the box with Visual C.

To prepare building in a build/ subdirectory, do the following from
within the TvRQ root:

	$ mkdir build
	$ cd build
	$ cmake ..

This verifies the system configuration and then populates the build/
subfolder with necessary build files, such as a Makefile, etc.  Building
can then be done by issuing:

	$ cmake --build .

To install the TvRQ libraries to be discovered easily by other software, do:

	$ sudo cmake --build . --target install

If all works out smoothly, this builds the tvrq dynamic library
(libtvrq.so on Linux), as well as a number of test tools called test_*
or testi_*

Optional Components
-------------------

On Linux, the _tvrq Python module is available, it's a simple CFFI-based
module that provides access to the TvRQ functionality from Python.  For
it to be built, the BUILD_PYTHON_MODULE option must be enabled, and the
necessary python development libraries, as well as the Python cffi
module must be installed.  The option is enabled by adding -D
BUILD_PYTHON_MODULE=ON to the cmake configuration command line, or by
editing build/CMakeCache.txt accordingly.

Running the testing tools
-------------------------

There are two kinds of testing tools, unit tests and interactive tests.  
The interactive tools print out some output (e.g. a matrix) on screen in 
response to command line arguments.  The user can thus check if the output 
is as expected.  They do not however have a way to sanity-check that 
output themselves.  The unit test tools on the other hand perform some 
self-consistency checks, and return a nonzero exit status in the case of 
failures. 

Short description of some of the tools:
	Unit tests:
	gf256	- sanity checks the GF(256) finite field operations
	m256v	- sanity checks matrix operations
	rq_encdec_match	- sanity check the RQ code via the API (help: -h)
	
	Interactive tests:
	lt	- display lt rows. Args: <K> <ISI0> <ISI1> ...
	ldpc	- display ldpc rows. Args: <K>
	hdpc	- display hdpc rows. Args: <K> <fast-flag>
	parameters - display parameter computation. Args: <K>
	rq_matrix - display the entire RQ matrix. Args <K> <ISI0> ...

Running the test suite
----------------------

The testsuite is based on ctest.  From the build directory, run

	$ ctest

Takes a few seconds to complete.  Currently, only the unit tests are added 
to the test suite.  
