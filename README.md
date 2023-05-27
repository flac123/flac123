# flac123

flac123 is a command-line program for playing audio files encoded with [FLAC](https://xiph.org/flac/).  It uses [libao](https://xiph.org/ao/) and has been tested on Linux, FreeBSD, and macOS.

## Build

You will need the following three libraries installed.  The naming convention depends on the OS:

* macOS (brew) and FreeBSD: `flac` `libao` `popt`
* Debian variants: `libflac-dev` `libao-dev` `libpopt-dev`
* Red Hat variants: `flac-devel` `libao-devel` `popt-devel`

To build:

```
./configure
make
make install
```

## Usage

```
Usage: flac123 [OPTIONS] FILES...
  -d, --driver=STRING          set libao output driver (pulse, macosx, oss, etc).  Default is OS dependent
  -w, --wav=FILENAME           send output to wav file (use --wav=- and -q for stdout)
  -R, --remote                 set remote mode for programmatic control
  -b, --buffer-time=INT        override default hardware buffer size (in milliseconds)
  -q, --quiet                  suppress text output
  -v, --version                version info

Help options:
  -?, --help                   Show this help message
      --usage                  Display brief usage message
```
