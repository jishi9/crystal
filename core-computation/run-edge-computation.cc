#include "edge-computation.h"
#include "mesh-builder.h"
#include "mesh.h"
#include "utils.h"

using std::forward_as_tuple;
using std::make_tuple;

auto& structured_kernel = difference_edge_kernel<Structured>;
auto& unstructured_kernel = difference_edge_kernel<Unstructured>;


int main(int argc, const char* argv[]) {
  // Check arguments
  const char* usage = "USAGE:\n"
    "    bin/run-edge-computation [filename] [--structured]\n";
  check(argc == 2 || argc == 3, usage);
  if (argc == 3) {
    check(match("--structured", argv[2]), "Unknown flag");
  }
  const bool structured_flag = argc == 3;


  // Get mesh filename
  const char* filename = argv[1];


  // Read mesh
  const QuadCellMesh mesh = readQuadMesh(filename);

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
  double total_sum = 0.0;
  const auto EmptyInputs = make_tuple();
  auto Outputs = make_tuple(&total_sum);

  if (structured_flag) {
    Timer t;
    run_structured_edges(mesh, structured_kernel, EmptyInputs, Outputs);
    t.lapAndPrint("Structured run completed");
    printf("Sum = %f\n", total_sum);
  }
  else {
    Timer t;
    run_unstructured_edges(mesh, unstructured_kernel, EmptyInputs, Outputs);
    t.lapAndPrint("Unstructured run completed");
    printf("Sum = %f\n", total_sum);
  }
}
