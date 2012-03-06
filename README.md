# aenix

aenix (Adam and Eriks NIX) is a small hobby operating system.

## Dependencies
- [GCC](http://gcc.gnu.org/)
- [NASM](http://www.nasm.us/)
- [genisoimage](http://cdrkit.org/)
- [bochs](http://bochs.sourceforge.net/)
- [Make](http://www.gnu.org/software/make/)

If you are running Ubuntu 10.10 or later, these can all be installed by running

    sudo apt-get install build-essential nasm genisoimage bochs bochs-x

## Building

    cd src
    make

## Running

    cd src
    make run

## Documentation
The book [The little book about OS development](http://littleosbook.github.com) is
based on our experience writing aenix. Everything described in the book is used
in aenix.

## License
[GPLv3](http://www.gnu.org/licenses/gpl.html)
