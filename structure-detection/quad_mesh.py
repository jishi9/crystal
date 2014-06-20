from itertools import product
from random import shuffle

### Build example grid ###
def enumerate2d(rows, cols):
    return product(xrange(rows), xrange(cols))


class RandomFrozenSet(frozenset):
    def __init__(self, *args, **kwargs):
        super(RandomFrozenSet, self).__init__(*args, **kwargs)

    def __iter__(self):
        elems = list(super(RandomFrozenSet, self).__iter__())
        shuffle(elems)
        return iter(elems)


class GridMesh(object):
    """Class responsible for building sample meshes"""
    def __init__(self, rows, cols, add_neighbour_func):
        self.rows, self.cols = rows, cols
        self._build(add_neighbour_func)

    # Build node adjacency map
    def _build(self, try_add_neighbour):
        # Initialise grid and node_map
        self.grid = [ [(r,c) for c in xrange(self.cols)] for r in xrange(self.rows) ]
        self.node_map = dict()

        for r, c in enumerate2d(self.rows, self.cols):
            node = self.grid[r][c]
            neighbours = set()

            try_add_neighbour(self, neighbours, r+1, c)
            try_add_neighbour(self, neighbours, r-1, c)
            try_add_neighbour(self, neighbours, r, c+1)
            try_add_neighbour(self, neighbours, r, c-1)

            self.node_map[node] = RandomFrozenSet(neighbours)

    # No wrap
    def add_neighbour_no_wrap(self, node_neighbors, neighbor_row, neighbor_col):
        if (0 <= neighbor_row < self.rows) and (0 <= neighbor_col < self.cols):
            node_neighbors.add(self.grid[neighbor_row][neighbor_col])

    # Wrap around
    def add_neighbour_wrap(self, node_neighbors, neighbor_row, neighbor_col):
        node_neighbors.add(self.grid[neighbor_row % self.rows][neighbor_col % self.cols])

    # Rows wrap around
    def add_neighbour_row_wrap(self, node_neighbors, neighbor_row, neighbor_col):
        if 0 <= neighbor_col < self.cols:
            node_neighbors.add(self.grid[neighbor_row % self.rows][neighbor_col])

    # Columns wrap around
    def add_neighbour_col_wrap(self, node_neighbors, neighbor_row, neighbor_col):
        if 0 <= neighbor_row < self.rows:
            node_neighbors.add(self.grid[neighbor_row][neighbor_col % self.cols])

class SizeLimiter(object):
    def __init__(self, max_rows, max_cols):
      self.ranges = SizeLimiter.iter_range(max_rows, max_cols)
      self.next()

    @staticmethod
    def iter_range(max_rows, max_cols):
      while True:
        yield (max_rows, max_cols)

    def next(self):
      self.row_max, self.col_max = next(self.ranges)

    def cols_ok(self, cols):
      return cols <= self.col_max

    def rows_ok(self, rows):
      return rows <= self.row_max



class DetectQuadStructure(object):
    """Class used to detect quad-based structures"""
    def __init__(self, node_map, start_node=None, max_rows=999999, max_cols=999999, num_node_neighbours=4):
        self.node_map = [ RandomFrozenSet(ns) for ns in node_map ]
        num_nodes = len(self.node_map)
        self.visited = set()
        self.not_visited = set(xrange(num_nodes))
        self.num_node_neighbours = num_node_neighbours
        self.limiter = SizeLimiter(max_rows, max_cols)
        if start_node: self.detect_region_from(start_node)


    def detect_region_from(self, start_node):
        self.limiter.next() # Note this skips first one!
        self.current_row_index = 0  # DEBUG
        self.current_row, self.next_row = self.output = [ [], [] ]

        # Get first quad
        init_quad = self.find_quad_from_point(start_node)

        # Build first row in the forward direction
        cols_expanded = 3
        try:
            current_quad = init_quad
            while self.limiter.cols_ok(cols_expanded):
                current_quad = self.extend_quad_row(*current_quad)
                cols_expanded += 1
        except StructureException, e:
            print 'Caught structure assertion in first row (forwards):', e

        # Build first row in the backward direction
        try:
            current_quad = self.mirror_cols(*init_quad)
            while self.limiter.cols_ok(cols_expanded):
                current_quad = self.extend_quad_row(*current_quad, reverse_direction=True)
                cols_expanded += 1
        except StructureException, e:
            print 'Caught structure assertion in first row (backwards):', e


        prev_prev_row, prev_row  = self.current_row, self.next_row

        # Too narrow (2 or less columns of nodes)
        try:
            self.assert_structure(len(prev_row) >= 3, 'Region too narrow')
            self.assert_structure(len(prev_prev_row) >= 3, 'Region too narrow')
        except StructureException:
            # Remove visited nodes
            self.unvisit()
            raise


        self.advance_row()

        # Build subsequent rows
        rows_expanded = 3
        try:
            while self.limiter.rows_ok(rows_expanded):
                self.build_row_from_prev_rows(prev_prev_row, prev_row)
                self.advance_row()
                prev_prev_row, prev_row = prev_row, self.current_row
                rows_expanded += 1
        except StructureException, e:
            print 'Caught structure assertion in subsequent row:', e

        # Discard last row (may be partially populated)
        assert self.output.pop() == self.next_row


        # Build previous rows
        prev_prev_row, prev_row = self.current_row, self.next_row = self.output[1], self.output[0]
        self.advance_row(reverse_direction=True)

        try:
            while self.limiter.rows_ok(rows_expanded):
                self.build_row_from_prev_rows(prev_prev_row, prev_row)
                self.advance_row(reverse_direction=True)
                prev_prev_row, prev_row = prev_row, self.current_row
                rows_expanded += 1
        except StructureException, e:
            print 'Caught structure assertion in previous row:', e

        # Discard first row (may be partially populated)
        assert self.output.pop(0) == self.next_row

        return self.output


    def get_structured_grid(self):
        return self.output

    # Get neighbours of the given node
    def neighbours(self, node):
        ns = self.node_map[node]
        self.assert_structure(len(ns) == self.num_node_neighbours,
            'node %d should have %d neighbours' % (node, self.num_node_neighbours))
        return ns


    # Assert a certain fact about structure
    @staticmethod
    def assert_structure(cond, message):
        if(not cond):
            raise StructureException(message)


    def advance_row(self, reverse_direction=False):
        self.current_row, self.next_row = self.next_row, []

        if reverse_direction:
            self.current_row_index -= 1 # Debug
            self.output.insert(0, self.next_row)
        else:
            self.current_row_index += 1 # DEBUG
            self.output.append(self.next_row)


    # Unvisit nodes in output
    def unvisit(self):
        unvisited_nodes = { node for row in self.output for node in row }
        self.visited.difference_update(unvisited_nodes)
        self.not_visited.update(unvisited_nodes)


    def _add_node_to_row(self, node, lst, end):
        self.assert_structure(node not in self.visited, 'Already visited this node')
        self.visited.add(node)
        self.not_visited.remove(node)

        if end:
            lst.append(node)
        else:
            lst.insert(0, node)

    def append_to_this_row(self, node):
        self._add_node_to_row(node, self.current_row, end=True)

    def append_to_next_row(self, node):
        self._add_node_to_row(node, self.next_row, end=True)

    def prepend_to_this_row(self, node):
        self._add_node_to_row(node, self.current_row, end=False)

    def prepend_to_next_row(self, node):
        self._add_node_to_row(node, self.next_row, end=False)


    def _remove_node_from_row(self, node, lst, end):
        self.assert_structure(node in self.visited, 'Should have already visited this node')
        self.visited.remove(node)
        self.not_visited.add(node)

        if end:
            assert lst.pop(-1) == node
        else:
            assert lst.pop(0) == node

    def pop_back_from_this_row(self, node):
        self._remove_node_from_row(node, self.current_row, end=True)

    def pop_back_from_next_row(self, node):
        self._remove_node_from_row(node, self.next_row, end=True)

    def pop_front_from_this_row(self, node):
        self._remove_node_from_row(node, self.current_row, end=False)

    def pop_front_from_next_row(self, node):
        self._remove_node_from_row(node, self.next_row, end=False)


    def mirror_cols(self, r1c1, r1c2, r2c1, r2c2):
        return (r1c2, r1c1, r2c2, r2c1)


    def find_quad_from_point(self, start_node):
        self.assert_structure(start_node not in self.visited, 'Start node is already visited')

        r2c1 = start_node
        a, b, c, _ = self.neighbours(start_node)

        common_neighbors = self.neighbours(a) & self.neighbours(b)
        num_common_neighbors = len(common_neighbors)

        # CASE 1:
        # a and b are on the same line
        #
        #  |            |           |
        #  |            |           |
        #  |            |           |
        #  a ----- start_node ----- b
        #  |            |           |
        #  |            |           |
        #  |            |           |
        #
        if num_common_neighbors == 1:
            (common_node,) = common_neighbors
            self.assert_structure(common_node == start_node, 'if a and b have one neighbour in common, it should be start_node')

            # Swap b and c, and fall-through to next case
            a, b, c = a, c, b
            common_neighbors = self.neighbours(a) & self.neighbours(b)
            num_common_neighbors = len(common_neighbors)

        # CASE 2:
        # a and b are opposite corners on a quad face
        #
        #           a -----------------?
        #           |                  |
        #           |                  |
        #           |                  |
        #           |                  |
        #           |                  |
        #           |                  |
        #           |                  |
        #        start_node -----------b

        if num_common_neighbors == 2:
            com1, com2 = common_neighbors
            self.assert_structure(com1 != com2, 'Neighbours should be distinct!')

            r1c1, r2c2 = a, b

            if com1 == start_node:
                r1c2 = com2
            elif com2 == start_node:
                r1c2 = com1
            else:
                self.assert_structure(False, 'One of the common neighbours of a and b should be start_node')


        # CASE 3:
        # num_common_neighbors not in (1, 2)
        else:
            self.assert_structure(False, 'unexpected num_common_neighbors')


        # Structure thus far:
        #
        #   r1c1 ---------- r1c2
        #    |                |
        #    |                |
        #    |                |
        #    |                |
        #    |                |
        #    |                |
        #    |                |
        #   r2c1 ---------- r2c2
        #

        try:
            pop_count = 0
            #Store
            self.append_to_this_row(r1c1) ; pop_count += 1
            self.append_to_this_row(r1c2) ; pop_count += 1
            self.append_to_next_row(r2c1) ; pop_count += 1
            self.append_to_next_row(r2c2) ; pop_count += 1

            # Ensure that the remaining unmatched edges are not attached to any visited node
            self.assert_structure(
                self.neighbours(r1c1) & self.visited == {r1c2, r2c1} and
                self.neighbours(r1c2) & self.visited == {r1c1, r2c2} and
                self.neighbours(r2c2) & self.visited == {r1c2, r2c1} and
                self.neighbours(r2c1) & self.visited == {r1c1, r2c2},
                'Edges on the (thus far) structured borders are in contact with visited nodes (find_quad_from_point)')
        except StructureException:
            # Revert action
            if pop_count >= 4: self.pop_back_from_next_row(r2c2)
            if pop_count >= 3: self.pop_back_from_next_row(r2c1)
            if pop_count >= 2: self.pop_back_from_this_row(r1c2)
            if pop_count >= 1: self.pop_back_from_this_row(r1c1)
            raise

        return r1c1, r1c2, r2c1, r2c2


    ### Carry through: r1c1, r1c2, r2c1, r2c2
    def extend_quad_row(self, r1c1, r1c2, r2c1, r2c2, reverse_direction=False):
        new_r1c2_neighbors = self.neighbours(r1c2) - {r1c1, r2c2}
        self.assert_structure(len(new_r1c2_neighbors) == 2, 'r1c1 and r2c2 Should have been removed')

        new_r2c2_neighbors = self.neighbours(r2c2) - {r2c1, r1c2}
        self.assert_structure(len(new_r2c2_neighbors) == 2, 'r2c1 and r1c2 Should have been removed')

        a, b = new_r1c2_neighbors
        c, d = new_r2c2_neighbors

        try:
            self.neighbours(c)
        except StructureException:
            c_ok = False
        else:
            c_ok = True

        if c_ok and a in self.neighbours(c):
            r1c3, r2c3 = a, c
        elif c_ok and b in self.neighbours(c):
            r1c3, r2c3 = b, c
        elif a in self.neighbours(d):
            r1c3, r2c3 = a, d
        else:
            self.assert_structure(b in self.neighbours(d), 'None of the new neighbours of r1c2 and r2c2 intersect')
            r1c3, r2c3 = b, d

        # Non-reverse_direction case:
        # c.f In reverse_direction case, this would be mirrored left-right,
        #     such that the columns appear in reverse order (c3 - c2 -c1)
        #
        # IMPORTANT: This includes the input quad, which should also be mirrored accordingly
        #
        #    ? --------------a/b ------------- ?
        #    |                |                |
        #    |                |                |
        #    |                |                |
        #    |                |                |
        #    |                |                |
        #    |                |                |
        #    |                |                |
        #   r1c1 ---------- r1c2 ------------- a/b = r1c3
        #    |                |                |
        #    |                |                |
        #    |                |                |
        #    |                |                |
        #    |                |                |
        #    |                |                |
        #    |                |                |
        #   r2c1 ---------- r2c2 ------------- c/d = r2c3
        #    |                |                |
        #    |                |                |
        #    |                |                |
        #    |                |                |
        #    |                |                |
        #    |                |                |
        #    |                |                |
        #    ? ------------- c/d ------------- ?

        # Store
        try:
            pop_count = 0
            if reverse_direction:
                self.prepend_to_this_row(r1c3) ; pop_count += 1
                self.prepend_to_next_row(r2c3) ; pop_count += 1
            else:
                self.append_to_this_row(r1c3) ; pop_count += 1
                self.append_to_next_row(r2c3) ; pop_count += 1

            self.assert_structure(
                self.neighbours(r1c3) & self.visited == {r1c2, r2c3} and
                self.neighbours(r2c3) & self.visited == {r2c2, r1c3},
                'Edges on the (thus far) structured borders are in contact with visited nodes (extend_quad_row)')
        except StructureException:
            # Revert action
            if reverse_direction:
                if pop_count >= 2: self.pop_front_from_next_row(r2c3)
                if pop_count >= 1: self.pop_front_from_this_row(r1c3)
            else:
                if pop_count >= 2: self.pop_back_from_this_row(r1c3)
                if pop_count >= 1: self.pop_back_from_next_row(r2c3)
            raise

        # Return this cell
        return r1c2, r1c3, r2c2, r2c3


    def build_row_from_prev_rows(self, prev_prev_row, prev_row):
        row1, row2 = prev_prev_row, prev_row

        ## Find first node in row (r3c1)
        #
        #            r1c1 ---- r1c2 ---- r1c3 ----
        #             |         |         |
        #             |         |         |
        #             |         |         |
        #   a/b ---- r2c1 ---- r2c2 ---- r2c3 ----
        #             |         |
        #             |         |
        #             |         |
        #        a/b=r3c1 ---- r3c2
        #

        # Get r3c2
        r3c2_candidates = self.neighbours(row2[1]) - {row2[0], row2[2], row1[1]}
        self.assert_structure(len(r3c2_candidates) == 1, "r3c2 should be uniquely identifiable through row2[1]'s neighbours")

        (r3c2, ) = r3c2_candidates
        penultimate_node = r3c2


        # Candidates for r3c1
        r3c1_candidates = self.neighbours(row2[0]) - {row2[1], row1[0]}
        self.assert_structure(len(r3c1_candidates) == 2, 'First node in row2 should have exactly two unvisited neighbours')
        a, b = r3c1_candidates

        # Use r3c2 to determine r3c1
        if a in self.neighbours(r3c2):
            r3c1 = a
        else:
            self.assert_structure(b in self.neighbours(r3c2), 'r3c1 should be a neighbor of r3c2')
            r3c1 = b

        try:
            pop_count = 0
            self.append_to_next_row(r3c1) ; pop_count += 1
            self.append_to_next_row(r3c2) ; pop_count += 1

            self.assert_structure(
                self.neighbours(r3c1) & self.visited == {row2[0], r3c2} and
                self.neighbours(r3c2) & self.visited == {row2[1], r3c1},
                'Edges on the (thus far) structured borders are in contact with visited nodes (build_row_from_prev_rows)')
        except StructureException:
            # Revert last added nodes
            if pop_count >= 2: self.pop_back_from_next_row(r3c2)
            if pop_count >= 1: self.pop_back_from_next_row(r3c1)
            raise



        ## Find all remaining nodes in this row, apart from the last
        #
        #     r1c-1 ---- r1c+0 ---- r1c+1 ----
        #       |          |          |
        #       |          |          |
        #       |          |          |
        #     r2c-1 ---- r2c+0 ---- r2c+1 ----
        #       |          |
        #       |          |
        #       |          |
        #     r3c-1 ---- r3c+0
        #             = penultimate_node
        #
        for col in xrange(2, len(row2)-1):
            candidates = self.neighbours(row2[col]) - {row2[col-1], row2[col+1], row1[col]}
            self.assert_structure(len(candidates) == 1, 'Node discovered below an existing row should be uniquely identifiable')
            penultimate_node, previous_node = next(iter(candidates)), penultimate_node
            try:
                pop_count = 0
                self.append_to_next_row(penultimate_node) ; pop_count += 1
                self.assert_structure(self.neighbours(penultimate_node) & self.visited == {previous_node, row2[col]},
                    'Edges on the (thus far) structured borders are in contact with visited nodes (build_row_from_prev_rows2)')
            except StructureException:
                # Revert action
                if pop_count >= 1: self.pop_back_from_next_row(penultimate_node)
                raise


        ## Find last node in row
        #
        #    ---- r1cn-2 -------- r1cn-1
        #           |              |
        #           |              |
        #           |              |
        #    ---- r2cn-2 -------- r2cn-1
        #           |              |
        #           |              |
        #           |              |
        #         r3cn-2 -------- last_node
        #  = penultimate_node
        #
        last_node_candidates = self.neighbours(penultimate_node) & self.neighbours(row2[-1]) - {row2[-2]}
        self.assert_structure(len(last_node_candidates) == 1, 'Last node in row should be uniquely identifiable')
        (last_node, ) = last_node_candidates
        try:
            pop_count = 0
            self.append_to_next_row(last_node) ; pop_count += 1
            self.assert_structure(self.neighbours(last_node) & self.visited == {penultimate_node, row2[-1]},
                'Edges on the (thus far) structured borders are in contact with visited nodes (build_row_from_prev_rows3)')
        except StructureException:
            # Revert action
            if pop_count >= 1: self.pop_back_from_next_row(last_node)
            raise



class StructureException(Exception):
    """docstring for StructureException"""
    def __init__(self, message):
        super(StructureException, self).__init__(message)



def main():
    # Build a mesh
    rows, cols = 20, 30
    # add_neighbour_func = GridMesh.add_neighbour_no_wrap
    add_neighbour_func = GridMesh.add_neighbour_no_wrap
    mesh = GridMesh(rows, cols, add_neighbour_func)

    # Get start point
    start_node = mesh.grid[5][11]

    # Detect structure
    structured_grid = DetectQuadStructure(mesh.node_map, start_node)

    # Print grid
    for row in structured_grid.output:
        print row


if __name__ == '__main__':
    main()
