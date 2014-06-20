#!/usr/bin/python

from collections import deque, defaultdict
from random import randint, seed
from graph import draw
from magic_iterators import read_mesh_from_file
from quad_mesh import DetectQuadStructure, RandomFrozenSet, StructureException

class Quad(object):
    def __init__(self, nodes):
        super(Quad, self).__init__()
        self.nodes = nodes
        self.nodes_set = frozenset(nodes)
        self.neighbours = [None, None, None, None]

    def __eq__(self, other):
        return self.nodes_set == other.nodes_set

    def __hash__(self):
        return hash(self.nodes_set)

    def __iter__(self):
        return iter(self.nodes)


BIGNUM = float('Inf')
SMALLNUM = float('-Inf')


class Unitco(object):
    def __init__(self, quad, row, col, level):
        self.quad = quad
        self.row = row
        self.col = col
        self.level = level

    def get_cell_id(self):
        global nodes2cell
        return nodes2cell[frozenset(self.quad.nodes)]


    def __repr__(self):
        return 'Unitco(cell %d = %s, %d, %d, %d)' % (self.get_cell_id(), self.quad.nodes, self.row, self.col, self.level)


class QuadSearch(DetectQuadStructure):
    EAST, SOUTH, WEST, NORTH = range(4)

    d2rows = {EAST: 0, SOUTH: -1, NORTH: 1, WEST: 0}
    d2cols = {EAST: 1, SOUTH: 0, NORTH: 0, WEST: -1}

    @staticmethod
    def quad_rotations((r1c1, r1c2, r2c1, r2c2)):
        yield QuadSearch.EAST, (r1c1, r1c2, r2c1, r2c2)
        yield QuadSearch.SOUTH, (r1c2, r2c2, r1c1, r2c1)
        yield QuadSearch.WEST, (r2c2, r2c1, r1c2, r1c1)
        yield QuadSearch.NORTH, (r2c1, r1c1, r2c2, r1c2)


    def _add_node_to_row(self, node, lst, end):
        self.visited.add(node)

    def _remove_node_from_row(self, node, lst, end):
        self.visited.remove(node)

    def __init__(self, node_map, num_node_neighbours=4):
        self.node_map = [ RandomFrozenSet(ns) for ns in node_map ]
        self.current_row_index = 0  # DEBUG
        self.visited = set()
        self.current_row, self.next_row = self.output = [ [], [] ]
        self.num_node_neighbours = num_node_neighbours


    def visit(self, start_node, searcher_class):
        # Get first quad
        init_quad = Quad(self.find_quad_from_point(start_node))
        r1c1, r1c2, r2c1, r2c2 = init_quad.nodes
        assert {r1c2, r2c1} < self.neighbours(r1c1)
        assert {r1c1, r2c2} < self.neighbours(r1c2)
        assert {r1c1, r2c2} < self.neighbours(r2c1)
        assert {r1c2, r2c1} < self.neighbours(r2c2)

        # To figure out region area
        min_row, max_row = BIGNUM, SMALLNUM
        min_col, max_col = BIGNUM, SMALLNUM

        global quad2score
        quad2score = defaultdict(int)

        visited_quads = set()
        quads = searcher_class([Unitco(init_quad, 0, 0, 0)])

        while quads:
            q = quads.take()

            me_quad = Quad(q.quad)
            if me_quad in visited_quads:
                continue
            else:
                visited_quads.add(me_quad)
            yield q

            for direction, current_quad in QuadSearch.quad_rotations(q.quad):
                row = q.row + QuadSearch.d2rows[direction]
                col = q.col + QuadSearch.d2cols[direction]

                # Clear set of visited nodes such that it only includes the current quad
                self.visited = set(current_quad)
                try:
                    # Extend quad in direction
                    new_quad = self.extend_quad_row(*current_quad)
                except StructureException as e:
                    continue

                # Convert to quad objects
                current_quad = Quad(current_quad)
                new_quad = Quad(new_quad)

                # Dampen score
                quad2score.update({ c: score*0.999 for (c,score) in quad2score.iteritems() })
                quad2score[new_quad] += 1

                # Add new quad to direction
                assert current_quad.neighbours[direction] is None, 'Bruv already has a neighbour in that direction?'
                current_quad.neighbours[direction] = new_quad

                quad_meta = Unitco(new_quad, row, col, q.level+1)
                quads.put(quad_meta)

                min_row = min(min_row, row)
                max_row = max(max_row, row)
                min_col = min(min_col, col)
                max_col = max(max_col, col)


def invert_list(a2b):
    b2a = dict()
    for a, b in enumerate(a2b):
        assert b not in b2a
        b2a[b] = a
    return b2a


class BreadthFirstSearchCandidates(object):
    def __init__(self, iterable):
        self.q = deque(iterable)

    def put(self, obj):
        self.q.append(obj)

    def take(self):
        return self.q.popleft()

    def __nonzero__(self):
        return not not self.q


class DepthFirstSearchCandidates(object):
    def __init__(self, iterable):
        self.q = list(iterable)

    def put(self, obj):
        self.q.append(obj)

    def take(self):
        return self.q.pop()

    def __nonzero__(self):
        return not not self.q


class BestFirstSearchCandidates(object):
    global quad2score
    def __init__(self, iterable):
        self.q = set(iterable)

    def put(self, obj):
        self.q.add(obj)

    def take(self):
        # print map(lambda c: quad2score[c.quad], self.q)
        best_quad = max(self.q, key=lambda c: quad2score[c.quad])
        self.q.remove(best_quad)
        return best_quad

    def __nonzero__(self):
        return not not self.q


def main():
    mesh_filename = 'meshes/mini-airfoil.p.part'

    cell2nodes = read_mesh_from_file(mesh_filename, 'cell_to_ord_nodes', int, frozenset)
    node2node = read_mesh_from_file(mesh_filename, 'node_to_node', int, frozenset)
    # DEBUGGING
    global nodes2cell
    nodes2cell = invert_list(cell2nodes)
    ## END DEBUGGING

    # Setup conditions
    seed(7)
    start_node = randint(0, len(node2node) - 1)
    start_node = 58

    # Go
    cell2ord = dict()
    quad_search = QuadSearch(node2node)
    for count, c in enumerate(quad_search.visit(start_node, BestFirstSearchCandidates)):
        # print c
        cell2ord[c.get_cell_id()] = count

    def cell_labeller(old_cell, new_cell):
        if old_cell in cell2ord:
            return '*%d*' % cell2ord[old_cell]
        else:
            return old_cell

    draw(label_cells=cell_labeller)


if __name__ == '__main__':
    main()
