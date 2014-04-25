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

#ifndef MODULESTATISTIC_H_
#define MODULESTATISTIC_H_

#include "Statistic.h"
#include "FunctionStatistic.h"
#include "llvm/Module.h"
#include <string>
#include <iostream>
#include <map>

using namespace std;
using namespace llvm;

namespace esp {

class ModuleStatistic: public Statistic {
public:
  string applicationName;

  map<Function*, FunctionStatistic*> functionStatistics;

  uint functionNumber;
  uint lockFunctionNumber;

  FunctionStatistic *currentStatistic;

  ModuleStatistic(){
    applicationName      = ""     ;
    functionNumber        = 0      ;
    lockFunctionNumber = 0      ;
  }

  ModuleStatistic(string name){
    applicationName      = name;
    functionNumber        = 0      ;
    lockFunctionNumber = 0      ;
  }

  virtual ~ModuleStatistic(){}

  virtual void printStatistic();
};

} /* namespace esp */
#endif /* MODULESTATISTIC_H_ */
