#!/usr/bin/python

from subprocess import Popen, PIPE
from collections import defaultdict
from magic_iterators import read_mesh_from_file

def enum(**enums):
    return type('Enum', (), enums)

FILENAME = 'meshes/mini-airfoil.p'
NODEFONTSIZE = 7
EDGEFONTSIZE = 7
CELLFONTSIZE = 7
COLOR_BORDER_EDGES = False
COLOR_STRUCTURED_NODES = False
COLOR_STRUCTURED_EDGES = False
COLOR_STRUCTURED_CELLS = False

LabellingDegree = enum(No=0, Old=1, New=2, Both=3)
LABEL_EDGES = LabellingDegree.No
LABEL_CELLS = LabellingDegree.Old
LABEL_NODES = LabellingDegree.Old

new_structured_nodes = set(xrange(55))
new_structured_cells = set(xrange(40))
new_structured_h_edges = set(xrange(27))
new_structured_v_edges = set(xrange(27, 27+27))


def read_text_from_file(filename, field, data_type, collection):
    output = []
    p = Popen(['unzip', '-p', filename, field], stdout=PIPE, bufsize=1)
    for line in iter(p.stdout.readline, b''):
        elements_in_line = collection(map(data_type, line.split()))
        output.append(elements_in_line)

    assert p.wait() == 0, 'Unzip failed'

    return output


def read_one_to_one(section_name):
    a2b = dict()
    for a, b in read_text_from_file(FILENAME, section_name, int, list):
        a2b[a] = b
    return a2b

def invert_map(a2b):
    b2a = dict()
    for a, b in a2b.iteritems():
        b2a[b] = a
    return b2a


def read_inverted_enumerated_map(filename, field, offset):
    b2as = defaultdict(set)
    a_count = 0
    for a, bs in enumerate(read_mesh_from_file(FILENAME, field, int, list), offset):
        a_count += 1
        for b in bs:
            b2as[b].add(a)

    return b2as, a_count



class GraphPrinter(object):
    def get_elem_label(self, flag, old, new):
        if flag == LabellingDegree.No:
            return ''
        elif flag == LabellingDegree.Old:
            return str(old)
        elif flag == LabellingDegree.New:
            return str(new)
        elif flag == LabellingDegree.Both:
            return '%d=%d' % (old, new)
        elif hasattr(flag, '__call__'):
            return flag(old, new)
        else:
            raise Exception('Unhandled case')


    def get_elem_label_attribute(self, flag, old, new):
        if flag == LabellingDegree.No:
            return []
        else:
            value = self.get_elem_label(flag, old, new)
            return ['label = "%s"' % value]


    # Get node name
    def get_node_name(self, old_node):
        assert LABEL_NODES != LabellingDegree.No, "Can't have nameless nodes!"
        new_node = self.oldnode2newnode[old_node]
        return self.get_elem_label(LABEL_NODES, old_node, new_node)


    def get_edge_attribs(self, n1, n2):
        inedges = self.node2inedges[n1] & self.node2inedges[n2]
        borderedges = self.node2borderedges[n1] & self.node2borderedges[n2]
        assert len(inedges) + len(borderedges) == 1, 'What...'

        attributes = ['fontsize=%d'%EDGEFONTSIZE]
        if inedges:
            old_edge = next(iter(inedges))

            # Color structured edges
            if COLOR_STRUCTURED_EDGES:
                new_edge = self.oldedge2newedge[old_edge]
                if new_edge in new_structured_h_edges: attributes += ['color=green', 'fontcolor=green']
                elif new_edge in new_structured_v_edges: attributes += ['color=orange', 'fontcolor=orange']

        else:
            # Border edges
            old_edge = next(iter(borderedges))
            if COLOR_BORDER_EDGES:
                attributes += ['color=blue']

        new_edge = self.oldedge2newedge[old_edge]
        attributes += self.get_elem_label_attribute(LABEL_EDGES, old_edge, new_edge)

        return attributes


    def edge_a_to_b(self, a, b, attribs):
        return '"%s" -- "%s" [%s]' % (self.get_node_name(a), self.get_node_name(b), ','.join(attribs))


def draw(label_cells=None):
    if label_cells: LABEL_CELLS = label_cells

    print 'graph {'

    printer = GraphPrinter()

    # Read edge2node
    printer.node2inedges, num_inedges = read_inverted_enumerated_map(FILENAME, 'inedge_to_nodes', 0)
    printer.node2borderedges, num_borderedges = read_inverted_enumerated_map(FILENAME, 'borderedge_to_nodes', num_inedges)

    # Read maps
    printer.oldnode2newnode = read_one_to_one('oldnode_to_newnode')
    printer.oldcell2newcell = read_one_to_one('oldcell_to_newcell')
    printer.oldedge2newedge = invert_map(read_one_to_one('newedge_to_oldedge'))
    # Append border edges
    for edge_id in xrange(num_inedges, num_inedges + num_borderedges):
        assert edge_id not in printer.oldedge2newedge
        printer.oldedge2newedge[edge_id] = edge_id

    # Build inverse maps
    newnode2oldnode = invert_map(printer.oldnode2newnode)
    newcell2oldcell = invert_map(printer.oldcell2newcell)


    # Color structured nodes
    for old_node, new_node in printer.oldnode2newnode.iteritems():
        attributes = ['fontsize=%d' % NODEFONTSIZE]
        if COLOR_STRUCTURED_NODES and new_node in new_structured_nodes:
            attributes += ['fillcolor=red', 'style=filled']
        print '"%s" [%s];' % (printer.get_node_name(old_node), ','.join(attributes))


    # Draw edges
    for source, targets in enumerate(read_mesh_from_file(FILENAME, 'node_to_node', int, list)):
        for t in targets:
            # Only one instance of each edge
            if source >= t:
                attribs = printer.get_edge_attribs(source, t)
                print printer.edge_a_to_b(source, t, attribs)


    # Draw cells
    for new_cell, new_nodes in enumerate(read_mesh_from_file(FILENAME, 'new_cell_to_ord_nodes', int, list)):
        n1, n2, n3, n4 = [ newnode2oldnode[new] for new in new_nodes ]
        old_cell = newcell2oldcell[new_cell]

        attribs = ['color=white', 'fontsize=%d' % CELLFONTSIZE]
        attribs += printer.get_elem_label_attribute(LABEL_CELLS, old_cell, new_cell)

        if COLOR_STRUCTURED_CELLS and new_cell in new_structured_cells:
            attribs += ['fontcolor=brown']
        else:
            attribs += ['fontcolor=blue']

        print printer.edge_a_to_b(n1, n3, attribs)

    print '}'



if __name__ == '__main__':
    draw()
