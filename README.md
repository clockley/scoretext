# scoretext
# How to build
First install the kcgi library and devel packages you can find kcgi on github https://github.com/kristapsdz/kcgi
or in your distro's repo https://repology.org/metapackage/kcgi/versions

After building and installing kcgi compile scoretext using the make command.

# Installation 
Install like any other FastCGI script.

# Note
scoreurl.fcgi does not read urls passed to it, you must supply a web scraper that reads from stdin and writes text to stdout followed by the string "\n\0EOF\n"
