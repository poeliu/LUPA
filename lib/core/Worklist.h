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

#ifndef WORKLIST_H_
#define WORKLIST_H_

#include <set>

using namespace std;

namespace esp {

class Work {
public:

  /*
   * Destructor
   */
  virtual ~Work();

  /*
   * Determine whether two works are the same
   * Implemented by child class
   */
  virtual bool equalsTo(Work *other) = 0;

  /*
   * Print the content of the work
   */
  virtual void printContent() = 0;
};

class Worklist {
protected:
  /* work set */
  set<Work*> works;

  /*
   * Add new work to list
   * @Params
   * work: pointer of the new work
   */
  void addToWorklist(Work* work);

  /*
   * Remove finished work from worklist
   * @Params
   * work: finished work
   */
  void removeFromWorklist(Work *work);

  /*
   * Solve work list
   */
  void solveWorklist();

  /*
   * Get a work from worklist.
   * Subclass can override this function in order to
   * define a new schedule strategy
   */
  virtual Work* selectAWork();

public:
  /*
   * Constructor
   */
  Worklist();

  /*
   * Destructor
   * Implemented by child class
   */
  virtual ~Worklist() = 0;

  /*
   * Define how to finish each work
   * @Params
   * work: work to be handled
   */
  virtual void doEachWork(Work *work) = 0;

  /*
   * Initialize or reinitialize the worklist
   */
  virtual void initWorklist() = 0;
};

} /* namespace esp */
#endif /* WORKLIST_H_ */
