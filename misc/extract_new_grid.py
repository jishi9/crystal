#!/usr/bin/python

from sys import argv
from signal import signal, SIGPIPE, SIG_DFL
signal(SIGPIPE,SIG_DFL) 

FILENAME = 'dat/airfoil.dat'

def getNumber(argv, f):
    if len(argv) != 3:
      print 'No number'
      exit(1)

    number = int(argv[2])
    if number < 0:
      print 'Negative number'
      exit(1)

    return number
 

def main(args):
  if len(argv) not in (2, 3):
    print 'USAGE:', argv[0], '[node|cell] number OR', argv[0], 'label'
    exit(1)

  with open(FILENAME, 'r') as f:
    # read header
    num_nodes, num_cells, num_internal_edges, num_border_edges = map(int, f.readline().split())

    if argv[1] == 'node':
      number = getNumber(argv, f)
      if number >= num_nodes:
        print 'Too large - only', num_nodes, 'nodes'
        exit(1)

      # Skip lines
      for _ in xrange(number): next(f)
      print next(f).rstrip()

    elif argv[1] == 'cell':
      number = getNumber(argv, f)
      if number >= num_cells:
        print 'Too large - only', num_cells, 'cells'
        exit(1)

      # Skip nodes and cells
      for _ in xrange(num_nodes + number): next(f)
      print next(f).rstrip()

    elif argv[1] == 'label':
      for i in xrange(num_nodes):
        print 'node', i, ':', next(f).rstrip()

      for i in xrange(num_cells):
        print 'cell', i, ':', next(f).rstrip()

      for i in xrange(num_internal_edges):
        print 'internal edge', i, ':', next(f).rstrip()

      for i in xrange(num_border_edges):
        print 'border edge', i, ':', next(f).rstrip()

    else:
      print 'Unknown command', argv[1]
      exit(1)

if __name__ == '__main__':
  main(argv)
