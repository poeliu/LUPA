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

#include "Log.h"

using namespace std;
using namespace esp;
using namespace llvm;

namespace esp{
//ofstream log;
//raw_fd_ostream log((const char*)"-", (string&)"", 0);
raw_fd_ostream *log;

string fileName = "analysis_log";
string directory = "";

bool opened = false;
}

string esp::getValueName(Value *value){
  if(value->hasName()){
    if(isa<GlobalValue>(value))
      return "@"+value->getNameStr();
    else
      return "%"+value->getNameStr();
  }

  if(isa<Instruction>(value)){
    Instruction *vi = dyn_cast<Instruction>(value);
    MySlotTracker SlotTable(vi->getParent()->getParent());
    return SlotTable.getVariableName(value);
  }

  if(isa<Constant>(value)){
    return "CONSTANT_VALUE";
  }

  return "UNKNOWN_NAME";
}

void esp::initLog(string parent){
  directory = parent;
  fileName = parent+fileName;
  //esp::log.open(fileName.c_str());
  string errorMsg = "";
  log = new raw_fd_ostream(fileName.c_str(), errorMsg);
  opened  = true;
}

string esp::getLogDirectory(){
  return directory;
}

void esp::printErrorMsg(int errorNo, string info){
  if(!opened)
    return;
  switch(errorNo){
  case DOUBLE_LOCK :
    (*esp::log)<<"Error: double lock";
    break;
  case DOUBLE_UNLOCK :
    (*esp::log)<<"Error: double unlock";
    break;
  case UNINIT_UNLOCK :
    (*esp::log)<<"Error: uninit unlock";
    break;
  case UNRELEASE_LOCK:
    (*esp::log)<<"Error: unreleased lock";
    break;
  default :
    break;
  }
  if(info != "")
    (*esp::log)<<'('<<info<<')'<<"\n";
  else
    (*esp::log)<<"\n";

  esp::log->flush();
}

void esp::printWarningMsg(string info){
  if(opened)
    (*esp::log)<<"Warning: "<<info<<"\n";

  esp::log->flush();
}

void esp::printDebugMsg(string info){
  if(opened)
    (*esp::log)<<info<<"\n";

  esp::log->flush();
}

void esp::printInstruction(llvm::Instruction *inst){
  (*esp::log)<<(*inst)<<"\n";

  esp::log->flush();
}

void esp::closeLog(){
  if(opened)
    (*esp::log).close();
  opened = false;
}


