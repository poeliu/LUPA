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

#include "AliasAnalyzer.h"
#include <assert.h>
#include <iostream>
#include <fstream>

using namespace esp;

AliasResult AliasSet::isAlias(Value *va, Value *vb, Function *fa, Function *fb){
  bool inA = false, inB = false;
  for(set<AliasValue*>::iterator it = aliasValues.begin();
      it != aliasValues.end(); it++){
    if((*it)->value == va->getNameStr() &&
        (*it)->function == (fa != NULL ? fa->getNameStr() : ""))
      inA = true;
    else if((*it)->value == vb->getNameStr() &&
        (*it)->function == (fb != NULL ? fb->getNameStr() : ""))
      inB = true;

    if( inA && inB)
      return aliasType;
  }

  return NoResult;
}

#ifdef USE_ALIAS_FILE
void AliasSet::printAliasSet(){
  switch(aliasType){
  case MustAlias:
    cout<<"MustAlias set ";
    break;
  case MayAlias:
    cout<<"MayAlias set ";
    break;
  case NoAlias:
    cout<<"NoAlias set ";
    break;
  default:
    cout<<"Unknown type set" ;
    break;
  }

  cout<<aliasValues.size()<<endl;

  for(set<AliasValue*>::iterator it = aliasValues.begin();
      it != aliasValues.end(); it++){
    if((*it)->function != "" )
      cout<<(*it)->function<<" ";
    cout<<(*it)->value<<endl;
  }
}
#endif

AliasAnalyzer::AliasAnalyzer(){ allTheSame = true; }

AliasAnalyzer::AliasAnalyzer(bool isAllTheSame){ allTheSame = isAllTheSame; }

AliasAnalyzer::~AliasAnalyzer(){
#ifndef USE_ALIAS_FILE
  delete aa;
  aa = NULL;
#endif
}

#ifdef USE_ALIAS_FILE
void AliasAnalyzer::run(string filePath){
  if(allTheSame)
    return;
  assert(readResult(filePath) && "Cannot open or analyze the alias file");
}

#else
void AliasAnalyzer::run(Module *module){
  aa = new Andersens(module);
  aa->runOnModule();
}
#endif

AliasResult AliasAnalyzer::isAlias(Value *va, Value *vb, Function *fa, Function *fb){
#ifdef USE_ALIAS_FILE

  if(allTheSame)
    return MustAlias;

  for(set<AliasSet*>::iterator it = aliasSets.begin();
      it != aliasSets.end(); it++){
    AliasResult ar = (*it)->isAlias(va, vb, fa, fb);
    if(ar == MustAlias || ar == MayAlias)
      return ar;
  }

  return NoAlias;

#else
  if(va == NULL || vb == NULL )
    return NoResult;
  aliasAnalysis::aliasResult result = aa->alias(va, vb);
  switch(result){
  case 0:    return NoAlias;
  case 1:    return MayAlias;
  case 2:    return MustAlias;
  default:   return NoResult;
  }

#endif
}

#ifdef USE_ALIAS_FILE
bool AliasAnalyzer::readResult(string filePath){
  ifstream input;
  input.open(filePath.c_str());
  if(!input)
    return false;

  while(!input.eof()){
    string type;
    int setsNum;
    input>>type;
    input>>setsNum;

    for(int i = 0; i < setsNum; i++){
      string tmp;
      int valuesNum;
      input>>tmp;
      input>>valuesNum;

      AliasSet *aliasSet = new AliasSet();
      if(type == "must")
        aliasSet->aliasType = MustAlias;
      else if(type == "may")
        aliasSet->aliasType = MayAlias;
      else if(type == "no")
        aliasSet->aliasType = NoAlias;
      else{
        cout<<"fuck!"<<type<<endl;
        return false;
      }

      for(int j = 0; j< valuesNum; j++){
        string first, second;
        input>>first;
        if(first.substr(0,1) == "@"){ // global variable
          AliasValue *aliasValue = new AliasValue();
          aliasValue->function = "";
          aliasValue->value     = first;
          aliasSet->aliasValues.insert(aliasValue);
        }else{
          input>>second;
          AliasValue *aliasValue = new AliasValue();
          aliasValue->function = first;
          aliasValue->value     = second;
          aliasSet->aliasValues.insert(aliasValue);
        }
      }

      this->aliasSets.insert(aliasSet);
    }
  }

  input.close();
  return true;
}

void AliasAnalyzer::printAliasSets(){
  for(set<AliasSet*>::iterator it = aliasSets.begin();
      it != aliasSets.end(); it++){
    (*it)->printAliasSet();
    cout<<endl;
  }
}
#endif
