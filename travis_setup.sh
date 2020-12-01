#!/bin/bash

mkdir cbuild

if [ "$TRAVIS_OS_NAME" == "osx" ]; then
  wget -q https://products.solace.com/download/C_API_OSX 
  tar xvf C_API_OSX -C ./cbuild --strip-components=1
elif [ "$TRAVIS_OS_NAME" == "linux" ]; then
  wget -q https://products.solace.com/download/C_API_LINUX64
  tar xvf C_API_LINUX64 -C ./cbuild --strip-components=1
elif [ "$TRAVIS_OS_NAME" == "windows" ]; then
  wget -q https://products.solace.com/download/C_API_VS2015
  tar xvf C_API_VS2015 -C ./cbuild --strip-components=1
else
  echo "$TRAVIS_OS_NAME is currently not supported"  
fi
