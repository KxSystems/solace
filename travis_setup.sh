#!/bin/bash

mkdir cbuild

if [ "$TRAVIS_OS_NAME" == "osx" ]; then
  wget -q https://products.solace.com/download/C_API_OSX 
  tar xvf C_API_OSX -C ./cbuild --strip-components=1
elif [ "$TRAVIS_OS_NAME" == "linux" ]; then
  wget -q https://products.solace.com/download/C_API_LINUX64
  tar xvf C_API_LINUX64 -C ./cbuild --strip-components=1
elif [ "$TRAVIS_OS_NAME" == "windows" ]; then
  if [ "$ARCH" == "x64" ]; then
    wget -q https://products.solace.com/download/C_API_WIN64
    tar xvf C_API_WIN64 -C ./cbuild --strip-components=1
  else
    wget -q https://products.solace.com/download/C_API_WIN32
    tar xvf C_API_WIN32 -C ./cbuild --strip-components=1
  fi
else
  echo "$TRAVIS_OS_NAME is currently not supported"  
fi
