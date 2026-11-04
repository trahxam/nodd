#!/bin/bash
# One condor job: generate ttbar+PU HepMC3, then simulate with ddsim (ODD).
# Usage: run_job.sh <jobIndex> <eventsPerJob> <nPileup> <outDir> <oddInstall> <genBinary>
set -euo pipefail
job=$1; nev=$2; npu=$3; out=$4; odd=$5; gen=$6
seed=$((1000 + job))

set +eu
source /cvmfs/sw.hsf.org/key4hep/setup.sh -r 2026-04-08 >/dev/null 2>&1
source "$odd/bin/this_odd.sh"
export PYTHIA8DATA=$(pythia8-config --xmldoc)
set -eu

work=$(mktemp -d)
cd "$work"
"$gen" "$nev" "$seed" "$npu" events.hepmc3
ddsim --compactFile "$odd/share/OpenDataDetector/xml/OpenDataDetector.xml" \
      --inputFiles events.hepmc3 -N "$nev" \
      --random.seed "$seed" \
      --outputFile sim.edm4hep.root

mkdir -p "$out"
cp sim.edm4hep.root "$out/ttbar_pu${npu}_job$(printf %04d "$job").edm4hep.root"
rm -rf "$work"
