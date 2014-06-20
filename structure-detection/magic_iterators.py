from itertools import chain, izip, tee
from subprocess import Popen, PIPE


def pairwise(iterable):
    "s -> (s0,s1), (s1,s2), (s2, s3), ..."
    a, b = tee(iterable)
    next(b, None)
    return izip(a, b)


def null_pairwise(iterable):
    "s -> (None,s0), (s0, s1), (s1, s2), ..., (sn, None)"
    return pairwise(chain([None], iterable, [None]))


# Return intersection of given iters
def intersection(*iters):
    return set(iters[0]).intersection(*iters) if iters else set()


# Return the indices of matching values
def indices_of(value, iterable):
    return [ i for (i, v) in enumerate(iterable) if v == value ]

# Check whether all values in lst are unique
def unique_values(lst):
    return len(set(lst)) == len(lst)


def renumber_keys(source, renumbering):
    return [ source[i] for i in renumbering ]


def renumber_value_lists(source, renumbering):
    return [ [ renumbering[n] for n in ns] for ns in source ]


class PairwiseAccessor(object):
    """Allows indexing and slicing operations"""
    def __init__(self, lst):
        self.lst = lst

    def __iter__(self):
        return pairwise(self.lst)

    def __len__(self):
        return max(0, len(self.lst) - 1)

    def __getitem__(self, rng):
        if isinstance(rng, slice):
            iterator = xrange(*rng.indices(len(self)))
            # Non-negative step
            if slice.step >= 0:
                return ( (self.lst[i], self.lst[i+1]) for i in iterator )
            # Negative step
            else:
                return ( (self.lst[i-1], self.lst[i]) for i in iterator )

        elif isinstance(rng, int):
            if rng >= 0:
                return (self.lst[rng], self.lst[rng+1])
            else:
                return (self.lst[rng-1], self.lst[rng])

        else:
            raise TypeError("Illegal index type %s", type(rng).__name__)


def read_mesh_from_file(filename, field, data_type, collection):
    output = []
    p = Popen(['bin/mesh-printer', field, filename], stdout=PIPE, bufsize=1)
    for line in iter(p.stdout.readline, b''):
        elements_in_line = collection(map(data_type, line.split()))
        output.append(elements_in_line)

    assert p.wait() == 0, 'mesh-printer exited with non-zero status'

    return output
