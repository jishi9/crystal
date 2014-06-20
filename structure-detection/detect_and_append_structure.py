#!/usr/bin/python

from detect_cell_structure import CellStructureFromNodeStructure
from detect_edge_structure import EdgeStructureFromNodeStructure
from detect_node_structure import DetectNodeStructure
from magic_iterators import renumber_keys, renumber_value_lists, read_mesh_from_file
from write_structure_info import write_structured_node_info, write_structured_cell_info, write_structured_inedge_info, write_renumbered_border_edges, write_new_coord_data, write_text_map
from random import seed, randint
from shutil import copyfile
from sys import stderr
import argparse



def log(message):
    print >> stderr, '>>', message


def main(in_filename, out_filename, random_seed, start_node):
    # Copy file
    copyfile(in_filename, out_filename)

    # Read adjacency list
    log('Reading node2node map')
    node2node = read_mesh_from_file(out_filename, 'node_to_node', int, frozenset)


    # Set random seed, and choose a start node
    if random_seed:
        seed(random_seed)

    if start_node is None:
        start_node = randint(0, len(node2node) - 1)

    log('Out of %d nodes, we chose %d' % (len(node2node), start_node))


    # Detect node structure
    log('Detecting structure')
    node_structure = DetectNodeStructure(node2node, start_node)

    # Write out structured node data
    log('Writing out structured node to ' + out_filename)
    write_structured_node_info(node_structure, out_filename)


    log('Writing oldnode2newnode')
    write_text_map('oldnode_to_newnode', node_structure.oldnode2newnode, out_filename)

    # Renumber nodes
    node_structure.structured_node_regions = [ renumber_value_lists(region, node_structure.oldnode2newnode)
    for region in node_structure.structured_node_regions ]


    # Reorder coordinates
    log('Reordering coordinates')
    coord_data = read_mesh_from_file(out_filename, 'coord_data', float, list)
    new_coord_data = renumber_keys(coord_data, node_structure.newnode2oldnode)
    write_new_coord_data(new_coord_data, out_filename)


    # Figure out cell2nodes stuff
    log('Figuring out cell2nodes')
    cell2ordnodes = read_mesh_from_file(out_filename, 'cell_to_ord_nodes', int, list)

    # Apply node renumbering to cell map
    cell2ordnodes = renumber_value_lists(cell2ordnodes, node_structure.oldnode2newnode)

    cell_structure = CellStructureFromNodeStructure(node_structure.structured_node_regions, cell2ordnodes)
    log('Writing out structured cells to ' + out_filename)
    write_structured_cell_info(cell_structure, out_filename)


    # Figure out inedge2nodes
    log('Figuring out inedge2nodes and inedge2cells')
    inedge2nodes = read_mesh_from_file(out_filename, 'inedge_to_nodes', int, list)
    inedge2cells = read_mesh_from_file(out_filename, 'inedge_to_cells', int, list)

    # Apply node and cell renumbering to inedge maps
    inedge2nodes = renumber_value_lists(inedge2nodes, node_structure.oldnode2newnode)
    inedge2cells = renumber_value_lists(inedge2cells, cell_structure.oldcell2newcell)

    # Renumber cells
    for cell_region in cell_structure.structured_cell_regions:
        cell_region.structured_cells = renumber_value_lists(cell_region.structured_cells, cell_structure.oldcell2newcell)

    edge_structure = EdgeStructureFromNodeStructure(node_structure.structured_node_regions, cell_structure.structured_cell_regions, inedge2nodes, inedge2cells)

    log('Writing out structured inedges to ' + out_filename)
    write_structured_inedge_info(edge_structure, out_filename)



    # Read border edges
    borderedge2nodes = read_mesh_from_file(out_filename, 'borderedge_to_nodes', int, list)
    borderedge2cell = read_mesh_from_file(out_filename, 'borderedge_to_cell', int, list)

    # Apply node and cell renumbering to borderedge maps
    new_borderedge2nodes = renumber_value_lists(borderedge2nodes, node_structure.oldnode2newnode)
    new_borderedge2cell = renumber_value_lists(borderedge2cell, cell_structure.oldcell2newcell)

    log('Writing renumbered borderedges to ' + out_filename)
    write_renumbered_border_edges(new_borderedge2nodes, new_borderedge2cell, out_filename)

    log('Writing oldcell2newcell')
    write_text_map('oldcell_to_newcell', cell_structure.oldcell2newcell, out_filename)

    log('Writing oldedge2newedge')
    write_text_map('newedge_to_oldedge', edge_structure.newedge2oldedge, out_filename)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Detect structure in given *.p.part file, and append to form a *.p file')

    parser.add_argument('infile', metavar='INFILE', help='The input *.p.part file')
    parser.add_argument('outfile', metavar='OUTFILE', help='The output *.p file')
    parser.add_argument('--random_seed', metavar='SEED', type=int, nargs='?',
                   help='The random seed to use')
    parser.add_argument('--start_node', metavar='STARTNODE', type=int, nargs='?',
                   help='The node to start detecting structure from')

    args = parser.parse_args()
    main(args.infile, args.outfile, args.random_seed, args.start_node)
