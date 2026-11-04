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
      --outputFile sim.edm4hep.root > ddsim.log 2>&1 \
  || echo "ddsim exit status $? (known teardown crash; output checked below)" >&2
tail -5 ddsim.log

# ddsim's exit status is unreliable both ways: 0 after aborted input, nonzero
# from a teardown segfault after a complete run. Check the output instead.
if grep -qE "Must Be Aborted|event parsing failed" ddsim.log; then
  echo "ddsim run aborted" >&2; exit 1
fi
done=$(python3 -c "
import ROOT
f = ROOT.TFile.Open('sim.edm4hep.root')
print(-1 if not f or f.IsZombie() or f.TestBit(ROOT.TFile.kRecovered) else f.Get('events').GetEntries())")
if [ "$done" != "$nev" ]; then
  echo "simulated $done of $nev events" >&2; exit 1
fi

mkdir -p "$out"
cp sim.edm4hep.root "$out/ttbar_pu${npu}_job$(printf %04d "$job").edm4hep.root"
rm -rf "$work"
