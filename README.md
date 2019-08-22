# scoretext
# How to build
First install the kcgi and libopc library and devel packages you can find kcgi and libopc on github https://github.com/kristapsdz/kcgi

https://github.com/freuter/libopc

After building and installing kcgi compile the project with make

# Installation 
Install like any other FastCGI script.

# Note
scoreurl.fcgi does not read urls passed to it, use the included web scraper.
The included web scraper uses anyread-api.js by [maludecks](https://github.com/maludecks). I've forked this repo you can find it [here](https://github.com/clockley/anyread-api.js)