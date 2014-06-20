from quad_mesh import DetectQuadStructure
from collections import defaultdict
from random import choice, seed
from sys import stderr, stdout, argv

import cPickle


airfoil_mesh = 'new_grid.dat'


def load_mesh_from_dat(filename):
	# node -> [node]
	node_map = defaultdict(list)

	with open(filename, 'r') as f:
		# Read header
		line = map(int, f.readline().split())
		num_nodes, num_cells, num_internal_edges, num_border_edges = line

		# Read node data (coordinates)
		node_data = []
		for _ in xrange(num_nodes):
			x, y = map(float, f.readline().split())
			node_data.append((x,y))

		# Read cell data (cell to node)
		cell_to_nodes = []
		for _ in xrange(num_cells):
			n1, n2, n3, n4 = map(int, f.readline().split())
			cell_to_nodes.append((n1, n2, n3, n4))

		# Read internal and external edges
		edge_to_nodes = []
		for _ in xrange(num_internal_edges + num_border_edges):
			n1, n2, c1, c2 = map(int, f.readline().split())
			edge_to_nodes.append((n1, n2, n3, n4))

			# Add node-to-node info
			node_map[n1].append(n2)
			node_map[n2].append(n1)


	# Convert mapping of node_map - node -> {node}
	for key in node_map:
		node_map[key] = frozenset(node_map[key])

	return node_map


def log(message):
	print >> stderr, '>>', message

def load_serialized(filename):
	return cPickle.load(open(filename, 'rb'))


def serialize_to_file(obj, filename):
	cPickle.dump(obj, open(filename, 'wb'))

	loaded = load_serialized(filename)
	assert loaded == obj

def write_mesh(all_node_map, structured_regions, outfile=stdout):
	num_structured_regions = len(structured_regions)

	# Output header: number of structured regions
	print >> outfile, num_structured_regions

	for region in structured_regions:
		# Get number of rows and columns
		num_rows = len(region)
		assert num_rows >= 1, 'Need at least one row!'
		num_cols = len(region[0])

		# Output structured region header: number of rows and columns
		print >> outfile, num_rows, num_cols

		# Output structured region data
		for row in region:
			assert len(row) == num_cols, 'Non-uniform row size'
			print >> outfile, ' '.join(str(n) for n in row),
		print >> outfile


	# Output all-nodes header: number of nodes
	print >> outfile, len(all_node_map)

	# Output all-nodes data
	for expected_node, (node, neighbours) in enumerate(sorted(all_node_map.iteritems())):
		assert node == expected_node
		print >> outfile, ' '.join(neighbours)


def main(args):

	airfoil_dat = 'new_grid.dat'
	airfoil_serialized = 'new_grid.p'

	command = 'load'

	# Build airfoil mesh
	if command == 'build':
		log('Building mesh')
		node_map = load_mesh_from_dat(airfoil_dat)

	# Build airfoil mesh and dump to file
	elif command == 'save':
		log('Building mesh')
		node_map = load_mesh_from_dat(airfoil_dat)
		log('Saving mesh')
		serialize_to_file(node_map, airfoil_serialized)
		exit(0)

	# Load airfoil mesh
	elif command == 'load':
		log('Loading mesh')
		node_map = load_serialized(airfoil_serialized)

	else:
		raise ValueError('Illegal operation')


	# Set random seed, and choose a start node
	seed(123467)
	start_node = choice(node_map.keys())
	log('Out of %d nodes, we chose %d' % (len(node_map), start_node))

	# Detect structure
	log('Detecting structure')
	structured_grid = DetectQuadStructure(node_map, start_node).output

	# List of all detected structures
	structured_regions = [structured_grid]

	# Write out data
	log('Writing out mesh')
	write_mesh(len(node_map), node_map.keys(), structured_regions)


if __name__ == '__main__':
	main(argv)
