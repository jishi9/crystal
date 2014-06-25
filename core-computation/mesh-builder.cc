#include <algorithm>
#include <fstream>
#include <iterator>
#include <vector>

#include "protogen/mesh.pb.h"
#include "mesh-builder.h"
#include "mesh.h"
#include "utils.h"

using std::begin;
using std::end;
using std::all_of;
using std::array;
using std::ifstream;
using std::streampos;
using google::protobuf::RepeatedField;


// HELPER: Creates an adjacency list for a rectangular mesh of the given dimensions
static void writeGridAdjacencyList(const int num_rows, const int num_cols,
                      MeshOutputStream* out) {
  out->writeSection("adjacency_list");

  // Adjacency list: element index -> element neighbours
  // neighbours[0,1,2,3] = N, E, S, W
  PNeighbours element_neighbours;
  for (int row : IterAtoB(0, num_rows)) {
    for (int col : IterAtoB(0, num_cols)) {
      element_neighbours.clear_element_id();

      element_neighbours.add_element_id(row-1 >= 0 ? num_cols*(row-1) + col : NO_NEIGHBOUR);
      element_neighbours.add_element_id(col+1 < num_cols ? num_cols*row + (col+1) : NO_NEIGHBOUR);
      element_neighbours.add_element_id(row+1 < num_rows ? num_cols*(row+1) + col : NO_NEIGHBOUR);
      element_neighbours.add_element_id(col-1 >= 0 ? num_cols*row + (col-1) : NO_NEIGHBOUR);

      out->writeProtoBuf(element_neighbours);
    }
  }
}


static void writeData(const int num_elems, MeshOutputStream* out) {
  // Data array: node id -> node data
  out->writeSection("data");
  for (int dat : IterAtoB(0, num_elems)) {
    out->writeNonNegInt(dat);
  }
}

template <typename C>
static void appendStructuredRegion(int num_rows, int num_cols,
                                const C& indices, MeshOutputStream* out) {
  // Write meta-data: rows and columns
  out->writeSection("region");
  PStructured2DGrid structured_region;
  structured_region.set_num_rows(num_rows);
  structured_region.set_num_cols(num_cols);
  out->writeProtoBuf(structured_region);

  // Write indices
  for (int index : indices) {
    out->writeNonNegInt(index);
  }
}


// Build a 5x7 rectangular mesh with arbitrary structured regions
void build35Mesh(const char* filename) {
  const int kNumRows = 5;
  const int kNumCols = 7;
  const int kNumCells = kNumRows * kNumCols;

  MeshOutputStream out(filename);

  out.writeSection("num_cells");
  out.writeNonNegInt(kNumCells);

  // Data array: node id -> node data
  writeData(kNumCells, &out);


  // Adjacency list: cell_index -> cell neighbours
  // neighbours[0,1,2,3] = N, E, S, W
  writeGridAdjacencyList(kNumRows, kNumCols, &out);


  // Structured regions
  const int num_structured_regions = 2;
  out.writeSection("structured_regions");
  out.writeNonNegInt(num_structured_regions);

  const auto& region1 = {0,1,2,3,7,8,9,10,14,15,16,17};
  appendStructuredRegion(3, 4, region1, &out);

  const auto& region2 = {11,12,13,18,19,20,25,26,27};
  appendStructuredRegion(3, 3, region2, &out);
}


//Build a very big rectangular mesh
void buildBigMesh(const char* filename) {
  const int kNumRows = 9032;
  const int kNumCols = 1813;
  const int kNumCells = kNumRows * kNumCols;

  MeshOutputStream out(filename);

  out.writeSection("num_cells");
  out.writeNonNegInt(kNumCells);

  // Data array: node id -> node data
  writeData(kNumCells, &out);


  // Adjacency list: cell_index -> cell neighbours
  // neighbours[0,1,2,3] = N, E, S, W
  writeGridAdjacencyList(kNumRows, kNumCols, &out);


  // Structured regions
  const int num_structured_regions = 1;
  out.writeSection("structured_regions");
  out.writeNonNegInt(num_structured_regions);

  const int region1_size = kNumCells;
  IterAtoB region1(0, region1_size);
  appendStructuredRegion(kNumRows, kNumCols, region1, &out);
}


// HELPER
struct QuadMeshMetadata {
  int num_nodes, num_cells, num_internal_edges, num_border_edges;
};


// // HELPER
static struct QuadMeshMetadata readHeader(ifstream* file_stream) {
  // Read header
  struct QuadMeshMetadata result;

  check(*file_stream >> result.num_nodes >> result.num_cells
                    >> result.num_internal_edges >> result.num_border_edges,
                    "Couldn't read file header");
  return result;
}

template<typename C>
static void writeNeighbours(MeshOutputStream* out, const char* section_name, const C& neighbourhoods) {
  out->writeSection(section_name);
  PNeighbours neighbours;
  for (const auto& ns : neighbourhoods) {
    neighbours.clear_element_id();
    for (const int n : ns) {
      neighbours.add_element_id(n);
    }

    out->writeProtoBuf(neighbours);
  }
}

template<typename C>
static void writeBounds(MeshOutputStream* out, const C& bounds) {
  out->writeSection("borderedge_bounds");
  PBound bound_proto;
  for (const int b : bounds) {
    bound_proto.set_bound(b);
    out->writeProtoBuf(bound_proto);
  }
}

// In file: *.dat (like new_grid.dat)
// Out file: *.p.part file
void buildQuadMeshPart(const char* in_filename, const char* out_filename) {
  ifstream in(in_filename);
  check(in, "Cannot open file");

  MeshOutputStream out(out_filename);

  // Read header
  struct QuadMeshMetadata meta = readHeader(&in);

  // Write number of cells, nodes, and edges
  out.writeSection("num_cells");
  out.writeNonNegInt(meta.num_cells);
  out.writeSection("num_nodes");
  out.writeNonNegInt(meta.num_nodes);
  out.writeSection("num_inedges");
  out.writeNonNegInt(meta.num_internal_edges);
  out.writeSection("num_borderedges");
  out.writeNonNegInt(meta.num_border_edges);

  // Read and write node data (coordinates)
  out.writeSection("coord_data");
  PCoordinate coord;
  for (int i = 0 ; i < meta.num_nodes ; ++i) {
    double x, y;
    check(in >> x >> y, "Couldn't read node data");
    coord.set_x(x);
    coord.set_y(y);
    out.writeProtoBuf(coord);
  }


  // Read and write cell data (cell to node)
  out.writeSection("cell_to_ord_nodes");
  const int NUM_CELL_NEIGHBOURS = 4;
  PNeighbours cell_neighbours;
  RepeatedField<int>* ns = cell_neighbours.mutable_element_id();
  ns->Reserve(NUM_CELL_NEIGHBOURS);  // Ensure the following pointers are not invalidated
  int& n_a = *ns->Add();
  int& n_b = *ns->Add();
  int& n_c = *ns->Add();
  int& n_d = *ns->Add();

  for (int i = 0 ; i < meta.num_cells ; ++i) {
    check(in >> n_a >> n_b >> n_c >> n_d, "Couldn't read cell data");
    out.writeProtoBuf(cell_neighbours);
  }


  // Remember edge-info position
  const streampos edge_info_position = in.tellg();
  check(edge_info_position != -1, "Failed to get position");


  // Read inedge maps
  vector<array<int, 2>> inedge_to_nodes(meta.num_internal_edges);
  vector<array<int, 2>> inedge_to_cells(meta.num_internal_edges);
  {
    int n1, n2, c1, c2;
    for (int i = 0 ; i < meta.num_internal_edges ; ++i) {
      check(in >> n1 >> n2 >> c1 >> c2, "Couldn't read edges");
      check(n1 != n2, "Cannot have edge between node and itself");

      inedge_to_nodes[i] = {{n1, n2}};
      inedge_to_cells[i] = {{c1, c2}};
    }
  }

  // Write inedge maps
  writeNeighbours(&out, "inedge_to_nodes", inedge_to_nodes);
  writeNeighbours(&out, "inedge_to_cells", inedge_to_cells);





  // Read border-edges maps and dat
  vector<array<int, 2>> borderedge_to_nodes(meta.num_border_edges);
  vector<array<int, 1>> borderedge_to_cell(meta.num_border_edges);
  vector<int> borderedge_bounds(meta.num_border_edges);
  {
    int n1, n2, c, bound;
    for (int i = 0 ; i < meta.num_border_edges ; ++i) {
      check(in >> n1 >> n2 >> c >> bound, "Couldn't read edges");
      check(n1 != n2, "Cannot have edge between node and itself");

      borderedge_to_nodes[i] = {{n1, n2}};
      borderedge_to_cell[i] = {{c}};
      borderedge_bounds[i] = bound;
    }
  }

  // Write border-edges maps
  writeNeighbours(&out, "borderedge_to_nodes", borderedge_to_nodes);
  writeNeighbours(&out, "borderedge_to_cell", borderedge_to_cell);
  writeBounds(&out, borderedge_bounds);



  // Seek back to edge-info
  in.seekg(edge_info_position);
  check(in.good(), "Failed to seek back!");

  // Read node to node adjacency
  vector<vector<int>> node_to_node(meta.num_nodes);
  // Read node_to_node
  int n1, n2, c1, c2;
  for (int i = 0 ; i < meta.num_internal_edges + meta.num_border_edges ; ++i) {
    check(in >> n1 >> n2 >> c1 >> c2, "Couldn't read edges");
    check(n1 != n2, "Cannot have edge between node and itself");

    node_to_node[n1].push_back(n2);
    node_to_node[n2].push_back(n1);
  }

  // Write node_to_node
  writeNeighbours(&out, "node_to_node", node_to_node);

}

template<int NUM_NEIGHBOURS>
static void readNeighboursIntoMap(MeshInputStream* in, const char* section_name,
  const int num_elems, vector<Neighbours<NUM_NEIGHBOURS>>* target) {

  check(target->size() == 0, "Non-zero target for section ", section_name);
  Neighbours<NUM_NEIGHBOURS> reference_copy;
  reference_copy.fill(NO_NEIGHBOUR);
  target->assign(num_elems, reference_copy);

  in->readSection(section_name);

  PNeighbours neighbours_proto;
  for (Neighbours<NUM_NEIGHBOURS>& neighbours : *target) {
    neighbours_proto.clear_element_id();
    in->readProtoBuf(&neighbours_proto);
    check(neighbours_proto.element_id_size() == NUM_NEIGHBOURS, "Mismatched number of neighbours");

    for (int i=0 ; i < NUM_NEIGHBOURS ; ++i) {
      const int n = neighbours_proto.element_id(i);
      neighbours[i] = n;
    }
  }
}

template<int NUM_NEIGHBOURS, typename C>
static array<int, NUM_NEIGHBOURS> invertCompass(const C compass_begin, const C compass_end) {
  /* COMPASS: Unstructured -> Structured */
  const vector<int> compass(compass_begin, compass_end);
  const int compass_len = compass.size();
  check(compass_len == NUM_NEIGHBOURS, "Unexpected compass size: ", NUM_NEIGHBOURS, " vs ", compass_len);

  {
    // Check compass
    vector<int> compass_check(compass_len);
    for (const int direction : compass) {
      check(direction >= 0 && direction < compass_len, "bad compass direction");
      compass_check[direction] += 1;
    }
    check(all_of(compass_check.begin(), compass_check.end(),
      [] (const int value_count) { return value_count == 1; }),
      "Duplicate or missing values in compass");
  }

  // Invert compass: Structured -> Unstructured
  array<int, NUM_NEIGHBOURS> inv_compass;
  #pragma parallel
  for (int unstructured_direction : IterAtoB(0, compass_len)) {
    const int structured_direction = compass[unstructured_direction];
    inv_compass[structured_direction] = unstructured_direction;
  }

  return inv_compass;
}

constexpr int factorial(const int n) {
  return n > 0 ? n * factorial(n-1) : 1;
}

template <unsigned long COMPASS_SIZE, unsigned long NUM_REFS>
static int getCompassIndex(const array<int, COMPASS_SIZE>& inv_compass,
    const int (&compass_reference)[NUM_REFS][COMPASS_SIZE]) {

  static_assert(factorial(COMPASS_SIZE) == NUM_REFS, "Mismatched compass size and compass-refs size");


  auto compare_func = [&inv_compass] (const int (&ref)[COMPASS_SIZE]) {
    for (int i=0 ; i<COMPASS_SIZE ; ++i) {
      if (ref[i] != inv_compass[i]) return false;
    }
    return true;
  };

  int pos = std::find_if(begin(compass_reference), end(compass_reference), compare_func) - begin(compass_reference);
  check(pos < NUM_REFS, "Compass did not match anything!");
  return pos;
}

static int getCompassIndex(array<int, 2>& inv_compass) {
  return getCompassIndex(inv_compass, TwoCompass);
}

static int getCompassIndex(array<int, 4>& inv_compass) {
  return getCompassIndex(inv_compass, FourCompass);
}


void readCoordinates(MeshInputStream* in, const char* section_name, const int num_elems,
  vector<Coordinate>* target) {

  check(target->size() == 0, "Non-zero target for section ", section_name);
  target->resize(num_elems);

  in->readSection(section_name);
  PCoordinate coord_proto;
  for (Coordinate& coord : *target) {
    coord_proto.Clear();
    in->readProtoBuf(&coord_proto);
    coord.x = coord_proto.x();
    coord.y = coord_proto.y();
  }
}

void readBounds(MeshInputStream* in, const char* section_name, const int num_elems,
  vector<int>* target) {

  check(target->size() == 0, "Non-zero target for section ", section_name);
  target->resize(num_elems);

  in->readSection(section_name);
  PBound bound_proto;
  for (int& b : *target) {
    bound_proto.Clear();
    in->readProtoBuf(&bound_proto);
    b = bound_proto.bound();
  }
}



QuadCellMesh readQuadMesh(const char* filename) {
  MeshInputStream in(filename);

  QuadCellMesh result_mesh;

  // Read number of cells and number of nodes
  in.readSection("num_cells");
  const int num_cells = in.readNonNegInt();
  in.readSection("num_nodes");
  const int num_nodes = in.readNonNegInt();
  in.readSection("num_inedges");
  const int num_internal_edges = in.readNonNegInt();
  in.readSection("num_borderedges");
  const int num_border_edges = in.readNonNegInt();

  result_mesh.num_cells = num_cells;
  result_mesh.num_nodes = num_nodes;
  result_mesh.num_internal_edges = num_internal_edges;
  result_mesh.num_border_edges = num_border_edges;


  // Read dats
  readCoordinates(&in, "coord_data", num_nodes, &result_mesh.node_coord_data);
  readCoordinates(&in, "new_coord_data", num_nodes, &result_mesh.new_node_coord_data);
  readBounds(&in, "borderedge_bounds", num_border_edges, &result_mesh.borderedge_bounds);


  // Read unstructured maps
  readNeighboursIntoMap<4>(&in, "cell_to_ord_nodes", num_cells, &result_mesh.cell2ordnodes);
  readNeighboursIntoMap<2>(&in, "inedge_to_nodes", num_internal_edges, &result_mesh.inedge2ordnodes);
  readNeighboursIntoMap<2>(&in, "inedge_to_cells", num_internal_edges, &result_mesh.inedge2ordcells);
  readNeighboursIntoMap<2>(&in, "borderedge_to_nodes", num_border_edges, &result_mesh.borderedge2ordnodes);
  readNeighboursIntoMap<1>(&in, "borderedge_to_cell", num_border_edges, &result_mesh.borderedge2cell);


  // Read structured maps
  readNeighboursIntoMap<4>(&in, "new_cell_to_ord_nodes", num_cells, &result_mesh.new_cell2ordnodes);
  readNeighboursIntoMap<2>(&in, "new_inedge_to_nodes", num_internal_edges, &result_mesh.new_inedge2ordnodes);
  readNeighboursIntoMap<2>(&in, "new_inedge_to_cells", num_internal_edges, &result_mesh.new_inedge2ordcells);
  readNeighboursIntoMap<2>(&in, "new_borderedge_to_nodes", num_border_edges, &result_mesh.new_borderedge2ordnodes);
  readNeighboursIntoMap<1>(&in, "new_borderedge_to_cell", num_border_edges, &result_mesh.new_borderedge2cell);



  // Read structured node regions
  int num_node_structured_regions;
  {
    in.readSection("structured_node_regions");
    num_node_structured_regions = in.readNonNegInt();
    result_mesh.node_structured_regions.resize(num_node_structured_regions);

    int num_elems_thus_far = 0;
    for (Structured2DGrid& this_region : result_mesh.node_structured_regions) {
      in.readString("region");
      PStructuredNodeRegion region;
      in.readProtoBuf(&region);

      this_region.num_rows = region.num_rows();
      this_region.num_cols = region.num_cols();
      this_region.offset = num_elems_thus_far;

      in.readString("data");
      const int num_elems = this_region.num_rows * this_region.num_cols;
      num_elems_thus_far += num_elems;

      vector<int>& indices = this_region.indices;
      indices.resize(num_elems);
      for (int& i : indices) {
        i = in.readNonNegInt();
      }
    }
  }


  int total_cells = 0;
  // Read structured cell regions
  {
    in.readSection("structured_cell_regions");
    const int num_cell_structured_regions = in.readNonNegInt();
    check(num_cell_structured_regions == num_node_structured_regions,
      "Number of structured regions should be the same for nodes and cells");
    result_mesh.cell_structured_regions.resize(num_cell_structured_regions);

    // Structured regions metadata
    for (StructuredCellRegion& this_region : result_mesh.cell_structured_regions) {
      in.readString("region");
      PStructuredCellRegion region;
      in.readProtoBuf(&region);
      this_region.cell2node_offset = region.cell2node_offset();
      this_region.node_row_start = region.node_row_start();
      this_region.node_row_finish = region.node_row_finish();
      this_region.node_col_start = region.node_col_start();
      this_region.node_col_finish = region.node_col_finish();

      this_region.inv_compass = invertCompass<4>(region.compass().begin(), region.compass().end());
      this_region.four_compass_index = getCompassIndex(this_region.inv_compass);

      check(this_region.cell2node_offset == total_cells, "Mismatching cell count");
      const int num_cells_in_this_region =
        (this_region.node_row_finish - this_region.node_row_start) *
        (this_region.node_col_finish - this_region.node_col_start);
      total_cells += num_cells_in_this_region;
    }
  }



  // Unstructured cell region metadata
  {
    in.readSection("unstructured_cell_regions");
    PUnstructuredCellRegion region;
    in.readProtoBuf(&region);
    result_mesh.unstructured_cells_offset = region.unstructured_cells_offset();

    total_cells += region.num_unstructured_cells();
    check(total_cells == num_cells, "Cell number mismatch..");
  }



  int total_inedges = 0;
  // Read structured inedge regions
  {
    // FUNCTION: Read structured edge region
    auto read_edge_structured_region = [&in, &total_inedges] () {
      StructuredInedgeRegion this_region;
      PStructuredEdgeRegion region;

      in.readString("region");
      in.readProtoBuf(&region);

      this_region.inedge2node_offset = region.inedge2node_offset();
      this_region.node_row_start = region.node_row_start();
      this_region.node_row_finish = region.node_row_finish();
      this_region.node_col_start = region.node_col_start();
      this_region.node_col_finish = region.node_col_finish();

      this_region.inv_node_compass = invertCompass<2>(region.node_compass().begin(), region.node_compass().end());
      this_region.inv_cell_compass = invertCompass<2>(region.cell_compass().begin(), region.cell_compass().end());
      this_region.two_node_compass_index = getCompassIndex(this_region.inv_node_compass);
      this_region.two_cell_compass_index = getCompassIndex(this_region.inv_cell_compass);


      // TODO this does not take into account the edges of differing orientation
      //check(this_region.inedge2node_offset == total_inedges, "Mismatching inedge count: ", this_region.inedge2node_offset, " vs ", total_inedges);
      const int num_inedges_in_this_region =
        (this_region.node_row_finish - this_region.node_row_start) *
        (this_region.node_col_finish - this_region.node_col_start);
      total_inedges += num_inedges_in_this_region;

      return this_region;
    };
    // END FUNCTION

    in.readSection("structured_edge_regions");

    {
      in.readString("structured_h_edge_regions");
      const int num_h_inedge_structured_regions = in.readNonNegInt();
      check(num_h_inedge_structured_regions == num_node_structured_regions,
        "Number of structured regions should be the same for nodes and h_inedges");

      result_mesh.h_inedge_structured_regions.resize(num_h_inedge_structured_regions);
      generate(result_mesh.h_inedge_structured_regions.begin(),
        result_mesh.h_inedge_structured_regions.end(),
        read_edge_structured_region);
    }


    {
      in.readString("structured_v_edge_regions");
      const int num_v_inedge_structured_regions = in.readNonNegInt();
      check(num_v_inedge_structured_regions == num_node_structured_regions,
        "Number of structured regions should be the same for nodes and v_inedges");

      result_mesh.v_inedge_structured_regions.resize(num_v_inedge_structured_regions);
      generate(result_mesh.v_inedge_structured_regions.begin(),
        result_mesh.v_inedge_structured_regions.end(),
        read_edge_structured_region);
    }

    in.readString("unstructured_edges_offset");
    result_mesh.unstructured_inedges_offset = in.readNonNegInt();
  }



  // Struct of arrays data layout
#ifdef UNSTRUCTURED_SOA
  result_mesh.split_node_coord_data = StructOfArraysNodeData(result_mesh.node_coord_data);
#endif

#ifdef STRUCTURED_SOA
  result_mesh.split_new_node_coord_data = StructOfArraysNodeData(result_mesh.new_node_coord_data);
#endif

  return result_mesh;
}





void buildQuadMeshPartFromMshFile(const char* in_filename, const char* out_filename) {
  ifstream in(in_filename);
  MeshOutputStream out(out_filename);

  const int max_line_length = 128;
  char line[max_line_length];

  // Function to read lines
  auto readLine = [&in, &line, max_line_length] () {
    in.getline(line, max_line_length);
    check(in.good(), "Error reading line");
    return line;
  };

  // Read nodes section
  while (!match(readLine(), "$Nodes"));

  int num_nodes;
  check(in >> num_nodes, "Failed to read num_nodes");

  {
    out.writeSection("coord_data");
    PCoordinate coord;
    int node_id;
    double x, y, z;
    for (int i=0 ; i<num_nodes ; ++i) {
      check(in >> node_id >> x >> y >> z, "Failed to read node line ", i);
      check(node_id == i + 1, "node_id non-serial");

      check(z == 0.0, "Non-zero Z axis");
      coord.set_x(x);
      coord.set_y(y);
      out.writeProtoBuf(coord);
    }
    check(match(readLine(), ""), "Skip to EndNodes failed"); // Skip
    check(match(readLine(), "$EndNodes"), "$EndNodes expected. Found '", line, "'");
  }


  // Read elements section
  check(match(readLine(), "$Elements"), "Expected $Elements");

  int num_elements;
  check(in >> num_elements, "Failed to read num_elements");

  int element_id, element_type, num_tags, n1, n2, n3, n4;

  auto readIdAndType = [&in, &element_id, &element_type] (int expected_id) {
    check(in >> element_id >> element_type, "Failed to read element id and type");
    check(element_id == expected_id, "element_id non-serial");
  };

  auto readTags = [&in, &num_tags] () {
    int tag;
    check(in >> num_tags, "Failed to read numtags");
    for ( int i=0 ; i < num_tags ; ++i) {
      check(in >> tag, "Failed to read tag");
    }
  };


  bool pending_element = false;

  // Read edges
  out.writeSection("inedge_to_nodes");
  PNeighbours neighbours_proto;
  int current_element_id = 0;
  for (; current_element_id < num_elements; ++current_element_id) {
    readIdAndType(current_element_id+1);

    // Non-edge encountered, stop
    if (element_type != 1) {
      pending_element = true;
      break;
    }

    readTags();
    check(in >> n1 >> n2, "Failed to read edge nodes");
    n1--; n2--;
    neighbours_proto.clear_element_id();
    neighbours_proto.add_element_id(n1);
    neighbours_proto.add_element_id(n2);
    out.writeProtoBuf(neighbours_proto);
  }

  const int num_internal_edges = current_element_id;

  // Read cells (and node_to_node)
  vector<vector<int>> node_to_node(num_nodes);
  auto add_neighbours = [&node_to_node] (int a, int b) {
    node_to_node[a].push_back(b);
    node_to_node[b].push_back(a);
  };

  out.writeSection("cell_to_ord_nodes");
  int cell_id = 0;
  // Read pending cell
  if (pending_element) {
    check(element_type == 3, "Element type unexpected");
    readTags();
    check(in >> n1 >> n2 >> n3 >> n4, "Failed to read cell nodes");
    n1--; n2--; n3--; n4--;

    // Write cell nodes
    neighbours_proto.clear_element_id();
    neighbours_proto.add_element_id(n1);
    neighbours_proto.add_element_id(n2);
    neighbours_proto.add_element_id(n3);
    neighbours_proto.add_element_id(n4);
    out.writeProtoBuf(neighbours_proto);

    // Update node neighbours
    add_neighbours(n1, n2);
    add_neighbours(n2, n3);
    add_neighbours(n3, n4);
    add_neighbours(n4, n1);

    current_element_id++;
    cell_id++;
  }

  for (; current_element_id < num_elements ; ++current_element_id, ++cell_id ) {
    readIdAndType(current_element_id+1);
    check(element_type == 3, "Element type unexpected");
    readTags();
    check(in >> n1 >> n2 >> n3 >> n4, "Failed to read cell nodes");
    n1--; n2--; n3--; n4--;

    // Write cell nodes
    neighbours_proto.clear_element_id();
    neighbours_proto.add_element_id(n1);
    neighbours_proto.add_element_id(n2);
    neighbours_proto.add_element_id(n3);
    neighbours_proto.add_element_id(n4);
    out.writeProtoBuf(neighbours_proto);

    // Update node neighbours
    add_neighbours(n1, n2);
    add_neighbours(n2, n3);
    add_neighbours(n3, n4);
    add_neighbours(n4, n1);
  }

  const int num_cells = cell_id;

  check(match(readLine(), ""), "Skip to EndElements failed"); // Skip
  check(match(readLine(), "$EndElements"), "$EndElements expected");

  check(num_cells + num_internal_edges == num_elements, "Size mismatch");

  // Write node_to_node
  writeNeighbours(&out, "node_to_node", node_to_node);

  out.writeSection("num_nodes");
  out.writeNonNegInt(num_nodes);
  out.writeSection("num_cells");
  out.writeNonNegInt(num_cells);
  out.writeSection("num_inedges");
  out.writeNonNegInt(num_internal_edges);
}





// Mesh loadQuadMeshWithStructure() {
//   const char filename[] = "airfoil_node_to_node_with_structure.data";

//   ifstream file_stream(filename);
//   check(file_stream, "Cannot open file");

//   // Read header
//   int num_structured_regions;
//   check(file_stream >> num_structured_regions, "Couldn't read file header");


//   // Structured regions
//   vector<Structured2DGrid> structured_regions(num_structured_regions);
//   for (int i = 0; i < num_structured_regions; ++i) {
//     Structured2DGrid& this_region = structured_regions[i];

//     // Read number of rows and columns
//     int num_rows, num_cols;
//     check(file_stream >> num_rows >> num_cols,
//           "Couldn't read structured region header");
//     const int num_nodes = num_rows * num_cols;

//     this_region.num_rows = num_rows;
//     this_region.num_cols = num_cols;

//     // Read region data
//     this_region.indices.fill(num_nodes);
//     for (int i = 0; i < num_rows * num_cols; ++i) {
//       check(file_stream >> this_region.indices[i], "Couldn't read structured node");
//     }
//   }


//   // Node data
//   int total_num_nodes;
//   check(file_stream >> total_num_nodes, "Couldn't read unstructured region header");


//   vector<int> neighbours(total_num_nodes);
//   for (int i = 0; i < total_num_nodes; ++i) {
//     check(file_stream >> unstructured_nodes[i], "Couldn't read unstructured node");
//   }


//   // Read coordinates
//   const char filename[] = "new_grid.dat";
//   ifstream file_stream(filename);
//   check(file_stream, "Cannot open file");
//   struct QuadMeshMetadata meta = readHeader(ifstream);
//   vector<Coordinate> node_data = readCoordinates(&file_stream, meta.num_nodes);

//   return {node_data, element_neighbours, total_num_nodes, structured_regions};
// }
