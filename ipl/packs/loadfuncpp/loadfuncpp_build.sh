#!/bin/bash

set -o verbose #echo on

#loadfuncpp itself
make clean
make

pushd cgi
make
popd

#pushd icondb
#make
#popd

pushd socket
make clean
make
popd

pushd system
make clean
make
popd

pushd openssl
make clean
make
popd

set +o verbose #echo off
