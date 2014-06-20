#!/bin/bash -ue

OUTDIR=/tmp/mesh/

STARTNODE=$1
RANDSEED=$2

CANON="Sum = 1340.055675"

MESH="$(mktemp)"
trap "rm $MESH" EXIT

# Detect structure, exit if failed
./detect_and_append_structure.py --random_seed $RANDSEED --start_node $STARTNODE meshes/mini-airfoil.p.part "$MESH" 1>/dev/null 2>/dev/null ||
  { echo "Structure fail: $RANDSEED $STARTNODE" ; exit 13 ; }

# Run computation
RESULT=$(bin/run-edge-computation meshes/mini-airfoil.p  --structured 2>/dev/null)
SUCCESS=$?

# Check that computation did not crash
if [[ $SUCCESS -ne 0 ]] ; then
  echo "Crash: $RANDSEED $STARTNODE"
  exit 99
fi

# Compare with canon
if [[ "$RESULT" != "$CANON" ]] ; then
  echo "Mismatch: $CANON vs $RESULT"
  exit 7
fi

