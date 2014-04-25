// Copyright 2014 Shanghai Jiao Tong University
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Authors - Trusted Cloud Group (http://tcloud.sjtu.edu.cn)

#ifndef ANALYZER_H_
#define ANALYZER_H_

#include "llvm/Module.h"
#include "SymbolicState.h"

/*
 * Abstract class for analysis
 */
class Analyzer{
public:
  /*
   * Virtual destructor
   */
  virtual ~Analyzer(){}

  /*
   * Pure virtual function to initialize before
   * the analysis begin
   */
  virtual void initExecutor() = 0;

  /*
   * Pure virtual function to run analysis on the
   * given module
   * @Params
   * module: the given module to be analyzed
   */
  virtual void run(llvm::Module *module) = 0;

  /*
   * Pure virtual function to close the analysis
   * while the analyzer is still alive
   */
  virtual void closeAnalysis() = 0;
};


#endif /* ANALYZER_H_ */
