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

#ifndef LOG_H_
#define LOG_H_

#include <fstream>
#include <string>
#include <sstream>
#include "llvm/Instruction.h"
#include "llvm/Value.h"
#include "llvm/GlobalValue.h"
#include "llvm/Support/raw_ostream.h"
#include "MySlotTracker.h"

using namespace std;
using namespace llvm;

namespace esp{

#define DOUBLE_LOCK 1
#define DOUBLE_UNLOCK 2
#define UNINIT_UNLOCK 3
#define UNRELEASE_LOCK 4

/*
 * Get the name of the value
 */
string getValueName(Value *value);

/*
 * Init log file with default file name
 */
void initLog(string parentPath = "");

/*
 * Return the directory containing the
 * log file
 */
string getLogDirectory();

/*
 * Print error message to log file
 * @Params
 * errorNo: error number
 * info: error information
 */
void printErrorMsg(int errorNo, string info);

/*
 * Print warning message to log file
 * @Params
 * info: warning information
 */
void printWarningMsg(string info);

/*
 * Print debug message to log file
 * @Params
 * info: debug information
 */
void printDebugMsg(string info);

/*
 * Print instruction info to log file
 * @Params
 * inst: instruction to be printed
 */
void printInstruction(llvm::Instruction *inst);

/*
 * Close log file
 */
void closeLog();
}


#endif /* LOG_H_ */
