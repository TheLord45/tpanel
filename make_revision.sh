#!/bin/bash
set -e pipefail

if [ ! -d ".git" ]
then
    echo "No git repository found!"
    exit 1
fi

SHA=$(git rev-parse $(git branch --show-current) | cut -c1-8)
echo "ID: $SHA"

cat << EOF > src/build.h
#ifndef __BUILD_H__
#define __BUILD_H__

#define BUILD_ID   "$SHA"

#endif
EOF

