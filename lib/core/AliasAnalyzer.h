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

#ifndef ALIASANALYZER_H_
#define ALIASANALYZER_H_

// Whether alias result is read from file
//#define USE_ALIAS_FILE

#include "aliasAnalysis.h"
#include "Anders.h"
#include "llvm/Value.h"
#include "llvm/Function.h"
#include <set>
#include <string>

using namespace llvm;
using namespace std;

namespace esp{

enum AliasResult{
  //MustAlias,
  //MayAlias,
  //NoAlias,
  NoResult,
  NoAlias,
  MayAlias,
  MustAlias
};

class AliasValue{
public:
  string value;
  string function;
};

class AliasSet{
public:

  set<AliasValue*> aliasValues;

  AliasResult aliasType;

  AliasSet(){}

  AliasResult isAlias(Value *va, Value*vb, Function *fa, Function *fb);

  void printAliasSet();

};

class AliasAnalyzer{
private:

  bool allTheSame;

#ifdef USE_ALIAS_FILE

  set<AliasSet*> aliasSets;

  bool readResult(string filePath);

#else

  aliasAnalysis *aa;

#endif

public:

  AliasAnalyzer();

  AliasAnalyzer(bool isAllTheSame);

  ~AliasAnalyzer();

#ifdef USE_ALIAS_FILE
  void run(string filePath);
#else
  void run(Module *module);
#endif

  AliasResult isAlias(Value *va, Value*vb, Function *fa, Function *fb);

#ifdef USE_ALIAS_FILE
  void printAliasSets();
#endif

};

}


#endif /* ALIASANALYZER_H_ */
