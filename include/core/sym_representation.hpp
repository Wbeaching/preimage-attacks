/*
 * Copyright (c) 2022 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 */

#pragma once

#include <boost/dynamic_bitset.hpp>
#include <vector>

#include "core/logic_gate.hpp"
#include "core/sym_bit_vec.hpp"

namespace preimage {

class SymRepresentation {
 public:
  SymRepresentation(const std::vector<int> &input_indices,
                    const std::vector<int> &output_indices);

  int numVars() const;

  std::vector<LogicGate> gates() const;

  std::vector<int> hashInputIndices() const;

  std::vector<int> hashOutputIndices() const;

  void toDAG(const std::string &filename) const;

  void toCNF(const std::string &filename) const;

  void toMIP(const std::string &filename) const;

  void toGraphColoring(const std::string &filename) const;

 private:
  void pruneIrrelevantGates();

  void reindexBits();

  int num_vars_;
  std::vector<LogicGate> gates_;
  std::vector<int> hash_input_indices_;
  std::vector<int> hash_output_indices_;
};

}  // end namespace preimage