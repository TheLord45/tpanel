#!/bin/bash
# errors are propagated when using the pipe to concatenate commands 
set -o pipefail
# determine script directory
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
ROOT_DIR="${SCRIPT_DIR}/.."
LOG_FILE="${ROOT_DIR}/workflow.log"
export QT_DIR="/opt/Qt/6.6.0/gcc_64"
export CMAKE_PREFIX_PATH="${QT_DIR}/lib/cmake"

# the first parameter is the branch, that is currently being built
BRANCH="$1"

# Log the message.
# parameters:
#    message ... the message to log
log() {
  local message="$1"
  local currentTime=$(date +"%y-%m-%d %H:%M:%S")
  echo "${currentTime}: ${message}" | tee -a ${LOG_FILE}
}

CPUS=2
type nproc > /dev/null 2>&1

if [ $? -eq 0 ]
then
    CPUS=`nproc`
fi

rm -rf "${ROOT_DIR}/build" > /dev/null 2>&1
mkdir "${ROOT_DIR}/build"

log "Changing to directory $ROOT_DIR"
cd ${ROOT_DIR}
log "Creating the Makefile"
cmake -B build -S . >> ${LOG_FILE} 2>&1

if [ $? -ne 0 ]
then
    log "ERROR: Could not create Makefile!"
    exit 1
fi

log "Changing to build directory"
cd build
log "Building the application ..."
exec make -j${CPUS} >> ${LOG_FILE} 2>&1
