from mesh_writer import MeshWriter
from protogen.mesh_pb2 import PStructuredCellRegion, PUnstructuredCellRegion, PStructuredNodeRegion, PStructuredEdgeRegion, PNeighbours, PCoordinate


def write_map(mesh_writer, section_name, elem_to_neighbours):
    mesh_writer.WriteSection(section_name)

    neighbours_proto = PNeighbours()
    for neighbours in elem_to_neighbours:
        neighbours_proto.Clear()
        neighbours_proto.element_id.extend(neighbours)
        mesh_writer.WriteProto(neighbours_proto)


def write_structured_node_info(node_structure, filename):
    num_structured_regions = len(node_structure.structured_node_regions)

    with MeshWriter(filename) as e:
        e.WriteSection('structured_node_regions')
        e.WriteNonNegInt(num_structured_regions)

        for region_id, region in enumerate(node_structure.structured_node_regions):
            # Get number of rows and columns
            num_rows = len(region)
            assert num_rows >= 1, 'Need at least one row!'
            num_cols = len(region[0])

            # Output structured region header: number of rows and columns
            e.WriteString('region')
            region_proto = PStructuredNodeRegion()
            region_proto.region_number = region_id
            region_proto.num_rows = num_rows
            region_proto.num_cols = num_cols
            e.WriteProto(region_proto)

            # Output structured region data
            e.WriteString('data')
            for row in region:
                assert len(row) == num_cols, 'Non-uniform row size'
                for n in row:
                    e.WriteNonNegInt(n)



def write_structured_cell_info(cell_structure, filename):
    num_structured_regions = len(cell_structure.structured_cell_regions)

    with MeshWriter(filename) as e:
        e.WriteSection('structured_cell_regions')
        e.WriteNonNegInt(num_structured_regions)

        for region_meta in cell_structure.structured_cell_regions:
            # Output structured region header: number of rows and columns
            e.WriteString('region')
            region_proto = PStructuredCellRegion()
            region_proto.cell2node_offset = region_meta.cells_offset
            region_proto.node_row_start = region_meta.row_start
            region_proto.node_row_finish = region_meta.row_finish
            region_proto.node_col_start = region_meta.col_start
            region_proto.node_col_finish = region_meta.col_finish
            assert len(region_meta.compass) == 4
            for c in region_meta.compass:
                region_proto.compass.append(c)
            e.WriteProto(region_proto)

        # Output unstructured region headers
        e.WriteSection('unstructured_cell_regions')
        region_proto = PUnstructuredCellRegion()
        region_proto.num_unstructured_cells = cell_structure.unstructured_cell_regions.num_unstructured_cells
        region_proto.unstructured_cells_offset = cell_structure.unstructured_cell_regions.unstructured_cells_offset
        e.WriteProto(region_proto)


        # Output structured region data
        write_map(e, 'new_cell_to_ord_nodes', cell_structure.new_cell2ordnodes)


def write_structured_inedge_info(edge_structure, filename):
    num_structured_h_regions = len(edge_structure.h_edge_structured_regions)
    num_structured_v_regions = len(edge_structure.v_edge_structured_regions)

    with MeshWriter(filename) as e:

        ## Function to write out structure region metadata
        def write_edge_regions(regions):
            for edge_region in regions:
                e.WriteString('region')

                region_proto = PStructuredEdgeRegion()
                region_proto.inedge2node_offset = edge_region.inedge2node_offset
                region_proto.node_row_start = edge_region.node_row_start
                region_proto.node_row_finish = edge_region.node_row_finish
                region_proto.node_col_start = edge_region.node_col_start
                region_proto.node_col_finish = edge_region.node_col_finish

                # Node compass
                for c in edge_region.node_compass:
                    region_proto.node_compass.append(c)

                # Cell compass
                for c in edge_region.cell_compass:
                    region_proto.cell_compass.append(c)

                e.WriteProto(region_proto)

        e.WriteSection('structured_edge_regions')

        e.WriteString('structured_h_edge_regions')
        e.WriteNonNegInt(num_structured_h_regions)
        write_edge_regions(edge_structure.h_edge_structured_regions)

        e.WriteString('structured_v_edge_regions')
        e.WriteNonNegInt(num_structured_v_regions)
        write_edge_regions(edge_structure.v_edge_structured_regions)

        e.WriteString('unstructured_edges_offset')
        e.WriteNonNegInt(edge_structure.unstructured_edges_offset)

        ## Write out new mappings
        write_map(e, 'new_inedge_to_nodes', edge_structure.new_inedge2ordnodes)
        write_map(e, 'new_inedge_to_cells', edge_structure.new_inedge2ordcells)


def write_renumbered_border_edges(new_borderedge2nodes, new_borderedge2cell,  filename):
    with MeshWriter(filename) as e:
        write_map(e, 'new_borderedge_to_nodes', new_borderedge2nodes)
        write_map(e, 'new_borderedge_to_cell', new_borderedge2cell)


def write_new_coord_data(new_coord_data, filename):
    with MeshWriter(filename) as e:
        e.WriteSection('new_coord_data')
        coordinate_proto = PCoordinate()
        for x, y in new_coord_data:
            coordinate_proto.Clear()
            coordinate_proto.x = x
            coordinate_proto.y = y
            e.WriteProto(coordinate_proto)


def write_text_map(section_name, a_to_b, filename):
    with MeshWriter(filename) as e:
        e.WriteSection(section_name)

        iterable = enumerate(a_to_b) if isinstance(a_to_b, list) else a_to_b.iteritems()

        for a, b in iterable:
            e.WriteRaw('%d %d\n' % (a, b))
