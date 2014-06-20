#!/usr/bin/python

#
#      0   up  1
#       *-----*
# left  |     | right
#       |     |
#       *-----*
#      3  down  2
#

from collections import defaultdict, deque


class Cell(object):
	UP, DOWN, LEFT, RIGHT = 'up', 'down', 'left', 'right'

	direction2nodeindex = {
		UP:    0,
		RIGHT: 1,
		DOWN:  2,
		LEFT:  3
	}

	direction2opposite = {
		UP:    DOWN,
		RIGHT: LEFT,
		DOWN:  UP,
		LEFT:  RIGHT
	}

	@staticmethod
	def opposite_of(direction):
		return Cell.direction2opposite[direction]


	def __init__(self, cell_id, (n1, n2, n3, n4), etc, **kwargs):
		self.__dict__.update(kwargs)

		self.cell_id = cell_id
		self.nodes = deque((n1, n2, n3, n4))
		self.etc = etc
		self.up = self.right = self.down = self.left = None

		self.direction2edge = {
			Cell.UP:    lambda: Edge(self.nodes[0], self.nodes[1]),
			Cell.RIGHT: lambda: Edge(self.nodes[1], self.nodes[2]),
			Cell.DOWN:  lambda: Edge(self.nodes[2], self.nodes[3]),
			Cell.LEFT:  lambda: Edge(self.nodes[3], self.nodes[0])
		}

		self.direction2cell = {
			Cell.UP:    lambda: self.up,
			Cell.RIGHT: lambda: self.right,
			Cell.DOWN:  lambda: self.down,
			Cell.LEFT:  lambda: self.left
		}


	def __repr__(self):
		return '<Cell %d>' % self.cell_id

	def get_nodes_at(self, direction):
		return self.direction2edge[direction]()

	def fix_given_nodes_to_direction(self, (n1, n2), direction):
		assert self.up == self.right == self.down == self.left == None

		n1_pos = [ i for i in xrange(4) if self.nodes[i] == n1 ][0]
		n2_pos = (n1_pos + 1) % 4
		assert self.nodes[n2_pos] == n2

		# Rotate everything
		expected_n1_pos = Cell.direction2nodeindex[direction]
		rotation_amount = expected_n1_pos - n1_pos
		self.nodes.rotate(rotation_amount)

	def get_cell_at(self, direction):
		return self.direction2cell[direction]()

	def set_cell_at(self, direction, that_cell):
		if direction == Cell.UP:
			self.up = that_cell
		elif direction == Cell.RIGHT:
			self.right = that_cell
		elif direction == Cell.DOWN:
			self.down = that_cell
		elif direction == Cell.LEFT:
			self.left = that_cell

	def find_edge_direction(self, edge):
		for direction, that_edge in self.direction2edge.iteritems():
			if edge == that_edge():
				return direction
		return None


class Edge(object):
	"""docstring for Edge"""
	def __init__(self, a, b):
		assert a != b
		self.a = a
		self.b = b

	def __repr__(self):
		return "Edge(%d,%d)" % (self.a, self.b)

	def __iter__(self):
		yield self.a
		yield self.b

	def same_direction_as(self, other):
		return (self.a, self.b) == (other.a, other.b)

	def opposite_direction_as(self, other):
		return (self.b, self.a) == (other.a, other.b)

	def invert(self):
		return Edge(self.b, self.a)

	def __eq__(self, other):
		return (self.a, self.b) == (other.a, other.b)

	def __hash__(self):
		return hash((self.a, self.b))


class MehEdge(Edge):
	"""docstring for MehEdge"""
	def __init__(self, a, b):
		super(MehEdge, self).__init__(a, b)

	def __eq__(self, other):
		me = (self.a, self.b)
		me_rev = (self.b, self.a)
		them = (other.a, other.b)
		return me == them or me_rev == them

	def __hash__(self):
		x, y = self.a, self.b
		d = x - y;
		min_val = y + (d & d>>31)
		max_val = x - (d & d>>31)

		return max_val<<32 | min_val;

def toMehEdge(edge):
	return MehEdge(edge.a, edge.b)


# Read mesh
def iter_msh_cells(in_file, out_file):
	# Skip to elements
	while True:
		line = in_file.readline()
		out_file.write(line)
		if line.startswith('$Elements'):
			break

	line = in_file.readline()
	out_file.write(line)
	num_elems = int(line)

	for line in in_file:
		if line.startswith('$EndElements'):
			break

		fields = map(int, line.split())
		if fields[1] !=3:
			out_file.write(line)
			continue

		cell_id, a, b, c, d, n1, n2, n3, n4 = fields
		nodes = (n1, n2, n3, n4)
		etc = (a,b,c,d)

		yield (cell_id, nodes, etc)


out_filename = '/tmp/new.msh'

with open('dat/naca0012.msh', 'r') as in_file, open(out_filename, 'w') as out_file:
	cell2nodes = iter_msh_cells(in_file, out_file)

	cells = dict()
	edges2cells = dict()
	cell2cells = defaultdict(set)


	# Build cells
	for cell_id, nodes, etc in cell2nodes:
		cells[cell_id] = Cell(cell_id, nodes, etc)


	for cell in cells.itervalues():
		for direction in (Cell.UP, Cell.DOWN, Cell.LEFT, Cell.RIGHT):
			edge = cell.get_nodes_at(direction)
			assert edge not in edges2cells
			edges2cells[edge] = cell

	visited_a = set()
	for init_cell in cells.itervalues():
		if init_cell in visited_a: continue
		visited_a.add(init_cell)

		south_edge = init_cell.get_nodes_at(Cell.DOWN)

		prev_edge = south_edge

		while True:
			my_edge = prev_edge.invert()
			if my_edge not in edges2cells: break
			this_cell = edges2cells[my_edge]

			if this_cell in visited_a:
				# assert my_edge == this_cell.get_nodes_at(Cell.UP)
				break
			else:
				this_cell.fix_given_nodes_to_direction(my_edge, Cell.UP)
				assert this_cell.get_nodes_at(Cell.UP) == my_edge
				visited_a.add(this_cell)

			prev_edge = this_cell.get_nodes_at(Cell.DOWN)


	visited_b = set()
	for init_cell in cells.itervalues():
		if init_cell in visited_b: continue
		visited_b.add(init_cell)

		east_edge = init_cell.get_nodes_at(Cell.RIGHT)

		prev_edge = east_edge

		while True:
			my_edge = prev_edge.invert()
			if my_edge not in edges2cells: break
			this_cell = edges2cells[my_edge]

			if this_cell in visited_b:
				# assert my_edge == this_cell.get_nodes_at(Cell.LEFT)
				break
			else:
				this_cell.fix_given_nodes_to_direction(my_edge, Cell.LEFT)
				visited_b.add(this_cell)

			prev_edge = this_cell.get_nodes_at(Cell.RIGHT)


	# Write
	for c in cells.itervalues():
		etc_str = ' '.join(map(str, c.etc))
		nodes_str = ' '.join(map(str, c.nodes))
		out_file.write('%d %s %s\n' % (c.cell_id, etc_str, nodes_str))

	out_file.write('$EndElements\n')

	for line in in_file:
		out_file.write(line)



# # Build cells
# for cell_id, nodes in cell2nodes:
# 	this_cell = Cell(cell_id, nodes)

# 	for this_direction in (Cell.UP, Cell.DOWN, Cell.LEFT, Cell.RIGHT):
# 		a, b = this_cell.get_nodes_at(this_direction)
# 		this_nodes = Edge(a,b)
# 		this_nodes_rev = Edge(b, a)

# 		# Add edge for me
# 		edges2cells[this_nodes].append((this_cell, this_direction))

# 		# # Accept an offer
# 		# if this_nodes_rev in edges2cells:
# 		# 	that_cell, that_direction = edges2cells[this_nodes_rev]

# 		# 	assert that_cell.get_nodes_at(that_direction) == this_nodes_rev
# 		# 	assert this_cell.cell_id != that_cell.cell_id
# 		# 	if that_cell.get_cell_at(that_direction):
# 		# 		raise ValueError('For cell %s, rev edge %s seen again: %s.%s already equals %s' % (this_cell, this_nodes_rev, that_cell, that_direction, that_cell.get_cell_at(that_direction)))

# 		# 	that_cell.set_cell_at(that_direction, this_cell)
# 		# 	this_cell.set_cell_at(Cell.opposite_of(that_direction), that_cell)

# 	cells[cell_id] = this_cell

# for cell_id, this_cell in cells.iteritems():
# 	for this_direction in (Cell.UP, Cell.DOWN, Cell.LEFT, Cell.RIGHT):
# 		a, b = this_cell.get_nodes_at(this_direction)
# 		this_nodes = Edge(a,b)
# 		this_nodes_rev = Edge(b, a)

# 		cells_with_edge = edges2cells[this_nodes]
# 		if len(cells_with_edge) == 2:
# 			(c1, _), (c2, _) = cells_with_edge
# 			that_cell = c1 if c2 == this_cell else c2
# 			cell2cells[this_cell].add(that_cell)
# 		else:
# 			assert len(cells_with_edge) == 1




# fixed_cells = set()

# for start_cell in cells.itervalues():
# 	current_cell = start_cell
# 	# fixed_cells.add(current_cell)

# 	while True:
# 		current_edge = current_cell.get_nodes_at(Cell.UP)

# 		that_cell = that_direction = None
# 		for neighbour in cell2cells[current_cell]:
# 			direction = neighbour.find_edge_direction(current_edge)
# 			if direction:
# 				that_cell = neighbour
# 				that_direction = direction

# 		if not that_cell: break


# 		# Check orientation
# 		if that_cell in fixed_cells:
# 			print current_edge, that_cell.get_nodes_at(Cell.DOWN)
# 			assert current_edge == that_cell.get_nodes_at(Cell.DOWN)

# 		# Fix orientation
# 		else:
# 			n1, n2 = current_edge
# 			print that_cell.up
# 			assert that_cell.down is None
# 			assert current_cell.up is None
# 			that_cell.fix_given_nodes_to_direction(n2, n1, Cell.DOWN)
# 			assert that_cell.get_nodes_at(Cell.DOWN) == current_edge
# 			that_cell.down = current_cell
# 			current_cell.up = that_cell
# 			fixed_cells.add(that_cell)


# 		# Go up
# 		current_cell = that_cell
