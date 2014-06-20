#!/bin/bash -eu

MESHFILE="$1"
PRN="bin/mesh-printer"

"$PRN" header "$MESHFILE"
"$PRN" new_coord_data "$MESHFILE"
"$PRN" new_cell_to_ord_nodes "$MESHFILE"

paste -d' ' <("$PRN" new_inedge_to_nodes "$MESHFILE") <("$PRN" new_inedge_to_cells "$MESHFILE")

paste -d' ' <("$PRN" new_borderedge_to_nodes "$MESHFILE") <("$PRN" new_borderedge_to_cell "$MESHFILE") <("$PRN" borderedge_bounds "$MESHFILE")