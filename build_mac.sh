#!/bin/bash
cmake -B build -DCMAKE_PREFIX_PATH="/opt/Qt/6.6.2/macos/lib/cmake:/opt/homebrew/lib/cmake"

if [ $? -ne 0 ]
then
    exit 1
fi

cd build
exec make -j8
