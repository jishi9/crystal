#include <cfloat>
#include <iostream>

#include "mesh.h"
#include "utils.h"

static inline void printExactly(const PNeighbours& neighbours, const int num_neighbours) {
  int n1 = num_neighbours >= 1 ? neighbours.element_id(0) : NO_NEIGHBOUR;
  int n2 = num_neighbours >= 2 ? neighbours.element_id(1) : NO_NEIGHBOUR;
  int n3 = num_neighbours >= 3 ? neighbours.element_id(2) : NO_NEIGHBOUR;
  int n4 = num_neighbours >= 4 ? neighbours.element_id(3) : NO_NEIGHBOUR;
  printf("%d %d %d %d\n", n1, n2, n3, n4);
}

static inline void printMinimal(const PNeighbours& neighbours, const int num_neighbours) {
  for (int i : IterAtoB(0, num_neighbours)) {
    printf("%d ", neighbours.element_id(i));
  }
  printf("\n");
}

static inline int getNumber(MeshInputStream* in, const char* section_name) {
  in->readSection(section_name);
  return in->readNonNegInt();
}


static void printQuadMeshHeader(const char* filename) {
  MeshInputStream in(filename);
  printf("%d %d %d %d\n",
    getNumber(&in, "num_nodes"),
    getNumber(&in, "num_cells"),
    getNumber(&in, "num_inedges"),
    getNumber(&in, "num_borderedges"));
}

static void printQuadMeshMap(const char* filename,
  const char* num_elems_section_name, const char* data_section_name) {

  MeshInputStream in(filename);

  // Read number of nodes
  const int num_elems = getNumber(&in, num_elems_section_name);

  // Read node_to_node
  in.readSection(data_section_name);
  PNeighbours neighbours;
  for (int i = 0 ; i < num_elems ; ++i) {
    neighbours.clear_element_id();
    in.readProtoBuf(&neighbours);
    const int num_neighbours = neighbours.element_id_size();

    // Print
    printMinimal(neighbours, num_neighbours);
  }
}

static void printQuadMeshCoordData(const char* filename, const char* section_name) {
  MeshInputStream in(filename);

  // Read number of nodes
  const int num_elems = getNumber(&in, "num_nodes");

  // Read node_to_node
  in.readSection(section_name);
  PCoordinate coordinates;
  for (int i = 0 ; i < num_elems ; ++i) {
    coordinates.Clear();
    in.readProtoBuf(&coordinates);

    printf("%.*g %.*g\n", DBL_DIG, coordinates.x(), DBL_DIG, coordinates.y());
  }
}



static void printQuadMeshStructureMetadata(const char* filename, const bool nodes_only) {
  printf("%s\n", filename);

  MeshInputStream in(filename);

  // Read number of cells and number of nodes
  const int num_cells = getNumber(&in, "num_cells");
  const int num_nodes = getNumber(&in, "num_nodes");

  printf("%d cells\n%d nodes\n", num_cells, num_nodes);

  // Number of node structured regions
  in.readSection("structured_node_regions");
  const int num_structured_node_regions = in.readNonNegInt();
  printf("%d structured node regions\n", num_structured_node_regions);

  int num_structured_nodes = 0;
  // Read each node structured region's metadata
  for (int i = 1; i <= num_structured_node_regions; ++i) {
    in.readString("region");
    PStructuredNodeRegion region;
    in.readProtoBuf(&region);
    const int num_rows = region.num_rows();
    const int num_cols = region.num_cols();
    const int num_nodes_in_region = num_rows * num_cols;
    num_structured_nodes += num_nodes_in_region;

    // Skip data
    in.readString("data");
    for (int j=0 ; j < num_nodes_in_region ; ++j) {
      in.readNonNegInt();
    }

    printf("  Region %d: %d rows x %d cols = %d nodes\n", i, num_rows, num_cols,
      num_nodes_in_region);
  }

  printf("Total structured nodes: %d\n", num_structured_nodes);
  printf("Total unstructured nodes: %d\n", num_nodes - num_structured_nodes);

  if (nodes_only) return;

  // Number of cell structured regions
  in.readSection("structured_cell_regions");
  const int num_structured_cell_regions = in.readNonNegInt();
  printf("%d structured cell regions\n", num_structured_cell_regions);

  // Read each cell structured region's metadata
  for (int i = 1; i <= num_structured_cell_regions; ++i) {
    in.readString("region");
    PStructuredCellRegion region;
    in.readProtoBuf(&region);
    const int cell2node_offset = region.cell2node_offset();
    const int node_row_start = region.node_row_start();
    const int node_row_finish = region.node_row_finish();
    const int node_col_start = region.node_col_start();
    const int node_col_finish = region.node_col_finish();

    const auto& compass = region.compass();
    const int compass_len = compass.size();


    // Print
    const int num_rows = node_row_finish - node_row_start;
    const int num_cols = node_col_finish - node_col_start;
    const int num_cells_in_region = num_rows * num_cols;
    printf("  Region %d: %d rows x %d rows = %d nodes\n",
      i, num_rows, num_cols, num_cells_in_region);
    printf("    Node row %d to %d -- Node col %d to %d\n",
      node_row_start, node_row_finish, node_col_start, node_col_finish);
    printf("    cell2node offset = %d\n", cell2node_offset);
    printf("    Compass (Unstructured -> Structured):\n");
    for (int i = 0; i < compass_len; ++i) {
      printf("      %d -> %d\n", i, compass.Get(i));
    }
  }

  in.readSection("unstructured_cell_regions");
  PUnstructuredCellRegion region;
  in.readProtoBuf(&region);

  // Print
  printf("%d unstructured cells\nunstructured cells offset = %d\n",
    region.num_unstructured_cells(), region.unstructured_cells_offset());



  // EDGE
  {
    // Number of edge structured regions
    in.readSection("structured_edge_regions");


    in.readString("structured_h_edge_regions");
    const int num_structured_h_edge_regions = in.readNonNegInt();
    printf("%d structured h_edge regions\n", num_structured_h_edge_regions);

    // Read each edge structured region's metadata
    for (int i = 1; i <= num_structured_h_edge_regions; ++i) {
      in.readString("region");
      PStructuredEdgeRegion region;
      in.readProtoBuf(&region);
      const int edge2node_offset = region.inedge2node_offset();
      const int node_row_start = region.node_row_start();
      const int node_row_finish = region.node_row_finish();
      const int node_col_start = region.node_col_start();
      const int node_col_finish = region.node_col_finish();

      const auto& node_compass = region.node_compass();
      const int node_compass_len = node_compass.size();
      const auto& cell_compass = region.cell_compass();
      const int cell_compass_len = cell_compass.size();


      // Print
      const int num_rows = node_row_finish - node_row_start;
      const int num_cols = node_col_finish - node_col_start;
      const int num_edges_in_region = num_rows * num_cols;
      printf("  Region %d: %d rows x %d rows = %d nodes\n",
        i, num_rows, num_cols, num_edges_in_region);
      printf("    Node row %d to %d -- Node col %d to %d\n",
        node_row_start, node_row_finish, node_col_start, node_col_finish);
      printf("    edge2node offset = %d\n", edge2node_offset);
      printf("    Node compass (Unstructured -> Structured):\n");
      for (int i = 0; i < node_compass_len; ++i) {
        printf("      %d -> %d\n", i, node_compass.Get(i));
      }
      printf("    Cell compass (Unstructured -> Structured):\n");
      for (int i = 0; i < cell_compass_len; ++i) {
        printf("      %d -> %d\n", i, cell_compass.Get(i));
      }

    }



    in.readString("structured_v_edge_regions");
    const int num_structured_v_edge_regions = in.readNonNegInt();
    printf("%d structured v_edge regions\n", num_structured_v_edge_regions);

    // Read each edge structured region's metadata
    for (int i = 1; i <= num_structured_v_edge_regions; ++i) {
      in.readString("region");
      PStructuredEdgeRegion region;
      in.readProtoBuf(&region);
      const int edge2node_offset = region.inedge2node_offset();
      const int node_row_start = region.node_row_start();
      const int node_row_finish = region.node_row_finish();
      const int node_col_start = region.node_col_start();
      const int node_col_finish = region.node_col_finish();

      const auto& node_compass = region.node_compass();
      const int node_compass_len = node_compass.size();
      const auto& cell_compass = region.cell_compass();
      const int cell_compass_len = cell_compass.size();

      // Print
      const int num_rows = node_row_finish - node_row_start;
      const int num_cols = node_col_finish - node_col_start;
      const int num_edges_in_region = num_rows * num_cols;
      printf("  Region %d: %d rows x %d rows = %d nodes\n",
        i, num_rows, num_cols, num_edges_in_region);
      printf("    Node row %d to %d -- Node col %d to %d\n",
        node_row_start, node_row_finish, node_col_start, node_col_finish);
      printf("    edge2node offset = %d\n", edge2node_offset);
      printf("    Node compass (Unstructured -> Structured):\n");
      for (int i = 0; i < node_compass_len; ++i) {
        printf("      %d -> %d\n", i, node_compass.Get(i));
      }
      printf("    Cell compass (Unstructured -> Structured):\n");
      for (int i = 0; i < cell_compass_len; ++i) {
        printf("      %d -> %d\n", i, cell_compass.Get(i));
      }
    }
  }
}


void usage() {
  cerr << "USAGE: ./mesh-printer [structure|nodestructure|header|<map_name>] [filename]" << endl << endl;
}

int main(int argc, char const *argv[]) {
  if (argc != 3) {
    usage();
    return 1;
  }

  const char* command = argv[1];
  const char* filename = argv[2];

  if (strcmp(command, "node_to_node") == 0) {
    printQuadMeshMap(filename, "num_nodes", "node_to_node");
  }
  else if (strcmp(command, "cell_to_ord_nodes") == 0) {
    printQuadMeshMap(filename, "num_cells", "cell_to_ord_nodes");
  }
  else if (strcmp(command, "inedge_to_nodes") == 0) {
    printQuadMeshMap(filename, "num_inedges", "inedge_to_nodes");
  }
  else if (strcmp(command, "inedge_to_cells") == 0) {
    printQuadMeshMap(filename, "num_inedges", "inedge_to_cells");
  }
  else if (strcmp(command, "borderedge_to_nodes") == 0) {
    printQuadMeshMap(filename, "num_borderedges", "borderedge_to_nodes");
  }
  else if (strcmp(command, "borderedge_to_cell") == 0) {
    printQuadMeshMap(filename, "num_borderedges", "borderedge_to_cell");
  }
  else if (strcmp(command, "coord_data") == 0) {
    printQuadMeshCoordData(filename, "coord_data");
  }
  else if (strcmp(command, "new_coord_data") == 0) {
    printQuadMeshCoordData(filename, "new_coord_data");
  }
  else if (strcmp(command, "new_cell_to_ord_nodes") == 0) {
    printQuadMeshMap(filename, "num_cells", "new_cell_to_ord_nodes");
  }
  else if (strcmp(command, "new_inedge_to_nodes") == 0) {
    printQuadMeshMap(filename, "num_inedges", "new_inedge_to_nodes");
  }
  else if (strcmp(command, "new_inedge_to_cells") == 0) {
    printQuadMeshMap(filename, "num_inedges", "new_inedge_to_cells");
  }
  else if (strcmp(command, "new_borderedge_to_nodes") == 0) {
    printQuadMeshMap(filename, "num_borderedges", "new_borderedge_to_nodes");
  }
  else if (strcmp(command, "new_borderedge_to_cell") == 0) {
    printQuadMeshMap(filename, "num_borderedges", "new_borderedge_to_cell");
  }
  else if (strcmp(command, "borderedge_bounds") == 0) {
    printQuadMeshMap(filename, "num_borderedges", "borderedge_bounds");
  }


  else if (strcmp(command, "header") == 0) {
    printQuadMeshHeader(filename);
  }
  else if (strcmp(command, "structure") == 0) {
    printQuadMeshStructureMetadata(filename, false);
  }
  else if (strcmp(command, "nodestructure") == 0) {
    printQuadMeshStructureMetadata(filename, true);
  }

  else {
    usage();
    return 1;
  }
}
