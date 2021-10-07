##libcsv

This is a simple, fast library for parsing delimited data.  Utilizing fail-safe mode (slower),
libcsv can even parse malformed data that does not adhere to [RFC 4180](https://www.ietf.org/rfc/rfc4180.txt). 
There are no dependencies for this project unless you want to run the test suite which requires
[libcheck](https://github.com/libcheck/check).

The only real goal for this project is to provide a fast and robust library to do a lot of
the tedious IO work for [fql](https://github.com/jasonKercher/fql).

NOTE: As it stands this library is not cross-platform. This library targets *Linux*.

```sh
./configure
make 
make check # optional
make install
```

###Installs
Program: stdcsv
Library: libcsv.so

