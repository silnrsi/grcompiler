#!/bin/sh

find -type f -iregex '.*\.\([ch]pp\|[ch]\)' -exec sed -rf errors.sed {} \; \
| sort -u -k1,1d -k2,2n
