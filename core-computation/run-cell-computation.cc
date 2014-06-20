#include "cell-computation.h"
#include "mesh-builder.h"
#include "mesh.h"
#include "utils.h"


int main(int argc, const char* argv[]) {
  // Check arguments
  const char* usage = "USAGE:\n"
    "    bin/run-cell-computation [filename] [--structured]\n";
  check(argc == 2 || argc == 3, usage);
  if (argc == 3) {
    check(match("--structured", argv[2]), "Unknown flag");
  }
  const bool structured_flag = argc == 3;


  // Get mesh filename
  const char* filename = argv[1];

  // Read mesh
  QuadCellMesh mesh = readQuadMesh(filename);

  // Check mesh
  check(mesh.cell2ordnodes.size() == mesh.num_cells, "Bad cell2ordnodes");
  check(mesh.new_cell2ordnodes.size() == mesh.num_cells, "Bad new_cell2ordnodes");

  check(mesh.node_coord_data.size() == mesh.num_nodes, "Bad node_coord_data");
  check(mesh.new_node_coord_data.size() == mesh.num_nodes, "Bad new_node_coord_data");

  check(mesh.node_structured_regions.size() == mesh.node_structured_regions.size(),
    "Cell and node structured regions size mismatch");

  check(mesh.unstructured_cells_offset <= mesh.num_cells,
    "Unstructured cell offset too large");

  // Run
  double total = 0.0;
  if (structured_flag) {
    Timer t;
    run_structured_cells(mesh, area_cell_kernel, mesh.get_structured_node_data(), &total);
    t.lapAndPrint("Structured cells completed");
    printf("Sum = %f\n", total);
  }
  else {
    Timer t;
    run_unstructured_cells(mesh, area_cell_kernel, mesh.get_unstructured_node_data(), &total);
    t.lapAndPrint("unStructured cells completed");
    printf("Sum = %f\n", total);
  }
}
