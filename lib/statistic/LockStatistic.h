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

#ifndef LOCKSTATISTIC_H_

#include "Statistic.h"
#include "../util/Log.h"
#include "llvm/Value.h"
#include <iostream>
#include <map>
#include <string>
#include  <set>

using namespace std;
using namespace llvm;

namespace esp{

/*
 * Class that stores statistic data
 */
class LockStatistic : public Statistic{
public:
  /*
   * Function name
   */
  string functionName;

  /*
   * Statistic data
   */
  uint lockNumber;                          // Number of locks for each function
  uint directLockNumber;                // Number of lock..unlock patterns
  uint ifLockNumber;                       // Number of if..lock..if..unlock patterns
  uint testLockNumber;                   // Number of if(lock) patterns
  uint staticLockNumber;                // Number of static locks
  uint dynamicLockNumber;           // Number of dynamic locks
  uint globalLockNumber;               // Number of global locks
  uint localLockNumber;                 // Number of local locks
  set<string> lockNames;              // Names of all locks
  set<Value*> originalLocks;         // Original lock variables
  map<string, uint> callDeep;       // Call deep of each lock

  /*
   * Interface functions
   */
  LockStatistic( string name )   {
    lockNumber                 =           0 ;
    directLockNumber       =           0 ;
    ifLockNumber              =           0 ;
    testLockNumber          =           0 ;
    staticLockNumber       =           0 ;
    dynamicLockNumber  =           0 ;
    globalLockNumber      =           0 ;
    localLockNumber        =           0 ;
    callDeep  = map<string, uint>();
    lockNames = set<string>()       ;

    functionName             =  name;
  }

  ~LockStatistic(){}

  void printStatistic();
};

}

#define LOCKSTATISTIC_H_




#endif /* LOCKSTATISTIC_H_ */
