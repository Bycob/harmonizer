#!/bin/bash

set -ex

pasuspender -- jackd -Rd alsa -d hw:$1 -r 48000 -p 1024 -n 2
