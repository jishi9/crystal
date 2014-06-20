from collections import defaultdict
from itertools import islice
filename = '/home/wael/Documents/PyOP2/detect-structure/meshes/naca0012.p'

# Get grid and number of points
grid = self.GetOutput()
points = grid.GetPointData()
num_points = grid.GetNumberOfPoints()

# Create array
nums = paraview.vtk.vtkIntArray()
nums.SetNumberOfValues(num_points)
nums.SetName('colors')

# Color
d = defaultdict(lambda: 0)
with open('/tmp/region', 'r') as regions , open('/tmp/oldnew', 'r') as nodes:
	iter_regions = ( map(int, line.split()) for line in regions)
	for color, size in iter_regions:
		for line in islice(nodes, size):
			old, new = map(int, line.split())
			d[old] = color

# Apply values
for i in xrange(num_points):
  nums.SetValue(i, d[i])

# Add array
points.AddArray(nums)
del nums
