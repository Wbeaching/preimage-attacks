/*
 * Hash reversal
 *
 * Copyright (c) 2020 Authors:
 *   - Trevor Phillips <trevphil3@gmail.com>
 *
 * All rights reserved.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */

#pragma once

#include <map>
#include <vector>
#include <string>

#include "core/factor.hpp"
#include "core/bit.hpp"

namespace preimage {

class Solver {
 public:
  explicit Solver(bool verbose);

  virtual ~Solver();

  void setFactors(const std::map<size_t, Factor> &factors);

  void setInputIndices(const std::vector<size_t> &input_indices);

  void setObserved(const std::map<size_t, bool> &observed);

  std::map<size_t, bool> solve();

  virtual std::string solverName() const = 0;

 protected:
  virtual void initialize() = 0;

  virtual std::map<size_t, bool> solveInternal() = 0;

  bool verbose_;
  std::map<size_t, Factor> factors_;
  std::vector<size_t> input_indices_;
  std::map<size_t, bool> observed_;

 private:
  void setImplicitObserved();
  size_t propagateBackward();
  void propagateForward(size_t smallest_obs);
};

}  // end namespace preimage