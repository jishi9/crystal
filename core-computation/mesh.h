#ifndef MESH_H
#define MESH_H

#include <iostream>
#include <limits>
#include <vector>
#include <array>

using std::numeric_limits;
using std::ostream;
using std::vector;
using std::array;

// Class representing arbitrary element data
// Each created Data object has an incremented integer 'data'
// class Data {
//   private:
//     static int counter;

//   public:
//     const int data;
//     Data() : data(counter++) {}

//     // For printing
//     friend ostream& operator<<(ostream& out, const Data& me) {
//       out << me.data;
//       return out;
//     }

//     // Reset counter
//     static void reset() {
//       counter = 0;
//     }
// };


// -1 indicates non-existing neighbour
static const int NO_NEIGHBOUR = -1;

// A vector-like class representing a cell's neighbours
template <int NumNeighbours>
using Neighbours = array<int, NumNeighbours>;


// Special case for quad meshes
typedef Neighbours<4> QuadNeighbours;
typedef Neighbours<2> TwoNeighbours;
typedef Neighbours<1> OneNeighbour;



// Element indices corresponding to a 2D structured grid
struct Structured2DGrid {
  vector<int> indices;
  int num_rows;
  int num_cols;
  int offset;
};

const int TwoCompass[][2] = { {0, 1}, {1, 0} };

const int FourCompass[][4] = {
  {0, 1, 2, 3},
  {0, 1, 3, 2},
  {0, 2, 1, 3},
  {0, 2, 3, 1},
  {0, 3, 1, 2},
  {0, 3, 2, 1},
  {1, 0, 2, 3},
  {1, 0, 3, 2},
  {1, 2, 0, 3},
  {1, 2, 3, 0},
  {1, 3, 0, 2},
  {1, 3, 2, 0},
  {2, 0, 1, 3},
  {2, 0, 3, 1},
  {2, 1, 0, 3},
  {2, 1, 3, 0},
  {2, 3, 0, 1},
  {2, 3, 1, 0},
  {3, 0, 1, 2},
  {3, 0, 2, 1},
  {3, 1, 0, 2},
  {3, 1, 2, 0},
  {3, 2, 0, 1},
  {3, 2, 1, 0}
};

struct StructuredCellRegion {
  array<int, 4> inv_compass;
  int four_compass_index;
  int cell2node_offset;
  int node_row_start;
  int node_row_finish;
  int node_col_start;
  int node_col_finish;
};

struct StructuredInedgeRegion {
  array<int, 2> inv_node_compass;
  array<int, 2> inv_cell_compass;
  int two_node_compass_index;
  int two_cell_compass_index;
  int inedge2node_offset;
  int node_row_start;
  int node_row_finish;
  int node_col_start;
  int node_col_finish;
};


// Struct representing all the information about a mesh
typedef struct Mesh {
  // Data: element index -> element data
  // vector<Data> element_data;

  // Adjacency list: element index -> [element index]
  vector<QuadNeighbours> element_neighbours;

  // Number of elements in the mesh
  int num_elems;

  // Structured regions discovered in this mesh
  vector<Structured2DGrid> structured_regions;
} Mesh;


class Coordinate {
  public:
    Coordinate() : x(numeric_limits<double>::quiet_NaN()), y(x) {}
    Coordinate(double x, double y) : x(x), y(y) {}
    double x, y;

    // For printing
    friend ostream& operator<<(ostream& out, const Coordinate& me) {
      out << me.x << ',' << me.y << ' ';
      return out;
    }

    bool operator==(const Coordinate& that) const {
      return x == that.x && y == that.y;
    }

};


// Struct representing all the information about a cell-based mesh
typedef struct CellMesh {
  // Data: node index -> node coordinate data
  vector<Coordinate> node_data;

  // Adjacency list: cell index -> [node index]
  vector<QuadNeighbours> cell_to_nodes;

  // Number of cells in the mesh
  int num_cells;

  // Structured regions discovered in this mesh
  // vector<Structured2DGrid> structured_regions;
} CellMesh;


// Enable data layout transformation
class StructOfArraysNodeData {
  public:
    StructOfArraysNodeData() {}
    StructOfArraysNodeData(const vector<Coordinate>& arr_of_coords)
    : xs(arr_of_coords.size()), ys(arr_of_coords.size()) {
      // Convert to struct of arrays
      for (int node_id = 0; node_id < arr_of_coords.size(); ++node_id) {
        const Coordinate& coord = arr_of_coords[node_id];
        xs[node_id] = coord.x;
        ys[node_id] = coord.y;
      }
    }

    const Coordinate operator[](const int node_id) const {
      return Coordinate(xs[node_id], ys[node_id]);
    }

  private:
    vector<double> xs, ys;
};


typedef struct QuadCellMesh {
  int num_cells;
  int num_nodes;
  int num_internal_edges;
  int num_border_edges;

  // Map: cell index -> [node index]
  vector<QuadNeighbours> cell2ordnodes;
  vector<QuadNeighbours> new_cell2ordnodes;

  // Map: edge index -> [node index]
  vector<TwoNeighbours> inedge2ordnodes;
  vector<TwoNeighbours> new_inedge2ordnodes;
  vector<TwoNeighbours> borderedge2ordnodes;
  vector<TwoNeighbours> new_borderedge2ordnodes;

  // Map: edge index -> [cell index]
  vector<TwoNeighbours> inedge2ordcells;
  vector<TwoNeighbours> new_inedge2ordcells;
  vector<OneNeighbour> borderedge2cell;
  vector<OneNeighbour> new_borderedge2cell;

  // Dat: node index -> Coordinate
  vector<Coordinate> node_coord_data;
  vector<Coordinate> new_node_coord_data;

  // Dat: edge index -> Int
  vector<int> borderedge_bounds;


  // Structured region meta-data
  vector<Structured2DGrid> node_structured_regions;  // Used for offsets only

  vector<StructuredCellRegion> cell_structured_regions;
  int unstructured_cells_offset;

  vector<StructuredInedgeRegion> h_inedge_structured_regions;
  vector<StructuredInedgeRegion> v_inedge_structured_regions;
  int unstructured_inedges_offset;



#ifdef UNSTRUCTURED_SOA
  StructOfArraysNodeData split_node_coord_data;
  inline const StructOfArraysNodeData& get_unstructured_node_data() const {
    return split_node_coord_data;
  }
  typedef StructOfArraysNodeData UnstructuredNodeType;
#else
  inline const vector<Coordinate>& get_unstructured_node_data() const {
    return node_coord_data;
  }
  typedef vector<Coordinate> UnstructuredNodeType;
#endif



#ifdef STRUCTURED_SOA
  StructOfArraysNodeData split_new_node_coord_data;
  inline const StructOfArraysNodeData& get_structured_node_data() const {
    return split_new_node_coord_data;
  }
  typedef StructOfArraysNodeData StructuredNodeType;
#else
  inline const vector<Coordinate>& get_structured_node_data() const {
    return new_node_coord_data;
  }
  typedef vector<Coordinate> StructuredNodeType;
#endif


  double total = 0.0;


} QuadCellMesh;

#endif
