#ifndef DATA_RELABELER
#define DATA_RELABELER

#include <iostream>
#include <unordered_map>

#include "mesh.h"
#include "utils.h"

using std::cerr;
using std::endl;
using std::make_pair;
using std::unordered_map;


class DataRelabeler {
  public:
    // Meta-data about a structured region in the new data array
    class StructuredRegion {
      public:
        StructuredRegion(int offset, int num_rows, int num_cols) :
          offset(offset),
          num_rows(num_rows),
          num_cols(num_cols) {}

        const int offset;
        const int num_rows;
        const int num_cols;
    };

  private:
    void addOldToNewMapping(int old_index, int new_index) {
      if (!conditionalAddOldToNewMapping(old_index, new_index)) {
        cerr << "Re-adding existing index" << endl;
        exit(1);
      }
    }

    // Returns true if mapping added successfully, false if old_index already added.
    bool conditionalAddOldToNewMapping(int old_index, int new_index) {
      const auto result = oldToNewIndex.insert(make_pair(old_index, new_index));
      return result.second;
    }

    // Meta-data about structured regions
    vector<StructuredRegion> structured_regions;


    // Current offset into new reordered_elem_indices
    int current_offset = 0;

    // Reordered elem indices
    vector<int> reordered_elem_indices;

    // BitMask over reordered_elem_indices - True -> Unstructured, False -> Structured
    vector<bool> elem_type_bitmask;


    // Map: old_elem_index -> new_elem_index
    unordered_map<int, int> oldToNewIndex;


    // Offset (into reordered_elem_indices) at which unstructured region begins
    // *NOTE* that some parts of the structured region (halos) are actually in the unstructured region
    int unstructured_start_offset = -1;

  public:

    // Test whether elem is in unstructured region
    bool isUnstructured(int index) {
      return elem_type_bitmask[index];
    }

    // Get list of structured regions
    const vector<StructuredRegion>& getStructuredRegions() {
      return structured_regions;
    }

    template <typename const_iter>
    void addStructuredGrid(const_iter original_indices, int num_rows, int num_cols) {
      check(num_rows >= 2 && num_cols >= 2, "Structured region too small");

      structured_regions.emplace_back(current_offset, num_rows, num_cols);

      // Append structured region elem indices and mapping
      const const_iter end_ptr = original_indices + num_rows*num_cols;
      for (const_iter ptr=original_indices; ptr!=end_ptr ; ++ptr) {
        const int elem_index = *ptr;

        // Add mapping: elem_index -> offset (AKA new index)
        addOldToNewMapping(elem_index, current_offset);

        // Add elem_index to data array
        reordered_elem_indices.push_back(elem_index);
        current_offset++;
      }

      /* Append elem-status to bitmask */

      // First row is part of the unstructured data
      elem_type_bitmask.insert(elem_type_bitmask.end(), num_cols, true);

      // From second to penultimate row, only first and last element of each
      // row are unstructured data
      for (int i = 1; i < num_rows-1; ++i) {
        // First element in row is unstructured data
        elem_type_bitmask.push_back(true);

        // Middle elements in row are structured data
        elem_type_bitmask.insert(elem_type_bitmask.end(), num_cols-2, false);

        // Last element in row is unstructured data
        elem_type_bitmask.push_back(true);
      }

      // Last row is part of the unstructured data
      elem_type_bitmask.insert(elem_type_bitmask.end(), num_cols, true);
    }


    template <typename const_iter>
    void addUnstructuredData(const_iter original_indices, int num_elems) {
      unstructured_start_offset = current_offset;

      // Append unstructured region elem indices and mapping
      const const_iter end_ptr = original_indices + num_elems;
      for (const_iter ptr=original_indices ; ptr!=end_ptr; ++ptr) {
        const int elem_index = *ptr;

        // Add mapping: elem_index -> offset (AKA new index)
        bool first_seen = conditionalAddOldToNewMapping(elem_index, current_offset);

        // If elem_index was first seen now (i.e. is not part of a structured region)
        // then append it to reordered_elem_indices
        if (first_seen) {
          // Append elem_index to data array
          reordered_elem_indices.push_back(elem_index);
          current_offset++;

          // Append elem-status to bitmask
          elem_type_bitmask.push_back(true);
        }
      }
    }

    int translate(int old_index) const {
      if (old_index == NO_NEIGHBOUR) return NO_NEIGHBOUR;

      const auto match = oldToNewIndex.find(old_index);
      if (match == oldToNewIndex.end()) {
        cerr << "mismatched index in neighbour map: " << old_index << endl;
        exit(1);
      }

      // new index
      return match->second;
    }

    // Assumes that oldToNewIndex is a bijection: [0, num_elems) -> [0, num_elems)
    vector<QuadNeighbours> translateIndices(const vector<QuadNeighbours>& neighbourhoods) {
      QuadNeighbours reference_copy {NO_NEIGHBOUR, NO_NEIGHBOUR, NO_NEIGHBOUR, NO_NEIGHBOUR};
      vector<QuadNeighbours> out(neighbourhoods.size(), reference_copy);

      for (int node=0 ; node<neighbourhoods.size() ; ++node) {
        // Translate an old node index to its new index
        const int new_node_index = translate(node);

        // Translate neighbours and write to new index
        const QuadNeighbours::const_iterator old_neighbour_end = neighbourhoods[node].end();
        QuadNeighbours::const_iterator old_neighbour = neighbourhoods[node].begin();
        QuadNeighbours::iterator new_neighbour = out[new_node_index].begin();
        for ( ; old_neighbour != old_neighbour_end ; ++old_neighbour, ++new_neighbour) {
          *new_neighbour = translate(*old_neighbour);
        }
      }
      return out;
    }


    // Translate only the *value* of the map
    vector<QuadNeighbours> translateIndicesPartial(const vector<QuadNeighbours>& neighbourhoods) {
      QuadNeighbours reference_copy {NO_NEIGHBOUR, NO_NEIGHBOUR, NO_NEIGHBOUR, NO_NEIGHBOUR};
      vector<QuadNeighbours> out(neighbourhoods.size(), reference_copy);

      /*QuadNeighbours::const_iterator*/ auto old_iter = neighbourhoods.begin();
      const /*QuadNeighbours::const_iterator*/ auto end_old_iter = neighbourhoods.end();
      /*QuadNeighbours::iterator*/ auto new_iter = out.begin();

      for ( ; old_iter != end_old_iter; ++old_iter, ++new_iter) {
        const QuadNeighbours& old_neighbours = *old_iter;
        QuadNeighbours& new_neighbours = *new_iter;
        for (int i = 0; i < 4; ++i) {
          new_neighbours[i] = translate(old_neighbours[i]);
        }
      }
      return out;
    }


    template <typename T>
    vector<T> reorderByElemIndices(const vector<T>& data) {
      vector<T> out;
      out.reserve(data.size());

      for (int index : reordered_elem_indices) {
        out.push_back(data[index]);
      }

      return out;
    }

    vector<int> getNewToOldMap() {
      vector<int> new_to_old(oldToNewIndex.size());

      for (const auto& old_to_new : oldToNewIndex) {
        int new_index = old_to_new.second;
        int old_index = old_to_new.first;

        new_to_old[new_index] = old_index;
      }
      return new_to_old;
    }
};


#endif