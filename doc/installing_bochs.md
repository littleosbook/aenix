% How to install bochs
% Erik Helin, Adam Renberg
% 25 January, 2012

How to install bochs
====================

1. Grab the soruce for bochs 2.5.1 from [sourceforge][sf]
2. Extract the source with `tar xvzf bochs-2.5.1.tar.gz`
3. Install the package `xorg-dev` with
    sudo apt-get install xorg-dev
4. Configure with `./configure --enable-gdb-stub` to enable GDB support
5. Compile with `make`

[sf]: http://sourceforge.net/projects/bochs/files/bochs/2.5.1/ 
