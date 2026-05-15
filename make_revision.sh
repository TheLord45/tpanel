#!/bin/bash
set -e pipefail

if [ ! -d ".git" ]
then
    echo "No git repository found!"
    exit 1
fi

SHA=$(git rev-parse $(git branch --show-current) | cut -c1-8)
DATUM=$(date '+%Y-%m-%d')
TIME=$(date '+%H:%M:%S')
echo "ID: $SHA"

cat << EOF > src/build.h
#ifndef __BUILD_H__
#define __BUILD_H__

#define BUILD_ID   "$SHA"
#define BUILD_DATE "$DATUM"
#define BUILD_TIME "$TIME"

#endif
EOF

