# scoretext
# How to build
First install the kcgi library and devel packages you can find kcgi on github https://github.com/kristapsdz/kcgi
or in your distro's repo https://repology.org/metapackage/kcgi/versions

After building and installing kcgi compile scoretext, be sure to link with the zlib, libc math and kcgi:
gcc scoretext.c -lkcgi -lz -lm -o scoretext.fcgi

# Installation 
Install like any other FastCGI script.
