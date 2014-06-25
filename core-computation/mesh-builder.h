#ifndef MESH_BUILDER_H
#define MESH_BUILDER_H

#include <vector>

#include "mesh.h"
#include "protogen/mesh.pb.h"
#include "utils.h"


// Build a 5x7 rectangular mesh with arbitrary structured regions
void build35Mesh(const char* filename);

//Build a very big rectangular mesh
void buildBigMesh(const char* filename);


// In file: *.dat (like new_grid.dat)
// Out file: *.p.part file
void buildQuadMeshPart(const char* in_filename, const char* out_filename);


// Read a quad mesh (*.p file)
QuadCellMesh readQuadMesh(const char* filename);


void buildQuadMeshPartFromMshFile(const char* in_filename, const char* out_filename);

// Reads data from input stream
static inline vector<int> readData(const int num_elems, MeshInputStream* in) {
  in->readSection("data");

  vector<int> data(num_elems);
  for (int& d : data) {
    d = in->readNonNegInt();
  }

  return data;
}


template <int NUM_NEIGHBOURS>
static vector<PNeighbours> readAdjacencyList(const int num_elems, MeshInputStream* in) {
  in->readSection("adjacency_list");

  vector<PNeighbours> elem_to_neighbours(num_elems);
  for (PNeighbours& ns : elem_to_neighbours) {
    in->readProtoBuf(&ns);
    check(ns.element_id_size() == NUM_NEIGHBOURS, "Unexpected number of neighbours");
  }

  return elem_to_neighbours;
}


static inline struct Structured2DGrid readStructuredRegion(MeshInputStream* in) {
  in->readSection("region");
  PStructured2DGrid region;
  in->readProtoBuf(&region);
  const int num_elems = region.num_rows() * region.num_cols();

  // Result
  struct Structured2DGrid result;
  result.num_rows = region.num_rows();
  result.num_cols = region.num_cols();

  // Read indices
  vector<int>& indices = result.indices;
  indices.resize(num_elems);
  for (int& index : indices) {
    index = in->readNonNegInt();
  }
  return result;
}




// Read a mesh from file with the given number of neighbours
template <int NUM_NEIGHBOURS>
void readMesh(const char* filename) {
  Mesh result;

  MeshInputStream in(filename);

  in.readSection("num_cells");
  const int kNumCells = in.readNonNegInt();

  // Data array: element id -> element data
  vector<int> data = readData(kNumCells, &in);

  // Adjacency list: cell_index -> cell_neighbours
  vector<PNeighbours> adj_list = readAdjacencyList<NUM_NEIGHBOURS>(kNumCells, &in);

  // Structured regions
  in.readSection("structured_regions");
  const int num_structured_regions = in.readNonNegInt();

  vector<struct Structured2DGrid> structured_regions;
  structured_regions.reserve(num_structured_regions);
  for (int i = 0 ; i < num_structured_regions ; ++i) {
    structured_regions.push_back(readStructuredRegion(&in));
  }
}


#endif
