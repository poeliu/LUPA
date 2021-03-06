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

#ifndef MYSLOTTRACKER_H_
#define MYSLOTTRACKER_H_

#include "llvm/Value.h"
#include "llvm/Module.h"
#include "llvm/Type.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Instructions.h"
#include <map>
#include <string>
#include <sstream>

using namespace llvm;
using namespace std;

namespace esp {


/*
 * This class is the copy of llvm::SlotTracker
 */
class MySlotTracker {
public:
  MySlotTracker();
  virtual ~MySlotTracker();
public:
  /// ValueMap - A mapping of Values to slot numbers.
  typedef map<const Value*, unsigned> ValueMap;

private:
  /// TheModule - The module for which we are holding slot numbers.
  const Module* TheModule;

  /// TheFunction - The function for which we are holding slot numbers.
  const Function* TheFunction;
  bool FunctionProcessed;

  /// mMap - The TypePlanes map for the module level data.
  ValueMap mMap;
  unsigned mNext;

  /// fMap - The TypePlanes map for the function level data.
  ValueMap fMap;
  unsigned fNext;

  /// mdnMap - Map for MDNodes.
  map<const MDNode*, unsigned> mdnMap;
  unsigned mdnNext;
public:
  /// Construct from a module
  explicit MySlotTracker(const Module *M);
  /// Construct from a function, starting out in incorp state.
  explicit MySlotTracker(const Function *F);

  /// Return the slot number of the specified value in it's type
  /// plane.  If something is not in the SlotTracker, return -1.
  int getLocalSlot(const Value *V);
  int getGlobalSlot(const GlobalValue *V);
  int getMetadataSlot(const MDNode *N);

  /// If you'd like to deal with a function instead of just a module, use
  /// this method to get its data into the SlotTracker.
  void incorporateFunction(const Function *F) {
    TheFunction = F;
    FunctionProcessed = false;
  }

  /// After calling incorporateFunction, use this method to remove the
  /// most recently incorporated function from the SlotTracker. This
  /// will reset the state of the machine back to just the module contents.
  void purgeFunction();

  /// MDNode map iterators.
  typedef map<const MDNode*, unsigned>::iterator mdn_iterator;
  mdn_iterator mdn_begin() { return mdnMap.begin(); }
  mdn_iterator mdn_end() { return mdnMap.end(); }
  unsigned mdn_size() const { return mdnMap.size(); }
  bool mdn_empty() const { return mdnMap.empty(); }

  /// This function does the actual initialization.
  inline void initialize();

  // Return the name of the given variable. If it has name then the name
  // is returned. Otherwise the index is returned ( usually it is a temporary
  // variable).
  string getVariableName(Value* value);

  // Implementation Details
private:
  /// CreateModuleSlot - Insert the specified GlobalValue* into the slot table.
  void CreateModuleSlot(const GlobalValue *V);

  /// CreateMetadataSlot - Insert the specified MDNode* into the slot table.
  void CreateMetadataSlot(const MDNode *N);

  /// CreateFunctionSlot - Insert the specified Value* into the slot table.
  void CreateFunctionSlot(const Value *V);

  /// Add all of the module level global variables (and their initializers)
  /// and function declarations, but not the contents of those functions.
  void processModule();

  /// Add all of the functions arguments, basic blocks, and instructions.
  void processFunction();

  MySlotTracker(const MySlotTracker &);  // DO NOT IMPLEMENT
  void operator=(const MySlotTracker &);  // DO NOT IMPLEMENT

};

} /* namespace esp */
#endif /* MYSLOTTRACKER_H_ */
