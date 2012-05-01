#!/bin/bash
V=`grep FLAC123_VERSION flac123/version.h | sed -e 's/[^"]*["]//' | sed -e 's/["].*//'`
echo $V
