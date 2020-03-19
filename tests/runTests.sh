#!/bin/bash

export DELTASOLACE_HOME=$CI_BUILDS_DIR;
export LD_LIBRARY_PATH=$DELTASOLACE_HOME/clib:$LD_LIBRARY_PATH;

rlwrap q tests/connect.q "$@"
