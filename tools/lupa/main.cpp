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

#include "llvm/Constants.h"
#include "llvm/Module.h"
#if (LLVM_VERSION_MAJOR == 2 && LLVM_VERSION_MINOR < 7)
#include "llvm/ModuleProvider.h"
#endif
//#include "llvm/Type.h"
#include "llvm/InstrTypes.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#if !(LLVM_VERSION_MAJOR == 2 && LLVM_VERSION_MINOR < 7)
#include "llvm/LLVMContext.h"
#endif
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/Target/TargetSelect.h"
#include "llvm/System/Signals.h"

#include "llvm/Pass.h"
#include "llvm/Analysis/AliasAnalysis.h"

using namespace llvm;

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>

#include "../lib/core/Executor.h"
#include "../lib/core/InterExecutor.h"

using namespace esp;

namespace{
  cl::opt<std::string>
    InputFile(cl::desc("<input bytecode>"), cl::Positional, cl::init("-"));
}

void parseArguments(int argc, char **argv) {
  std::vector<std::string> arguments;

  for (int i=1; i<argc; i++) {
    if (!strcmp(argv[i],"--read-args") && i+1<argc) {
    } else {
      arguments.push_back(argv[i]);
    }
  }
    
  int numArgs = arguments.size() + 1;
  const char **argArray = new const char*[numArgs+1];
  argArray[0] = argv[0];
  argArray[numArgs] = 0;
  for (int i=1; i<numArgs; i++) {
    argArray[i] = arguments[i-1].c_str();
  }

  cl::ParseCommandLineOptions(numArgs, const_cast<char**>( argArray), " klee\n");
  delete[] argArray;
}


int 
main (int argc, char ** argv)
{
  // Load the bytecode...

  std::string ErrorMsg;
  Module *mainModule = 0;
  
  parseArguments(argc, argv);
  InterExecutor executor = InterExecutor("");

  string parentPath = InputFile;
  unsigned int index = parentPath.find_last_of('/', parentPath.length()-1);
  if(index != string::npos){
    initLog(parentPath.substr(0,index)+"/");
    executor = InterExecutor(parentPath.substr(index+1));
  }
  else{
    initLog();
    executor = InterExecutor(parentPath);
  }

  std::cout<<InputFile<<std::endl;
  //An error is thrown by Eclipse here....
  MemoryBuffer *Buffer = MemoryBuffer::getFileOrSTDIN(InputFile, &ErrorMsg);

  if (Buffer) {
    mainModule = getLazyBitcodeModule(Buffer, getGlobalContext(), &ErrorMsg);
    if (!mainModule) delete Buffer;
  }

  if (mainModule) {
    if (mainModule->MaterializeAllPermanently(&ErrorMsg)) {
      delete mainModule;
      mainModule = 0;
    }else{
      executor.run(mainModule);
    }
  }

  closeLog();
}

