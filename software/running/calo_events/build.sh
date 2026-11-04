#!/bin/bash
# Build the generator against Key4hep 2026-04-08. Usage: build.sh <outBinary>
set -euo pipefail
set +eu
source /cvmfs/sw.hsf.org/key4hep/setup.sh -r 2026-04-08 >/dev/null 2>&1
set -eu
here=$(cd "$(dirname "$0")" && pwd)
mkdir -p "$(dirname "$1")"
libs="$(pythia8-config --libs) $(HepMC3-config --libs)"
rpath=$(for d in $(echo "$libs" | tr ' ' '\n' | sed -n 's/^-L//p'); do printf -- '-Wl,-rpath,%s ' "$d"; done)
g++ -O2 -std=c++17 "$here/gen_ttbar_pu.cc" -o "$1" \
    $(pythia8-config --cxxflags) $(HepMC3-config --cflags) $libs $rpath
