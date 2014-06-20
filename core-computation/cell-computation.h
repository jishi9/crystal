#ifndef CELL_COMPUTATION_H
#define CELL_COMPUTATION_H

#include "mesh-builder.h"
#include "mesh.h"
#include "utils.h"


// Cell kernel function type

// using KCellFunc = void (
//   const int cell_id,
//   const QuadNeighbours& cell_nodes,
//   const QuadCellMesh& mesh,
//   const tuple<>&, const tuple<double*>& outputs);


static inline void print_cell_kernel(double* total,
                  const Coordinate& a, const Coordinate& b,
                  const Coordinate& c, const Coordinate& d) {
  printf("%f %f - %f %f - %f %f - %f %f\n", a.x, a.y, b.x, b.y, c.x, c.y, d.x, d.y);
}

static inline void null_cell_kernel(double* total,
                  const Coordinate& a, const Coordinate& b,
                  const Coordinate& c, const Coordinate& d) {}


static inline void area_cell_kernel(
  const int cell_id,
  const QuadNeighbours& cell_nodes,
  const QuadCellMesh& mesh,
  const vector<Coordinate>& node_data,
  double* const& sum) {

  const Coordinate& a = node_data[cell_nodes[0]];
  const Coordinate& b = node_data[cell_nodes[1]];
  const Coordinate& c = node_data[cell_nodes[2]];
  const Coordinate& d = node_data[cell_nodes[3]];

  // Compute area
  double area =
    (a.x*b.y - a.y*b.x) + (b.x*c.y - b.y*c.x) +
    (c.x*d.y - c.y*d.x) + (d.x*a.y - d.y*a.x);

  area = fabs(area / 2);

  // Update total area
  *sum += area;
}


template<typename InArgs, typename OutArgs>
static inline void run_unstructured_cells(
  const QuadCellMesh& mesh,
  void cell_kernel(const int, const QuadNeighbours&, const QuadCellMesh&, const InArgs&, const OutArgs&),
  const InArgs& inputs, const OutArgs& outputs) {

  int cells_processed = 0;
  for (const QuadNeighbours& nodes : mesh.cell2ordnodes) {
    cell_kernel(cells_processed, nodes, mesh, inputs, outputs);
    cells_processed++;
  }
  check(cells_processed == mesh.num_cells, "Did not process the expected number of cells");
}


template<typename InArgs, typename OutArgs>
static inline void run_structured_cells(
  const QuadCellMesh& mesh,
  void cell_kernel(const int, const QuadNeighbours&, const QuadCellMesh&, const InArgs&, const OutArgs&),
  const InArgs& inputs, const OutArgs& outputs) {

  // Structured loop
  int region_index = 0;
  int nodes_offset = 0;
  int cells_processed = 0;
  for (const StructuredCellRegion& cell_region : mesh.cell_structured_regions) {
    const int cells_per_row = mesh.node_structured_regions[region_index].num_cols;
    auto conv2d = [cells_per_row] (int row, int col) {
      return row*cells_per_row + col;
    };
#ifdef IGNORE_COMPASS
    const int cell_region_four_compass_index = 3;
#else
    const int cell_region_four_compass_index = cell_region.four_compass_index;
#endif
    const int inv_compass_0 = FourCompass[cell_region.four_compass_index][0];
    const int inv_compass_1 = FourCompass[cell_region.four_compass_index][1];
    const int inv_compass_2 = FourCompass[cell_region.four_compass_index][2];
    const int inv_compass_3 = FourCompass[cell_region.four_compass_index][3];
    //const int inv_compass[4] = FourCompass[cell_region.four_compass_index];

    const int old_cells_processed = cells_processed;
    const int node_row_start = cell_region.node_row_start;
    const int node_row_finish = cell_region.node_row_finish;
    const int node_col_start = cell_region.node_col_start;
    const int node_col_finish = cell_region.node_col_finish;
    for (int r =  node_row_start ; r < node_row_finish ; ++r) {
      for (int c = node_col_start ; c < node_col_finish ; ++c) {
        // IMPORTANT: the ordering here must be consistent with the compass ordering
        // See 'Quad.getNodes' (python)
        const int nodes[] = {
          nodes_offset + conv2d(r, c),
          nodes_offset + conv2d(r, c+1),
          nodes_offset + conv2d(r+1, c),
          nodes_offset + conv2d(r+1, c+1)
        };

        // Orient nodes based on compass value
        const int node0 = nodes[inv_compass_0];
        const int node1 = nodes[inv_compass_1];
        const int node2 = nodes[inv_compass_2];
        const int node3 = nodes[inv_compass_3];

        QuadNeighbours&& q = QuadNeighbours{{node0, node1, node2, node3}};
        cell_kernel(cells_processed, std::move(q), mesh, inputs, outputs);
        cells_processed++;
      }
    }
    const int num_cells_this_region =
      (cell_region.node_row_finish - cell_region.node_row_start) *
      (cell_region.node_col_finish - cell_region.node_col_start);
    check(cells_processed - old_cells_processed == num_cells_this_region, "OHNOES");

    nodes_offset += mesh.node_structured_regions[region_index].num_rows *
                    mesh.node_structured_regions[region_index].num_cols;
    ++region_index;
  }

  // Unstructured loop
  check(cells_processed == mesh.unstructured_cells_offset, "what");
  check(mesh.num_cells == mesh.new_cell2ordnodes.size(), "why");
  check(mesh.unstructured_cells_offset <= mesh.num_cells, "but");

  for (auto c = mesh.new_cell2ordnodes.begin() + mesh.unstructured_cells_offset ;
        c != mesh.new_cell2ordnodes.end() ; ++c) {
    const QuadNeighbours& nodes = *c;
    cell_kernel(cells_processed, nodes, mesh, inputs, outputs);
    cells_processed++;
  }

  check(cells_processed == mesh.num_cells, "no");
}

#endif
