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

#ifndef GLOBALSTATISTIC_H_
#define GLOBALSTATISTIC_H_

#include "LockStatistic.h"
#include "../util/Log.h"
#include <iostream>
#include <stack>

using namespace std;

namespace esp{

class GlobalStatistic : public Statistic{
public:

  string applicationName;

  stack<LockStatistic*> lockStatistics;

  uint functionNumber;
  uint lockFunctionNumber;

  GlobalStatistic(){
    applicationName      = ""     ;
    functionNumber        = 0      ;
    lockFunctionNumber = 0      ;
    lockStatistics = stack<LockStatistic*>();
  }

  GlobalStatistic(string name){
    applicationName      = name;
    functionNumber        = 0      ;
    lockFunctionNumber = 0      ;
    lockStatistics = stack<LockStatistic*>();
  }

  ~GlobalStatistic(){}

  LockStatistic* getLocal(){return lockStatistics.top();}

  void printStatistic();
};
}


#endif /* GLOBALSTATISTIC_H_ */
