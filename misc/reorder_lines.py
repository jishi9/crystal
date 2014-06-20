#!/usr/bin/python

import argparse
from reorder_dat import parseInts, read_and_reorder, write_lines, read_explicit_perm_file

def main(permfile, infile, outfile):
    with open(infile, 'r') as in_dat:
        # Read an parse header
        header = next(in_dat)
        num_lines, num_fields = parseInts(header)

        # Build map: old index -> new index
        old2new = read_explicit_perm_file(permfile, num_lines)

        # Read coordinates and reorder: node -> coordinates
        reordered_coords = read_and_reorder(in_dat, num_lines, old2new)

        with open(outfile, 'w') as out_dat:
            # Write out header
            assert header[-1] == '\n'
            out_dat.write(header)

            # Write out reordered data
            write_lines(reordered_coords, out_dat)

        # Check EOF
        assert in_dat.read(1) == ''

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Apply an element renumbering (from an *.iperm file) to the given *output* dat file')

    parser.add_argument('permfile', metavar='PERMFILE', help='The input *.iperm file')
    parser.add_argument('infile', metavar='INFILE', help='The input *.dat file')
    parser.add_argument('outfile', metavar='OUTFILE', help='The output *.dat file')

    args = parser.parse_args()
    main(permfile=args.permfile, infile=args.infile, outfile=args.outfile)
