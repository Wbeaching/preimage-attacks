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

#include <cryptominisat5/cryptominisat.h>

#include <map>
#include <set>
#include <vector>
#include <memory>

#include "core/solver.hpp"

namespace preimage {

class CMSatSolver : public Solver {
 public:
  explicit CMSatSolver(bool verbose);

  virtual ~CMSatSolver();

  std::string solverName() const override { return "CryptoMiniSAT"; }

 protected:
  void initialize() override;

  std::unordered_map<int, bool> solveInternal() override;

 private:
  inline CMSat::Lit getLit(int i) const {
    assert(i != 0);
    return CMSat::Lit(std::abs(i) - 1, i < 0);
  }

  void addClause(const LogicGate &g);

  void addXorClause(const LogicGate &g);

  CMSat::SATSolver *solver_;
};

}  // end namespace preimage
