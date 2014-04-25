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

#include "Worklist.h"
#include "../util/Log.h"
#include "iostream"

namespace esp {

esp::Work::~Work(){

}

esp::Worklist::Worklist() {
  works = set<Work*>();
}

esp::Worklist::~Worklist() {
  for(set<Work*>::iterator it = works.begin(); it!=works.end(); it++){
    Work *work = (*it);
    works.erase(it);
  }
}

void esp::Worklist::addToWorklist(Work* work){
  if(work!=NULL){
    for(set<Work*>::iterator it = works.begin();
        it != works.end(); it++)
      if((*it)->equalsTo(work))
        return;
    works.insert(work);
  }
}

void esp::Worklist::removeFromWorklist(Work *work){
  string info = "worklist size: ";
  stringstream ss;
  ss<<works.size();
  info += ss.str();
  info += "\ncurrent work: ";
  stringstream s2;
  s2<<(int)work;
  info += s2.str();
  for(set<Work*>::iterator it = works.begin(); it!=works.end(); it++){
    Work *workInList = (*it);
    info += "\nenum work: ";
    stringstream s3;
    s3<<(int)work;
    info += s3.str();
    if(workInList == work){
      works.erase(it);
      break;
    }
  }

  //printDebugMsg(info);
}

Work* esp::Worklist::selectAWork(){
  // Defaultly return the first element in the work list
  return (*works.begin());
}

void esp::Worklist::solveWorklist(){
  while(works.size() != 0){
    Work *currentWork = selectAWork();
    removeFromWorklist(currentWork);
    //do real work here
    if(currentWork != NULL)
      doEachWork(currentWork);

    printDebugMsg("Instructions in worklist:");
    for(set<Work*>::iterator it = works.begin(); it!=works.end(); it++){
      Work *workInList = (*it);
      workInList->printContent();
    }
  }
}

void esp::Worklist::initWorklist(){
  works.clear();
}

} /* namespace esp */
