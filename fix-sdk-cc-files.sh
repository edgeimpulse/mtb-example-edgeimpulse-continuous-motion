#!/bin/bash

cd ei-model/edge-impulse-sdk && find ./ -depth -name "*.cc" -exec sh -c 'mv "$1" "${1%.cc}.cpp"' _ {} \;
