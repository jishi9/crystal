#include <cassert>
#include <ctime>
#include <vector>

#include "data-relabeler.h"
#include "mesh.h"
#include "mesh-builder.h"

using std::cerr;
using std::vector;

Mesh buildMesh() {
  return build35Mesh();
}


// Original operation over unstructured mesh
int run_original(const Mesh& mesh) {
  const clock_t start = clock();
  // Data array: cell_index -> cell data
  const vector<Data>& element_data = mesh.element_data;

  // Adjacency list: cell_index -> cell neighbours
  const vector<QuadNeighbours>& element_neighbours = mesh.element_neighbours;

  // Unstructured mesh loop
  for (int cellIdx = 0; cellIdx < element_data.size(); ++cellIdx) {
    // Get cell data
    const Data& this_dat = element_data[cellIdx];

    // Print neighbours
    const QuadNeighbours& neighbours = element_neighbours[cellIdx];
    printf("%d:", this_dat.data);
    for (int neighbour_idx : neighbours) {
      if (neighbour_idx != NO_NEIGHBOUR) {
        printf(" %d", element_data[neighbour_idx].data);
      }
    }
    printf("\n");
  }

  cerr << clock() - start << " ticks" << endl;
  return 0;
}


int run_structurer(const Mesh& mesh) {
  const int num_elems = mesh.num_elems;

  // Data array: cell_index -> cell data
  const vector<Data>& element_data = mesh.element_data;

  // Adjacency list: cell_index -> cell neighbours
  // neighbours[0,1,2,3] = N, E, S, W
  const vector<QuadNeighbours>& element_neighbours = mesh.element_neighbours;

  DataRelabeler relabeler;

  // Add structured regions
  for (const Structured2DGrid& grid : mesh.structured_regions) {
    relabeler.addStructuredGrid(grid.indices.data(), grid.num_rows, grid.num_cols);
  }
  // Add unstructured regions: unstructured_indices = {0, 1, 2, ..., num_elems-1}
  vector<int> unstructured_indices(num_elems);
  std::iota(unstructured_indices.begin(), unstructured_indices.end(), 0);
  relabeler.addUnstructuredData(unstructured_indices.data(), num_elems);

  // Translate indices in maps
  vector<QuadNeighbours> new_element_neighbours = relabeler.translateIndices(element_neighbours);
  vector<Data> new_element_data = relabeler.reorderByCellIndices(element_data);

  const clock_t start = clock();

  // Apply kernel to each structured region
  for (DataRelabeler::StructuredRegion region : relabeler.getStructuredRegions()) {
    // Lambda to address by row/col
    auto conv2d = [=] (int row, int col) { return row*region.num_cols + col; };

    Data* grid = &new_element_data[region.offset];

    // Loop from second row to penultimate row
    for (int row = 1; row < region.num_rows-1; ++row) {
      // Loop from second element to penultimate element
      for (int col = 1; col < region.num_cols-1; ++col) {
        const Data& this_dat = grid[conv2d(row, col)];

        // COMPUTATION KERNEL GOES HERE
        const Data& north = grid[conv2d(row-1, col)];
        const Data& east = grid[conv2d(row, col+1)];
        const Data& south = grid[conv2d(row+1, col)];
        const Data& west = grid[conv2d(row, col-1)];
        printf("%d: %d %d %d %d\n", this_dat.data, north.data, east.data, south.data, west.data);
      }
    }
  }

  cerr << "Structured done: " << clock() - start << " ticks" << endl;

  // Apply kernel to unstructured region
  for (int cellIdx = 0; cellIdx < new_element_data.size(); ++cellIdx) {
    if (!relabeler.isUnstructured(cellIdx)) continue;

    // Get cell data
    const Data& this_dat = new_element_data[cellIdx];

    /* KERNEL */
    // Print neighbours
    const QuadNeighbours& neighbours = new_element_neighbours[cellIdx];
    printf("%d:", this_dat.data);
    for (int neighbour_idx : neighbours) {
      if (neighbour_idx != NO_NEIGHBOUR) {
        printf(" %d", new_element_data[neighbour_idx].data);
      }
    }
    printf("\n");
  }

  cerr << clock() - start << " ticks" << endl;

  // ALTERNATIVELY, CAN SIMPLY DERIVE HALO INDICES AND LOOP OVER THEM, AND THEN
  // LOOP OVER THE REST OF UNSTRUCTURED

  return 0;
}



// Compute area of unstructured mesh
int compute_area_unstructured(const CellMesh& mesh) {
  const clock_t start = clock();
  // Node index -> coordinates
  const vector<Coordinate>& node_data = mesh.node_data;

  // Cell index -> [node index]
  const vector<QuadNeighbours>& element_to_node = mesh.cell_to_nodes;

  double total_area = 0;

  // Unstructured mesh loop
  for (int cellIdx = 0; cellIdx < element_to_node.size(); ++cellIdx) {
    // Get cell nodes
    const QuadNeighbours& nodes = element_to_node[cellIdx];
    const Coordinate& n1 = node_data[nodes[0]];
    const Coordinate& n2 = node_data[nodes[1]];
    const Coordinate& n3 = node_data[nodes[2]];
    const Coordinate& n4 = node_data[nodes[3]];

    // Compute area
    double area = (n1.x*n2.y - n1.y*n2.x)
      + (n2.x*n3.y - n2.y*n3.x)
      + (n3.x*n4.y - n3.y*n4.x)
      + (n4.x*n1.y - n4.y*n1.x);

    area = fabs(area / 2);

    // Update total area
    total_area += area;
  }

  cerr << "sum = " << total_area << endl;

  cerr << clock() - start << " ticks" << endl;
  return 0;
}



int main(int argc, char const *argv[]) {
  if (argc != 2) {
    cerr << "One argument!" << endl;
    return 1;
  }

  switch (argv[1][0]) {
    case '1':
      return run_original(buildMesh());
    case '2':
      return run_structurer(buildMesh());
    case '3':
      return compute_area_unstructured(loadQuadMesh());
    default:
      cerr << "Argument must be either 1 or 2 or 3" << endl;
      return 1;
  }
}
