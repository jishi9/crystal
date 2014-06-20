
from collections import defaultdict
from itertools import imap, izip
from magic_iterators import indices_of, pairwise, PairwiseAccessor, renumber_keys, unique_values

class EdgeRegionInfo:
    def __init__(self, old_edges, (node_row_start, node_row_finish), (node_col_start, node_col_finish)):
        self.old_edges = old_edges
        self.node_row_start = node_row_start
        self.node_row_finish = node_row_finish
        self.node_col_start = node_col_start
        self.node_col_finish = node_col_finish

    def iter_edges(self):
        for row in self.old_edges[self.node_row_start : self.node_row_finish]:
            for edge in row[self.node_col_start : self.node_col_finish]:
                yield edge

    def iter_edge_pairs(self):
        for row in self.old_edges[self.node_row_start : self.node_row_finish]:
            for e1, e2 in pairwise(row[self.node_col_start : self.node_col_finish]):
                yield e1, e2


def _build_node2ordinedges(inedge2ordnodes, lim=2):
    # Sets of edges from each direction
    node2ordinedges = defaultdict(lambda: [set(), set()])

    for edge_id, nodes in enumerate(inedge2ordnodes):
        for direction, node_id in enumerate(nodes):
            assert node_id != -1
            node2ordinedges[node_id][direction].add(edge_id)
            assert len(node2ordinedges[node_id][direction]) <= lim, 'Expected only 2 edges in any given direction'

    return node2ordinedges


def _unique_values(lst):
    return len(set(lst)) == len(lst)


def _get_num_edges(edge_region):
    num_edge_rows = edge_region.node_row_finish - edge_region.node_row_start
    num_edge_cols = edge_region.node_col_finish - edge_region.node_col_start
    return num_edge_rows * num_edge_cols



class EdgeStructureFromNodeStructure(object):
    """docstring for EdgeStructureFromNodeStructure"""
    def __init__(self, structured_node_regions, structured_cell_regions, inedge2ordnodes, inedge2ordcells):
        self._inedge2ordnodes = inedge2ordnodes
        self._inedge2ordcells = inedge2ordcells

        # Get numbers
        assert len(self._inedge2ordnodes) == len(inedge2ordcells), "Number of edges must match!"
        assert len(structured_node_regions) == len(structured_cell_regions), "Number of edges must match!"

        num_edges = len(self._inedge2ordnodes)

        # Invert map
        self.node2ordinedges = _build_node2ordinedges(self._inedge2ordnodes)

        # Detect structure
        self.detect_edge_structure_and_renumber_edges(structured_node_regions, structured_cell_regions, num_edges)

        # Applying edge renumbering
        self.new_inedge2ordnodes = renumber_keys(self._inedge2ordnodes, self.newedge2oldedge)
        self.new_inedge2ordcells = renumber_keys(inedge2ordcells, self.newedge2oldedge)


    def find_common_edge(self, n1, n2, find_compass=False):
        # Get the edge shared between the two nodes: edge -> direction
        n1_edge_to_direction = { edge: direction for direction in xrange(2) for edge in self.node2ordinedges[n1][direction] }
        n2_edge_to_direction = { edge: direction for direction in xrange(2) for edge in self.node2ordinedges[n2][direction] }

        common_edges = set(n1_edge_to_direction.keys()) & set(n2_edge_to_direction.keys())
        assert len(common_edges) == 1, "Two neighbouring nodes must have exactly one edge in common! Actual %d" % len(common_edges)
        this_edge = iter(common_edges).next()

        if not find_compass:
            return this_edge

        # Find node compass
        node_direction1 = n1_edge_to_direction[this_edge]
        node_direction2 = n2_edge_to_direction[this_edge]
        node_compass = [ node_direction1, node_direction2 ]
        assert _unique_values(node_compass), "Multiple nodes pointing to the same edge from the same direction!"

        return this_edge, node_compass


    # Stream of node pairs to stream of edges
    def get_edges(self, node_pairs):
        for n1, n2 in node_pairs:
            yield self.find_common_edge(n1, n2)


    def is_internal_edge(self, edge):
        return edge < len(self._inedge2ordnodes)


    def get_edge_using_compass(self, n1, n2, compass):
        node_direction1, node_direction2 = compass

        n1_edges = self.node2ordinedges[n1][node_direction1]
        n2_edges = self.node2ordinedges[n2][node_direction2]

        common_edges = n1_edges & n2_edges
        assert len(common_edges) == 1, 'Compass inconsistent'
        this_edge = iter(common_edges).next()
        return this_edge


    # Given Structured-edge iteration space, which excludes border-edges:
    #   - Derive a compass
    #   - Obtain edge renumbering
    def add_edges_and_derive_edge_region_compass(self, edge_region_info):
        iter_internal_edges = edge_region_info.iter_edges()

        # Derive compass using first edge
        try:
            n1, n2 = iter_internal_edges.next()
        except StopIteration:
            # No structured edges! Set arbitrary compasses and return
            edge_region_info.node_compass = [0, 1]
            edge_region_info.cell_compass = [0, 1]
            return edge_region_info


        first_edge, node_compass = self.find_common_edge(n1, n2, find_compass=True)
        assert self.is_internal_edge(first_edge), 'Out of range edge (perhaps a border edge)'

        # Derive cell compass using cell ordinal
        c1, c2 = self._inedge2ordcells[first_edge]
        assert c1 != c2
        cell_compass = [0, 1] if c1 < c2 else [1, 0]


        # Add mapping: new edge to old edge
        self.newedge2oldedge.append(first_edge)

        # Mark edge as visited
        self.structured_edges.add(first_edge)

        #### Continue looping over edges
        for n1, n2 in iter_internal_edges:
            this_edge = self.get_edge_using_compass(n1, n2, node_compass)

            # Verify cell compass
            cells = self._inedge2ordcells[this_edge]
            lesser_cell = cells[cell_compass[0]]
            greater_cell = cells[cell_compass[1]]
            assert lesser_cell < greater_cell

            # Add mapping: new edge to old edge
            self.newedge2oldedge.append(this_edge)

            # Mark edge as visited
            self.structured_edges.add(this_edge)

        # Region data
        edge_region_info.node_compass = node_compass
        edge_region_info.cell_compass = cell_compass

        return edge_region_info


    def detect_edge_structure_and_renumber_edges(self, structured_node_regions, structured_cell_regions, num_edges):
        # Structured edge regions
        self.h_edge_structured_regions, self.v_edge_structured_regions = [], []

        # Renumbering map
        self.newedge2oldedge = []

        # Set of structured edges
        self.structured_edges = set()

        # Detect and renumber structured edges
        inedge2node_offset = 0
        for node_region, cell_region in zip(structured_node_regions, structured_cell_regions):
            # Find edge-region data
            horizontal_edges = HorizontalEdgeAccessor(node_region)
            vertical_edges = VerticalEdgeAccessor(node_region)
            h_inedges_info, v_inedges_info = self.find_edge_structure_boundary(horizontal_edges, vertical_edges, cell_region)


            ## Find horizontal-edge structured region
            h_edge_region = self.add_edges_and_derive_edge_region_compass(h_inedges_info)
            h_edge_region.inedge2node_offset = inedge2node_offset
            inedge2node_offset += _get_num_edges(h_edge_region)
            self.h_edge_structured_regions.append(h_edge_region)

            # Find vertical-edge structured region
            v_edge_region = self.add_edges_and_derive_edge_region_compass(v_inedges_info)
            v_edge_region.inedge2node_offset = inedge2node_offset
            inedge2node_offset += _get_num_edges(v_edge_region)
            self.v_edge_structured_regions.append(v_edge_region)

        self.unstructured_edges_offset = inedge2node_offset

        assert len(self.newedge2oldedge) == inedge2node_offset
        assert len(self.structured_edges) == inedge2node_offset

        # Renumber the unstructured edges
        for old_edge_id in xrange(num_edges):
            if old_edge_id not in self.structured_edges:
                self.newedge2oldedge.append(old_edge_id)

        assert len(self.newedge2oldedge) == num_edges


    # Find the minimum border
    # IMPORTANT first row of h_old_edges and first column of v_old_edges is to be skipped
    def find_edge_structure_boundary(self, h_old_edges, v_old_edges, cell_region):

        def edges_on_row(old_edges, row_index):
            return self.get_edges(old_edges[row_index])

        def edges_on_col(old_edges, col_index):
            return self.get_edges(old_edges.iter_col(col_index))

        def all_internal_edges(edge_iter):
            return all(map(self.is_internal_edge, edge_iter))


        first_edge_row_no_borders = (
            all_internal_edges(edges_on_row(h_old_edges, 1))
            and
            all_internal_edges(edges_on_row(v_old_edges, 0))
        )

        last_edge_row_no_borders = (
            all_internal_edges(edges_on_row(h_old_edges, -1))
            and
            all_internal_edges(edges_on_row(v_old_edges, -1))
        )

        first_edge_col_no_borders = (
            all_internal_edges(edges_on_col(h_old_edges, 0))
            and
            all_internal_edges(edges_on_col(v_old_edges, 1))
        )

        last_edge_col_no_borders = (
            all_internal_edges(edges_on_col(h_old_edges, -1))
            and
            all_internal_edges(edges_on_col(v_old_edges, -1))
        )


        # Edge rows
        edge_row_start = 0 if first_edge_row_no_borders else 1
        edge_row_finish = -1 if last_edge_row_no_borders else -2
        edge_col_start = 0 if first_edge_col_no_borders else 1
        edge_col_finish = -1 if last_edge_col_no_borders else -2

        # Node rows
        h_node_row_start = h_old_edges.edge_row_to_node_row(1 + edge_row_start)
        v_node_row_start = v_old_edges.edge_row_to_node_row(edge_row_start)
        node_row_start = max(h_node_row_start, v_node_row_start, cell_region.row_start)

        h_node_row_finish = 1 + h_old_edges.edge_row_to_node_row(edge_row_finish)
        v_node_row_finish = 1 + v_old_edges.edge_row_to_node_row(edge_row_finish)
        node_row_finish = min(h_node_row_finish, v_node_row_finish, cell_region.row_finish)

        h_node_col_start = h_old_edges.edge_col_to_node_col(edge_col_start)
        v_node_col_start = v_old_edges.edge_col_to_node_col(1 + edge_col_start)
        node_col_start = max(h_node_col_start, v_node_col_start, cell_region.col_start)

        h_node_col_finish = 1 + h_old_edges.edge_col_to_node_col(edge_col_finish)
        v_node_col_finish = 1 + v_old_edges.edge_col_to_node_col(edge_col_finish)
        node_col_finish = min(h_node_col_finish, v_node_col_finish, cell_region.col_finish)


        # Structured-edge iteration space, which excludes border-edges
        return [ EdgeRegionInfo(old_edges, (node_row_start, node_row_finish), (node_col_start, node_col_finish))
                 for old_edges in (h_old_edges, v_old_edges) ]


class AbstractEdgeAccessor(object):
    """docstring for AbstractEdgeAccessor"""
    def edge_row_to_node_row(self, edge_row):
        assert -self.num_edge_rows <= edge_row < self.num_edge_rows
        return edge_row if edge_row >= 0 else self.num_edge_rows + edge_row

    def edge_col_to_node_col(self, edge_col):
        assert -self.num_edge_cols <= edge_col < self.num_edge_cols
        return edge_col if edge_col >= 0 else self.num_edge_cols + edge_col


class HorizontalEdgeAccessor(AbstractEdgeAccessor):
    """docstring for HorizontalEdgeAccessor"""
    def __init__(self, node_region):
        self.node_region = node_region
        self.num_edge_rows = len(node_region)
        self.num_edge_cols = max(0, len(node_region[0]) - 1)

    def iter_edges(self):
        return (edge for row in self.iter_rows() for edge in row)

    def iter_rows(self):
        return self[:]

    def iter_col(self, col):
        for row in self.iter_rows():
            yield row[col]

    def __len__(self):
        return len(self.node_region)

    def __getitem__(self, rng):
        if isinstance(rng, int):
            return PairwiseAccessor(self.node_region[rng])

        elif isinstance(rng, slice):
            return imap(PairwiseAccessor, self.node_region[rng])

        else:
            raise TypeError("Illegal index type %s", type(rng).__name__)


class VerticalEdgeAccessor(AbstractEdgeAccessor):
    """docstring for VerticalEdgeAccessor"""
    def __init__(self, node_region):
        self.node_region = node_region
        self.num_edge_rows = max(0, len(node_region) - 1)
        self.num_edge_cols = len(node_region[0])

    def iter_edges(self):
        return ( e for row in self.iter_rows() for e in row )

    def iter_rows(self):
        return self[:]

    def iter_col(self, col):
        for row_a, row_b in PairwiseAccessor(self.node_region):
            yield (row_a[col], row_b[col])

    def __len__(self):
        return max(0, len(self.node_region)-1)

    def __getitem__(self, rng):
        if isinstance(rng, int):
            return izip(*PairwiseAccessor(self.node_region)[rng])

        elif isinstance(rng, slice):
            return ( zip (r1, r2) for r1, r2 in PairwiseAccessor(self.node_region)[rng] )

        else:
            raise TypeError("Illegal index type %s", type(rng).__name__)

