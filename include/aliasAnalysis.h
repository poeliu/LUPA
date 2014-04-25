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

#ifndef ALIASANALYSIS_H_
#define ALIASANALYSIS_H_

#include "llvm/Module.h"
#include "llvm/Value.h"


using namespace llvm;

class aliasAnalysis {
protected:
	llvm::Module *program;

	// Previous Alias Analysis to chain to.
	aliasAnalysis *AA;

public:
	aliasAnalysis(Module *p, aliasAnalysis *a = 0): program(p),AA(a){};
	virtual ~aliasAnalysis();

	/// UnknownSize - This is a special value which can be used with the
	/// size arguments in alias queries to indicate that the caller does not
	/// know the sizes of the potential memory references.
	static unsigned const UnknownSize = ~0u;

	//===--------------------------------------------------------------------===//
	/// Alias Queries...
	///
    /// Alias analysis result - Either we know for sure that it does not alias, we
	/// know for sure it must alias, or we don't know anything: The two pointers
	/// _might_ alias.  This enum is designed so you can do things like:
	///     if (AA.alias(P1, P2)) { ... }
	/// to check to see if two pointers might alias.
	///
	/// See docs/AliasAnalysis.html for more information on the specific meanings
	/// of these values.
	///
	enum aliasResult { NoAlias = 0, MayAlias = 1, MustAlias = 2 };

	virtual void runOnModule();


	virtual aliasResult alias(const Value *V1, unsigned V1Size,
	                            const Value *V2, unsigned V2Size);

	/// alias - A convenience wrapper for the case where the sizes are unknown.
	aliasResult alias(const Value *V1, const Value *V2) {
	  return alias(V1, UnknownSize, V2, UnknownSize);
	}

};





#endif /* ALIASANALYSIS_H_ */
