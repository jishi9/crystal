#!/usr/bin/python

from random import shuffle, uniform

grid = [
    [None, 0 , None , 1 , 2 , 3 , 4 , 5 , 6 ],
    [None, 7 , None , 8 , 9 , 10, 11, 12, 13],
    [None, 22, 14   , 15, 16, 17, 18, 19, 20],
    [21  , 23, 24   , 25, 26, 27, 28, 29, 30],
    [None, 31, 32   , 33, 34, 35, 36, 37, 38]
]

rows = len(grid)
cols = len(grid[0])


for r in xrange(rows):
    for c in xrange(cols):
        if grid[r][c] is not None:
            grid[r][c] = (grid[r][c], uniform(r, r+1) - rows/2, uniform(c, c+1) - cols/2)


elements = sorted(elem  for rr in grid for elem in rr if elem is not None)


# Shuffle data
num_nodes = 39
reordering = range(num_nodes)
shuffle(reordering)



# Cells - clockwise, starting top-left

cells = [
[0, 1, 8,  7],     #0
[1, 2, 9,  8],     #1
[2, 3, 10, 9],     #2
[3, 4, 11, 10],    #3
[4, 5, 12, 11],    #4
[5, 6, 13, 12],    #5
[7, 8, 15, 14],    #6
[8, 9, 16, 15],    #7
[9, 10, 17, 16],   #8
[10, 11, 18, 17],  #9
[11, 12, 19, 18],  #10
[12, 13, 20, 19],  #11
[7, 14, 24, 22],   #12
[14, 15, 25, 24],  #13
[15, 16, 26, 25],  #14
[16, 17, 27, 26],  #15
[17, 18, 28, 27],  #16
[18, 19, 29, 28],  #17
[19, 20, 30, 29],  #18
[21, 22, 24, 23],  #19
[23, 24, 32, 31],  #20
[24, 25, 33, 32],  #21
[25, 26, 34, 33],  #22
[26, 27, 35, 34],  #23
[27, 28, 36, 35],  #24
[28, 29, 37, 36],  #25
[29, 30, 38, 37]   #26
]

# Edges - internal
in_edges = [
[1,  8,  0,  1],
[2,  9,  1,  2],
[3,  10, 2,  3],
[4,  11, 3,  4],
[5,  12, 4,  5],
[7,  14, 6,  12],
[8,  15, 6,  7],
[9,  16, 7,  8],
[10, 17, 8,  9],
[11, 18, 9,  10],
[12, 19, 10, 11],
[14, 24, 12, 13],
[15, 25, 13, 14],
[16, 26, 14, 15],
[17, 27, 15, 16],
[18, 28, 16, 17],
[19, 29, 17, 18],
[23, 24, 19, 20],
[24, 32, 20, 21],
[25, 33, 21, 22],
[26, 34, 22, 23],
[27, 35, 23, 24],
[28, 36, 24, 25],
[29, 37, 25, 26],
[7,  8,  0,  6],
[8,  9,  1,  7],
[9,  10, 2,  8],
[10, 11, 3,  9],
[11, 12, 4,  10],
[12, 13, 5,  11],
[14, 15, 6,  13],
[15, 16, 7,  14],
[16, 17, 8,  15],
[17, 18, 9,  16],
[18, 19, 10, 17],
[19, 20, 11, 18],
[22, 24, 12, 19],
[24, 25, 13, 21],
[25, 26, 14, 22],
[26, 27, 15, 23],
[27, 28, 16, 24],
[28, 29, 17, 25],
[29, 30, 18, 26]
]


# Border edges
border_edges = [
[0,  1,  0,  0],
[1,  2,  1,  1],
[2,  3,  2,  2],
[3,  4,  3,  3],
[4,  5,  4,  4],
[5,  6,  5,  5],
[6,  13, 5,  5],
[13, 20, 11, 11],
[20, 30, 18, 18],
[30, 38, 26, 26],
[0,  7,  0,  0],
[7,  22, 12, 12],
[22, 21, 19, 19],
[21, 23, 19, 19],
[23, 31, 20, 20],
[31, 32, 20, 20],
[32, 33, 21, 21],
[33, 34, 22, 22],
[34, 35, 23, 23],
[35, 36, 24, 24],
[36, 37, 25, 25],
[37, 38, 26, 26]
]



# print headers
print ' %d %d %d %d ' % (num_nodes, len(cells), len(in_edges), len(border_edges))

# Print coordinates
elements = [ (elements[i][1], elements[i][2]) for i in reordering ]
for x, y in elements:
    print ' %f %f ' %(x, y)

old2new = dict()
for new, old in enumerate(reordering):
    old2new[old] = new

# Print cells
for cell, nodes in enumerate(cells):
    n1, n2, n3, n4 = [old2new[n] for n in nodes]
    print ' %d %d %d %d ' % ( n1, n2, n3, n4)

# print edges
for edge, dat in enumerate(in_edges + border_edges):
    n1, n2, c1, c2 = dat
    print ' %d %d %d %d ' % (old2new[n1], old2new[n2], c1, c2)
