from quad_mesh import DetectQuadStructure, StructureException
from random import sample

def _build_maps(structured_node_regions, num_nodes):

    iter_structured_nodes = (node
        for region in structured_node_regions
        for row in region
        for node in row)

    # Node renumbering
    oldnode2newnode = dict()
    newnode2oldnode = []

    # Renumber structured nodes
    for new_node_id, old_node_id in enumerate(iter_structured_nodes):
        assert old_node_id not in oldnode2newnode, 'Bad node mapping'
        oldnode2newnode[old_node_id] = new_node_id
        newnode2oldnode.append(old_node_id)


    num_structured_nodes = len(oldnode2newnode)
    current_new_node_id = num_structured_nodes

    # Renumber unstructured nodes
    for old_node_id in xrange(num_nodes):
        if old_node_id not in oldnode2newnode:
            oldnode2newnode[old_node_id] = current_new_node_id
            current_new_node_id += 1
            newnode2oldnode.append(old_node_id)

    assert current_new_node_id == num_nodes
    assert len(oldnode2newnode) == num_nodes
    assert len(newnode2oldnode) == num_nodes

    return oldnode2newnode, newnode2oldnode


class DetectNodeStructure(object):
    """docstring for DetectNodeStructure"""
    def __init__(self, node2node, start_node, max_rows=99999999, max_cols=99999999, max_regions=1, max_fail=3):
        num_nodes = len(node2node)

        # Detect structured region
        initial_sequence = [start_node] if start_node else []
        self.detect_multiple(node2node, max_rows, max_cols, max_regions, max_fail, initial_sequence)

        # Node renumbering map
        self.oldnode2newnode, self.newnode2oldnode = _build_maps(self.structured_node_regions, num_nodes)


    def detect_multiple(self, node2node, max_rows, max_cols, max_regions, max_fail, initial_sequence=[]):
        failed_nodes = set()

        self.structured_node_regions = []
        detector = DetectQuadStructure(node2node, None, max_rows, max_cols)
        # Yield initial sequence, then choose randomly thereafter
        def node_chooser():
            for node in initial_sequence:
                yield node
            while True:
                try:
                    really_unvisited = detector.not_visited - failed_nodes
                    num_left = len(really_unvisited)
                    if num_left % 1000 == 0:
                        print num_left, 'left'
                    yield sample(really_unvisited, 1)[0]
                except ValueError:
                    return
        node_seq = node_chooser()


        fails = 0
        while len(self.structured_node_regions) < max_regions:
            try:
                # Choose start node
                start_node = next(node_seq)
                region = detector.detect_region_from(start_node)
                self.structured_node_regions.append(region)
                fails = 0

            except StructureException:
                fails += 1
                failed_nodes.add(start_node)
                if fails > max_fail:
                    print 'Ending detection... with', len(self.structured_node_regions)
                    return
            except StopIteration:
                print 'Ran out of nodes'
                return

        print 'detected maximum', max_regions
