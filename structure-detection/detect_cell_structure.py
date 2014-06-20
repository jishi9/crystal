from collections import defaultdict, namedtuple
from itertools import izip
from magic_iterators import indices_of, intersection, pairwise, renumber_keys, unique_values

# A quad is formed by four nodes, as well as their positions along rows and columns
Quad = namedtuple('Quad', ['row1', 'row2', 'col1', 'col2', 'r1c1', 'r1c2', 'r2c1', 'r2c2', 'first_in_row'])

# IMPORTANT: This defines the compass ordering
Quad.getNodes = lambda self: (self.r1c1, self.r1c2, self.r2c1, self.r2c2)

def _pairwise_enumerate_2d(grid2d, row_offset=0, col_offset=0):
    for (r1_idx, r1_lst), (r2_idx, r2_lst) in pairwise(enumerate(grid2d, row_offset)):
        first_in_row = True
        for (c1_idx, (r1c1, r2c1)), (c2_idx, (r1c2, r2c2)) in pairwise(enumerate(izip(r1_lst, r2_lst), col_offset)):
            yield Quad(row1=r1_idx, row2=r2_idx, col1=c1_idx, col2=c2_idx,
                        r1c1=r1c1, r1c2=r1c2, r2c1=r2c1, r2c2=r2c2, first_in_row=first_in_row)
            first_in_row = False

def _invert_cell2ordnodes(cell2ordnodes):
    node2ordcells = defaultdict(lambda: [-1, -1, -1, -1])

    for cell_id, nodes in enumerate(cell2ordnodes):
        for direction, node_id in enumerate(nodes):
            assert node_id != -1
            assert node2ordcells[node_id][direction] == -1, "Multiple nodes adjacent to the same cell from the same direction: cell %d and node %d" % (cell_id, node_id)
            node2ordcells[node_id][direction] = cell_id

    return node2ordcells

def _print_partial_region(header, region):
    print header
    row_max = min(3, len(region))
    col_max = min(10, len(region[0]))

    for row in region[:row_max]:
        print row[:col_max], '...'
    print '...'


def _find_topleftmost_structured_quad(node_region, node2ordcells):
    # Find the top-left quad
    for quad in _pairwise_enumerate_2d(node_region):
        quad_nodes_to_cells = [ node2ordcells[n] for n in quad.getNodes() ]
        common_cells = intersection(*quad_nodes_to_cells)
        common_cells.discard(-1)

        # No cells in common. Advance to next quad.
        if not common_cells: continue

        # Multiple cells in common - wraparound case
        if len(common_cells) > 1: raise NotImplementedError('Wraparound case')

        # Get the unique cell that is adjacent to all four nodes
        this_cell = iter(common_cells).next()

        # Get all directions that point to this_cell
        matching_directions = [ indices_of(this_cell, cs) for cs in quad_nodes_to_cells ]
        assert all(len(n) == 1 for n in matching_directions), "A node is pointing to the same cell multiple times!"

        # For each node, get its unique direction wrt this_cell
        compass = [ n[0] for n in matching_directions ]
        assert unique_values(compass), "Multiple nodes pointing to the same cell from the same direction!"

        # End search
        return quad, compass

    # None found!
    raise NotImplementedError("NO CELL REGION INNIT! :'(")


def _find_remaining_structure(topleft_quad, compass, node_region, node2ordcells):
    row_start = topleft_quad.row1
    row_finish = None
    col_start = topleft_quad.col1
    col_finish = None

    current_row = []
    structured_cells = [ current_row ]

    # Traverse first row, from col_start until the last structured column reached
    first2rows = ( node_region[0][col_start:] , node_region[1][col_start:] )
    for quad in _pairwise_enumerate_2d(first2rows):
        # Find pointed-to cells
        pointed_to_cells = set()
        for node, direction in izip(quad.getNodes(), compass):
            pointed_to_cells.add(node2ordcells[node][direction])
        pointed_to_cells.discard(-1)

        # Nodes do not all point to a unique cell - passed last structured column
        if len(pointed_to_cells) != 1:
            col_finish = quad.col1
            break

        # Add cell to structured region
        cell = iter(pointed_to_cells).next()
        current_row.append(cell)

    if not col_finish:
        col_finish = quad.col2

    # Traverse remaining rows, from col_start to col_finish
    second_row_onwards = ( row[col_start:col_finish+1] for row in node_region[1:] )
    for quad in _pairwise_enumerate_2d(second_row_onwards, row_offset=1, col_offset=col_start):
        # If this is the first quad in this row, append a new row to structured_cells
        if quad.first_in_row:
            current_row = []
            structured_cells.append(current_row)

        # Find pointed-to cells
        pointed_to_cells = set(node2ordcells[node][direction]
            for node, direction in izip(quad.getNodes(), compass))

        # Nodes do not all point to a unique cell - remove this row and stop
        if len(pointed_to_cells) != 1:
            structured_cells.pop()
            row_finish = quad.row1
            break

        # Add pointed-to cell to structured region
        cell = iter(pointed_to_cells).next()
        current_row.append(cell)

    if not row_finish:
        print 'finish him'
        row_finish = quad.row2


    num_cell_rows = row_finish - row_start
    num_cell_cols = col_finish - col_start
    num_node_rows = len(node_region)
    num_node_cols = len(node_region[0])

    print 'node region: %d x %d' %(num_node_rows, num_node_cols)
    print 'cell region: %d x %d' %(num_cell_rows, num_cell_cols)
    print 'cell region: rows %d-%d, cols: %d-%d' %(row_start, row_finish, col_start, col_finish)

    assert len(structured_cells) == num_cell_rows, 'Number of rows mismatch'
    assert all(len(row) == num_cell_cols for row in structured_cells), 'Number of cols mismatch'

    cell_region = StructuredCellRegion()
    cell_region.structured_cells = structured_cells
    cell_region.row_start = row_start
    cell_region.row_finish = row_finish
    cell_region.col_start = col_start
    cell_region.col_finish = col_finish
    cell_region.num_cell_rows = num_cell_rows
    cell_region.num_cell_cols = num_cell_cols
    cell_region.compass = compass

    return cell_region


def _renumber_cells(structured_cell_regions, num_cells):
    # Construct reordering for structured cells
    oldcell2newcell = dict()
    newcell2oldcell = []
    visited_structured_cells = set()
    current_offset = 0
    current_new_node_id = 0
    for cell_region in structured_cell_regions:
        cell_region.cells_offset = current_offset
        current_offset += cell_region.num_cell_rows * cell_region.num_cell_cols
        for row in cell_region.structured_cells:
            for old_cell_id in row:
                newcell2oldcell.append(old_cell_id)
                visited_structured_cells.add(old_cell_id)
                oldcell2newcell[old_cell_id] = current_new_node_id
                current_new_node_id += 1

    assert len(visited_structured_cells) == len(newcell2oldcell), "Duplicate structured cells"
    assert len(visited_structured_cells) == current_new_node_id

    # Construct reordering for unstructured cells
    for old_cell_id in xrange(num_cells):
        if old_cell_id not in visited_structured_cells:
            newcell2oldcell.append(old_cell_id)
            oldcell2newcell[old_cell_id] = current_new_node_id
            current_new_node_id += 1

    unstructured_cells_offset = current_offset
    num_structured_cells = len(visited_structured_cells)
    assert len(oldcell2newcell) == len(newcell2oldcell) == num_cells

    return oldcell2newcell, newcell2oldcell, num_structured_cells, unstructured_cells_offset


class StructuredCellRegion:
    pass


class UnstructuredCellRegion:
    pass


class CellStructureFromNodeStructure(object):
    """Given structured node regions, derives structured cell regions"""
    def __init__(self, structured_node_regions, cell2ordnodes):
        num_cells = len(cell2ordnodes)
        node2ordcells = _invert_cell2ordnodes(cell2ordnodes)

        structured_cell_regions = []
        for node_region in structured_node_regions:
            # Preview of node region
            _print_partial_region('Structured node region:', node_region)

            # Extract cell structured region
            topleft_quad, compass = _find_topleftmost_structured_quad(node_region, node2ordcells)
            cell_region = _find_remaining_structure(topleft_quad, compass, node_region, node2ordcells)
            structured_cell_regions.append(cell_region)

        # Renumber cells
        oldcell2newcell, newcell2oldcell, num_structured_cells, unstructured_cells_offset = _renumber_cells(structured_cell_regions, num_cells)

        # Determine number of unstructured cells
        assert num_structured_cells == unstructured_cells_offset
        num_unstructured_cells = num_cells - num_structured_cells

        # Unstructured cell region metadata
        unstructured_cell_regions = UnstructuredCellRegion()
        unstructured_cell_regions.unstructured_cells_offset = unstructured_cells_offset
        unstructured_cell_regions.num_unstructured_cells = num_unstructured_cells

        # Apply cell reordering to original map
        new_cell2ordnodes = renumber_keys(cell2ordnodes, newcell2oldcell)

        # Save
        self.new_cell2ordnodes = new_cell2ordnodes
        self.structured_cell_regions = structured_cell_regions
        self.unstructured_cell_regions = unstructured_cell_regions
        self.oldcell2newcell = oldcell2newcell
