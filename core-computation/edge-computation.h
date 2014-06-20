#ifndef EDGE_COMPUTATION_H
#define EDGE_COMPUTATION_H

#include "mesh.h"
#include "utils.h"

#include <tuple>
#include <vector>

using std::get;
using std::tuple;
using std::vector;


// Edge kernel function type
template<typename... Output>
using KEdgeFunc = void (
  const int node0_id, const int node1_id,
  const int cell0_id, const int cell1_id,
  const QuadNeighbours& cell0_nodes, const QuadNeighbours& cell1_nodes,
  const int edge_id, const QuadCellMesh& mesh, Output*... output);


static inline void null_edge_kernel(
  const int node0_id, const int node1_id,
  const int cell0_id, const int cell1_id,
  const QuadNeighbours& cell0_nodes, const QuadNeighbours& cell1_nodes,
  const int edge_id, const QuadCellMesh& mesh) {}


static inline void print_edge_kernel(
  const int node0_id, const int node1_id,
  const int cell0_id, const int cell1_id,
  const QuadNeighbours& cell0_nodes, const QuadNeighbours& cell1_nodes,
  const int edge_id, const QuadCellMesh& mesh) {
  printf("%d -> %d = %d %d %d %d ; %d = %d %d %d %d ; %d %d\n", edge_id,
    cell0_id, cell0_nodes[0], cell0_nodes[1], cell0_nodes[2], cell0_nodes[3],
    cell1_id, cell1_nodes[0], cell1_nodes[1], cell1_nodes[2], cell1_nodes[3],
    node0_id, node1_id);
}


enum MapType { Structured, Unstructured };
using StructuredNodeType = QuadCellMesh::StructuredNodeType;
using UnstructuredNodeType = QuadCellMesh::UnstructuredNodeType;

// Helper to get the correct node data
template<MapType MT> struct item_return;
template<> struct item_return<Structured>{ typedef StructuredNodeType type; };
template<> struct item_return<Unstructured>{ typedef UnstructuredNodeType type; };

template <MapType MT>
const typename item_return<MT>::type& get_node_data(const QuadCellMesh& mesh);

template<>
const StructuredNodeType& get_node_data<Structured>(const QuadCellMesh& mesh) {
  return mesh.get_structured_node_data();
}

template<>
const UnstructuredNodeType& get_node_data<Unstructured>(const QuadCellMesh& mesh) {
  return mesh.get_unstructured_node_data();
}



// TODO PURGE
template<MapType MT>
const vector<QuadNeighbours>& get_cell_to_ord_nodes(const QuadCellMesh& mesh);

template<>
inline const vector<QuadNeighbours>& get_cell_to_ord_nodes<Structured>(const QuadCellMesh& mesh) {
  return mesh.new_cell2ordnodes;
}

template<>
inline const vector<QuadNeighbours>& get_cell_to_ord_nodes<Unstructured>(const QuadCellMesh& mesh) {
  return mesh.cell2ordnodes;
}
// END TODO



template <MapType MT>
static inline void difference_edge_kernel(
  const int node0_id, const int node1_id,
  const int cell0_id, const int cell1_id,
  const QuadNeighbours& cell0_nodes, const QuadNeighbours& cell1_nodes,
  const int edge_id, const QuadCellMesh& mesh,
  const tuple<>&, const tuple<double*>& outputs) {

  const auto& node_data = get_node_data<MT>(mesh);

  const Coordinate& dat0 = node_data[node0_id];
  const Coordinate& dat1 = node_data[node1_id];

  double difference = dat0.x * dat0.y - dat1.x * dat1.y;

  const Coordinate& a = node_data[cell0_nodes[0]];
  const Coordinate& b = node_data[cell0_nodes[1]];
  const Coordinate& c = node_data[cell0_nodes[2]];
  const Coordinate& d = node_data[cell0_nodes[3]];

  double area0 = 4*(a.x + a.y) + 3*(b.x + b.y) + 2*(c.x + c.y) + (d.x + d.y);

  const Coordinate& e = node_data[cell1_nodes[0]];
  const Coordinate& f = node_data[cell1_nodes[1]];
  const Coordinate& g = node_data[cell1_nodes[2]];
  const Coordinate& h = node_data[cell1_nodes[3]];

  double area1 = 4*(e.x + e.y) + 3*(f.x + f.y) + 2*(g.x + g.y) + (h.x + h.y);

  *get<0>(outputs) += difference + area0 - area1;
}



template<typename InArgs, typename OutArgs>
using EdgeKernel = void (&)(const int,const int, const int, const int, const QuadNeighbours&, const QuadNeighbours&, const int, const QuadCellMesh&, const InArgs&, const OutArgs&);

template<typename InArgs, typename OutArgs>
static inline void run_structured_edges(const QuadCellMesh& mesh,
//  void edge_kernel(const int,const int, const int, const int, const QuadNeighbours&, const QuadNeighbours&, const int, const QuadCellMesh&, const InArgs&, const OutArgs&),
  EdgeKernel<InArgs, OutArgs>& edge_kernel,
  const InArgs& inputs, const OutArgs& outputs) {

  const int num_structured_regions = mesh.node_structured_regions.size();
  int in_edges_processed = 0;

  for (int region_index : IterAtoB(0, num_structured_regions)) {
    const Structured2DGrid& node_region = mesh.node_structured_regions[region_index];
    const StructuredCellRegion& cell_region = mesh.cell_structured_regions[region_index];
    const StructuredInedgeRegion& h_inedge_region = mesh.h_inedge_structured_regions[region_index];
    const StructuredInedgeRegion& v_inedge_region = mesh.v_inedge_structured_regions[region_index];

    const int edges_per_row = node_region.num_cols;
    // Coordinate wrt the current node structured-region
    auto conv2d = [edges_per_row] (int row, int col) {
        return row*edges_per_row + col;
    };
    const int nodes_offset = node_region.offset;


    // Cell offset
    const int cell_region_num_cols = cell_region.node_col_finish - cell_region.node_col_start;
    auto convcell2d = [cell_region_num_cols] (int cell_row, int cell_col) {
      return cell_row*cell_region_num_cols + cell_col;
    };

    check(h_inedge_region.node_row_start == v_inedge_region.node_row_start, "Edge node_row_start mismatch");
    check(h_inedge_region.node_col_start == v_inedge_region.node_col_start, "Edge node_col_start mismatch");
    const int edge_node_row_start = h_inedge_region.node_row_start;
    const int edge_node_col_start = h_inedge_region.node_col_start;

    const int cells_row_offset = edge_node_row_start - cell_region.node_row_start - 1;
    const int cells_col_offset = edge_node_col_start - cell_region.node_col_start - 1;
    const int cells_offset = cell_region.cell2node_offset + convcell2d(cells_row_offset, cells_col_offset);


    const int old_edges_processed = in_edges_processed;

#ifdef IGNORE_COMPASS
    const int (&h_inv_node_compass)[2] = TwoCompass[0];
    const int (&v_inv_node_compass)[2] = TwoCompass[0];
    const int (&h_inv_cell_compass)[2] = TwoCompass[1];
    const int (&v_inv_cell_compass)[2] = TwoCompass[0];
    const int (&inv_cell_compass)[4] = FourCompass[3];
#else
    const int (&h_inv_node_compass)[2] = TwoCompass[h_inedge_region.two_node_compass_index];
    const int (&v_inv_node_compass)[2] = TwoCompass[v_inedge_region.two_node_compass_index];
    const int (&h_inv_cell_compass)[2] = TwoCompass[h_inedge_region.two_cell_compass_index];
    const int (&v_inv_cell_compass)[2] = TwoCompass[v_inedge_region.two_cell_compass_index];
    const int (&inv_cell_compass)[4] = FourCompass[cell_region.four_compass_index];
#endif

    check(h_inedge_region.node_row_start == v_inedge_region.node_row_start, "node_row_start mismatch");
    check(h_inedge_region.node_row_finish == v_inedge_region.node_row_finish, "node_row_finish mismatch");
    check(h_inedge_region.node_col_start == v_inedge_region.node_col_start, "node_col_start mismatch");
    check(h_inedge_region.node_col_finish == v_inedge_region.node_col_finish, "node_col_finish mismatch");

    const int node_row_start = h_inedge_region.node_row_start;
    const int node_row_finish = h_inedge_region.node_row_finish;
    const int node_col_start = h_inedge_region.node_col_start;
    const int node_col_finish = h_inedge_region.node_col_finish;
    const int h_inedge_offset = h_inedge_region.inedge2node_offset;
    const int v_inedge_offset = v_inedge_region.inedge2node_offset;

    int edge_offset_in_region = 0;
    for (int r : IterAtoB(node_row_start, node_row_finish)) {
      for (int c : IterAtoB(node_col_start, node_col_finish)) {
        // Node ids of the nodes adjacent to the horizontal edge
        const int h_edge_nodes[] = {
            nodes_offset + conv2d(r, c),
            nodes_offset + conv2d(r, c+1)
        };

        // Node ids of the nodes adjacent to the vertical edge
        const int v_edge_nodes[] = {
            nodes_offset + conv2d(r, c),
            nodes_offset + conv2d(r+1, c)
        };

        // Node ids of the two cells adjacent to the horizontal edge
        const int h_edge_cell0_nodes[] = {
            nodes_offset + conv2d(r-1, c),
            nodes_offset + conv2d(r-1, c+1),
            nodes_offset + conv2d(r, c),
            nodes_offset + conv2d(r, c+1),
        };
        const int h_edge_cell1_nodes[] = {
            nodes_offset + conv2d(r, c),
            nodes_offset + conv2d(r, c+1),
            nodes_offset + conv2d(r+1, c),
            nodes_offset + conv2d(r+1, c+1),
        };

        // Node ids of the two cells adjacent to the vertical edge
        const int v_edge_cell0_nodes[] = {
            nodes_offset + conv2d(r, c-1),
            nodes_offset + conv2d(r, c),
            nodes_offset + conv2d(r+1, c-1),
            nodes_offset + conv2d(r+1, c)
        };

        const int v_edge_cell1_nodes[] = {
          nodes_offset + conv2d(r, c),
            nodes_offset + conv2d(r, c+1),
            nodes_offset + conv2d(r+1, c),
            nodes_offset + conv2d(r+1, c+1)
        };

        // Cell ids of the cells adjacent to the horizontal edge
        const int cell_row = r - node_row_start;
        const int cell_col = c - node_col_start;
        const int h_cells[] = {
          cells_offset + convcell2d(cell_row, cell_col+1),
          cells_offset + convcell2d(cell_row+1, cell_col+1)
        };

        // Cell ids of the cells adjacent to the vertical edge
        const int v_cells[] = {
          cells_offset + convcell2d(cell_row+1, cell_col),
          cells_offset + convcell2d(cell_row+1, cell_col+1)
        };

        // Orient nodes based on compass value
        const int h_edge_node0_id = h_edge_nodes[h_inv_node_compass[0]];
        const int h_edge_node1_id = h_edge_nodes[h_inv_node_compass[1]];
        const int v_edge_node0_id = v_edge_nodes[v_inv_node_compass[0]];
        const int v_edge_node1_id = v_edge_nodes[v_inv_node_compass[1]];

        const int h_cell0_id = h_cells[h_inv_cell_compass[0]];
        const int h_cell1_id = h_cells[h_inv_cell_compass[1]];
        const int v_cell0_id = v_cells[v_inv_cell_compass[0]];
        const int v_cell1_id = v_cells[v_inv_cell_compass[1]];

        // Cell nodes
        const QuadNeighbours h_cell_nodes[2] = {
          {{
            h_edge_cell0_nodes[inv_cell_compass[0]],
            h_edge_cell0_nodes[inv_cell_compass[1]],
            h_edge_cell0_nodes[inv_cell_compass[2]],
            h_edge_cell0_nodes[inv_cell_compass[3]]
          }},
          {{
            h_edge_cell1_nodes[inv_cell_compass[0]],
            h_edge_cell1_nodes[inv_cell_compass[1]],
            h_edge_cell1_nodes[inv_cell_compass[2]],
            h_edge_cell1_nodes[inv_cell_compass[3]]
          }}
        };

        const QuadNeighbours v_cell_nodes[2] = {
          {{
            v_edge_cell0_nodes[inv_cell_compass[0]],
            v_edge_cell0_nodes[inv_cell_compass[1]],
            v_edge_cell0_nodes[inv_cell_compass[2]],
            v_edge_cell0_nodes[inv_cell_compass[3]]
          }},
          {{
            v_edge_cell1_nodes[inv_cell_compass[0]],
            v_edge_cell1_nodes[inv_cell_compass[1]],
            v_edge_cell1_nodes[inv_cell_compass[2]],
            v_edge_cell1_nodes[inv_cell_compass[3]]
          }}
        };

        const QuadNeighbours& h_cell0_nodes = h_cell_nodes[h_inv_cell_compass[0]];
        const QuadNeighbours& h_cell1_nodes = h_cell_nodes[h_inv_cell_compass[1]];
        const QuadNeighbours& v_cell0_nodes = v_cell_nodes[v_inv_cell_compass[0]];
        const QuadNeighbours& v_cell1_nodes = v_cell_nodes[v_inv_cell_compass[1]];


        const int h_edge_id = h_inedge_offset + edge_offset_in_region;
        const int v_edge_id = v_inedge_offset + edge_offset_in_region;

        check(mesh.new_inedge2ordnodes[h_edge_id][0] == h_edge_node0_id, "hnode0");
        check(mesh.new_inedge2ordnodes[h_edge_id][1] == h_edge_node1_id, "hnode1");
        check(mesh.new_inedge2ordnodes[v_edge_id][0] == v_edge_node0_id, "vnode0");
        check(mesh.new_inedge2ordnodes[v_edge_id][1] == v_edge_node1_id, "vnode1");

        check(mesh.new_inedge2ordcells[h_edge_id][0] == h_cell0_id, "hcell0 ", mesh.new_inedge2ordcells[h_edge_id][0], " vs ", h_cell0_id);
        check(mesh.new_inedge2ordcells[h_edge_id][1] == h_cell1_id, "hcell1 ", mesh.new_inedge2ordcells[h_edge_id][1], " vs ", h_cell1_id);
        check(mesh.new_inedge2ordcells[v_edge_id][0] == v_cell0_id, "vcell0 ", mesh.new_inedge2ordcells[v_edge_id][0], " vs ", v_cell0_id);
        check(mesh.new_inedge2ordcells[v_edge_id][1] == v_cell1_id, "vcell1 ", mesh.new_inedge2ordcells[v_edge_id][1], " vs ", v_cell1_id);
//        const int edge_offset_in_region = r*edges_per_row + c;
//        const int h_edge_id = h_inedge_offset + edge_offset_in_region;
//        const int v_edge_id = v_inedge_offset + edge_offset_in_region;

        edge_kernel(h_edge_node0_id, h_edge_node1_id, h_cell0_id, h_cell1_id,
          h_cell0_nodes, h_cell1_nodes, h_edge_id, mesh, inputs, outputs);

        edge_kernel(v_edge_node0_id, v_edge_node1_id, v_cell0_id, v_cell1_id,
          v_cell0_nodes, v_cell1_nodes, v_edge_id, mesh, inputs, outputs);

        in_edges_processed += 2;

        edge_offset_in_region++;
      }
    }

    const int num_edges_this_region =
      (node_row_finish - node_row_start) * (node_col_finish - node_col_start);
    check(edge_offset_in_region == num_edges_this_region, "yikes");
    check(in_edges_processed - old_edges_processed == 2 * num_edges_this_region, "OHNOES");
  }

  check(mesh.unstructured_inedges_offset == in_edges_processed, "Number edges processed does not match unstructured offset");
  check(mesh.unstructured_inedges_offset <= mesh.new_inedge2ordnodes.size(), "Bad inedges offset: ", mesh.unstructured_inedges_offset, " vs ", mesh.new_inedge2ordnodes.size());


  for (int edge_id : IterAtoB(mesh.unstructured_inedges_offset, mesh.num_internal_edges)) {
    const TwoNeighbours& nodes = mesh.new_inedge2ordnodes[edge_id];
    const TwoNeighbours& cells = mesh.new_inedge2ordcells[edge_id];

    const int node0_id = nodes[0];
    const int node1_id = nodes[1];
    const int cell0_id = cells[0];
    const int cell1_id = cells[1];

    const QuadNeighbours& cell0_nodes = mesh.new_cell2ordnodes[cell0_id];
    const QuadNeighbours& cell1_nodes = mesh.new_cell2ordnodes[cell1_id];

    edge_kernel(node0_id, node1_id, cell0_id, cell1_id,
      cell0_nodes, cell1_nodes, edge_id, mesh, inputs, outputs);

    in_edges_processed++;
  }
  check(in_edges_processed == mesh.num_internal_edges, "Did not process the expected number of edges");
}



template<typename InArgs, typename OutArgs>
static inline void run_unstructured_edges(const QuadCellMesh& mesh,
  void edge_kernel(const int,const int, const int, const int, const QuadNeighbours&, const QuadNeighbours&, const int, const QuadCellMesh&, const InArgs&, const OutArgs&),
  const InArgs& inputs, const OutArgs& outputs) {

  for (int edge_id : IterAtoB(0, mesh.num_internal_edges)) {
    const TwoNeighbours& nodes = mesh.inedge2ordnodes[edge_id];
    const TwoNeighbours& cells = mesh.inedge2ordcells[edge_id];

    const int node0_id = nodes[0];
    const int node1_id = nodes[1];
    const int cell0_id = cells[0];
    const int cell1_id = cells[1];

    const QuadNeighbours& cell0_nodes = mesh.cell2ordnodes[cell0_id];
    const QuadNeighbours& cell1_nodes = mesh.cell2ordnodes[cell1_id];

    edge_kernel(node0_id, node1_id, cell0_id, cell1_id,
      cell0_nodes, cell1_nodes, edge_id, mesh, inputs, outputs);
  }
}



template<typename InArgs, typename OutArgs>
using BEdgeKernel = void (&)(const int, const int, const int, const QuadNeighbours&, const int, const QuadCellMesh&, const InArgs&, const OutArgs&);

template<typename InArgs, typename OutArgs>
static inline void run_border_edges(
  const QuadCellMesh& mesh,
  const vector<TwoNeighbours>& borderedges_to_nodes,
  const vector<OneNeighbour>& borderedges_to_cell,
  const vector<QuadNeighbours>& cell_to_nodes,
  BEdgeKernel<InArgs, OutArgs>& border_edge_kernel,
//  void (& border_edge_kernel)(const int, const int, const int, const QuadNeighbours&, const int, const QuadCellMesh&, const InArgs&, const OutArgs&),
  const InArgs& inputs, const OutArgs& outputs) {

  for (int edge_id : IterAtoB(0, mesh.num_border_edges)) {
    const TwoNeighbours& nodes = borderedges_to_nodes[edge_id];
    const OneNeighbour& cells = borderedges_to_cell[edge_id];

    const int node0_id = nodes[0];
    const int node1_id = nodes[1];
    const int cell_id = cells[0];

    const QuadNeighbours& cell_nodes = cell_to_nodes[cell_id];

    border_edge_kernel(node0_id, node1_id, cell_id, cell_nodes,
      edge_id, mesh, inputs, outputs);
  }
}

#endif
