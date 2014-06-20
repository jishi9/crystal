#!/usr/bin/python

import argparse
from itertools import islice

def parseInts(string):
    return map(int, string.split())


def read_explicit_perm_file(filename, num_elems):
  old2new = [None] * num_elems
  with open(filename, 'r') as f:
    for line in f:
      old, new = map(int, line.split())
      old2new[old] = new
  return old2new


def read_implicit_perm_file(filename, num_elems):
  old2new = [None] * num_elems
  with open(filename, 'r') as perm:
    for new, old in enumerate(perm):
      old2new[int(old)] = new
  return old2new


def read_and_reorder(iter_data, num_elems, old2new):
  reordered_data = [None] * num_elems
  for old_order, data in enumerate(iter_data):
    new_order = old2new[old_order]
    reordered_data[new_order] = data
  return reordered_data


def write_lines(lines, out_handle):
  for l in lines:
    assert l[-1] == '\n'
    out_handle.write(l)


def reorder_values(iter_data, old2new, out_handle, limit=None):
  for data in iter_data:
    numerics = parseInts(data)
    partition = len(numerics) if limit is None else limit

    converted_data, static_data = numerics[:partition], map(str, numerics[partition:])
    new_converted_data = [ str(old2new[n]) for n in converted_data ]
    out_handle.write(' '.join(new_converted_data + static_data) + '\n')



def main(permfile, infile, outfile):
    with open(infile, 'r') as in_dat:
        # Read an parse header
        header = next(in_dat)
        num_nodes, num_cells, num_internal_edges, num_border_edges = parseInts(header)

        # Build map: old node -> new node
        old2new = read_implicit_perm_file(permfile, num_nodes)

        # Read coordinates and reorder: node -> coordinates
        reordered_coords = read_and_reorder(islice(in_dat, num_nodes), num_nodes, old2new)

        with open(outfile, 'w') as out_dat:
            # Write out header
            assert header[-1] == '\n'
            out_dat.write(header)

            # Write out reordered coordinates
            write_lines(reordered_coords, out_dat)

            # Reorder nodes in Cell -> Nodes
            reorder_values(islice(in_dat, num_cells), old2new, out_dat)

            # Reorder nodes in Edge -> Nodes
            reorder_values(islice(in_dat, num_internal_edges + num_border_edges), old2new, out_dat, limit=2)

        # Check EOF
        assert in_dat.read(1) == ''


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Apply a node renumbering (from an *.iperm file) to the given *.dat file')

    parser.add_argument('permfile', metavar='PERMFILE', help='The input *.iperm file')
    parser.add_argument('infile', metavar='INFILE', help='The input *.dat file')
    parser.add_argument('outfile', metavar='OUTFILE', help='The output *.dat file')

    args = parser.parse_args()
    main(permfile=args.permfile, infile=args.infile, outfile=args.outfile)
