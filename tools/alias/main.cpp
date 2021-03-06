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
#include "../include/Anders.h"

using namespace llvm;

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>

#include "../lib/core/Executor.h"
#include "../lib/core/InterExecutor.h"

using namespace esp;
#define LOCK_OPERAND_OFFSET 0

namespace {
cl::opt<std::string> InputFile(cl::desc("<input bytecode>"), cl::Positional,
		cl::init("-"));
}

void parseArguments(int argc, char **argv) {
	std::vector<std::string> arguments;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--read-args") && i + 1 < argc) {
		} else {
			arguments.push_back(argv[i]);
		}
	}

	int numArgs = arguments.size() + 1;
	const char **argArray = new const char*[numArgs + 1];
	argArray[0] = argv[0];
	argArray[numArgs] = 0;
	for (int i = 1; i < numArgs; i++) {
		argArray[i] = arguments[i - 1].c_str();
	}

	cl::ParseCommandLineOptions(numArgs, const_cast<char**>(argArray),
			" klee\n");
	delete[] argArray;
}

int main(int argc, char ** argv) {
	// Load the bytecode...

	std::string ErrorMsg;
	Module *mainModule = 0;

	parseArguments(argc, argv);
	InterExecutor executor = InterExecutor("");

	string parentPath = InputFile;
	unsigned int index = parentPath.find_last_of('/', parentPath.length() - 1);
	if (index != string::npos) {
		initLog(parentPath.substr(0, index) + "/");
		executor = InterExecutor(parentPath.substr(index + 1));
	} else {
		initLog();
		executor = InterExecutor(parentPath);
	}

	std::cout << InputFile << std::endl;
	//An error is thrown by Eclipse here....
	MemoryBuffer *Buffer = MemoryBuffer::getFileOrSTDIN(InputFile, &ErrorMsg);

	if (Buffer) {
		mainModule = getLazyBitcodeModule(Buffer, getGlobalContext(),
				&ErrorMsg);
		if (!mainModule)
			delete Buffer;
	}

	if (mainModule) {
		if (mainModule->MaterializeAllPermanently(&ErrorMsg)) {
			delete mainModule;
			mainModule = 0;
		} else {
			// TODO: do something here

			aliasAnalysis * AA = new Andersens(mainModule);
			AA->runOnModule();


			errs() << "Commencing Module************" << "\n";

			std::list<std::list<Value*>*> aliasSets;
			std::list<CallInst *> CallList;

			for (Module::iterator I = mainModule->begin(), E = mainModule->end(); I != E; ++I) {
				std::set<CallInst*> lock_statements;

				Function &F = *I;
				std::string fname = F.getName();
					errs() << "Commencing Function************" << fname <<"\n";

				for (inst_iterator Inst_I = inst_begin(F), Inst_E = inst_end(F);
						Inst_I != Inst_E; ++Inst_I) {
					if (CallInst * callInst = dyn_cast<CallInst>(&*Inst_I)) {

						// test and record lock statements
						Function *called_F = callInst->getCalledFunction();
						if (called_F) {
							std::string called_fname = called_F->getName();

							if (called_fname.compare("pthread_mutex_lock") == 0
									|| called_fname.compare(
											"pthread_mutex_unlock") == 0) {
								Value *Operand = callInst->getOperand(
										LOCK_OPERAND_OFFSET);
								errs() << "In Function:	" << fname << "\n";
								if (!isa<PointerType>(Operand->getType()))
									continue;
								errs() << "Instruction:	" << *callInst << "\n";
								              errs() << "Operand: " << *Operand << "\n";
								CallList.push_back(callInst);
								lock_statements.insert(callInst);
								bool found = false;

								for (std::list<std::list<Value*>*>::iterator
										it = aliasSets.begin(), it_e =
												aliasSets.end(); it != it_e;
										++it) {
									std::list<Value*>::iterator itt =
											(*it)->begin();
									aliasAnalysis::aliasResult result = AA->alias(*itt, Operand);

									if (result == aliasAnalysis::MayAlias
											|| result
													== aliasAnalysis::MustAlias //|| result == AliasAnalysis::NoAlias
													) {
										(*it)->push_back(Operand);
										found = true;
										break;
									}

								}
								if (!found) {
									std::list<Value*> * newSet = new std::list<
											Value*>;
									newSet->push_back(Operand);
									aliasSets.push_back(newSet);
								}
							}
						}
					}

				}

			} //for alias set identification finished

			int num = 0;
			std::list<CallInst *>::iterator callIt = CallList.begin();
			for (std::list<std::list<Value*>*>::iterator it = aliasSets.begin(),
					it_e = aliasSets.end(); it != it_e; ++it) {

				errs() << "Alias Set " << num++ << "\n";
				for (std::list<Value*>::iterator itt = (**it).begin(), itt_e =
						(**it).end(); itt != itt_e; ++itt) {

					errs() << *(*itt) << "\n";
					errs() << "Instruction: " << **callIt << "\n";
					++callIt;
				}
			}

		}
	}

	closeLog();
}

