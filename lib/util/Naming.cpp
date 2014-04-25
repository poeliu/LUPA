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

#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/Function.h"
#include "llvm/Constants.h"
#include "Naming.h"
#include "Log.h"

using namespace llvm;
using namespace esp;
using namespace std;

string parseBinaryOpName(Instruction *inst){
  string name = "";

  Value *v0 = inst->getOperand(0);
  Value *v1 = inst->getOperand(1);
  if (isa<ConstantInt > (v0)) {
      name += ((ConstantInt*) v0)->getValue().toString(10, false);
  } else if (isa<ConstantInt > (v1)) {
      name += ((ConstantInt*) v1)->getValue().toString(10, false);
  } else {
    //FIXME: need to handle variable operand
  }
  return name;
}

string esp::parseName(Value *value){
  // has existed
  if(names.find(value) != names.end())
    return names[value];

  string name = "";
  Value *current = value;

  /*
  bool continueFlag = true;
  do{
    if(isa<Instruction > (current)){
      Instruction* inst = dyn_cast<Instruction>(current);
      unsigned op = inst->getOpcode();
      switch(op){
      case Instruction::Ret :{
        break;
      }

      case Instruction::Br :{
        break;
      }

      case Instruction::Switch :{
        break;
      }

      case Instruction::Call :{
        CallInst *callinst = (CallInst*) current;

        if (((CallInst*) current)->getCalledFunction() != NULL) {
            name += string("@")+((CallInst*) current)->getCalledFunction()->getNameStr() + "(";
        } else {
            name += string("@[funcPTR](");
            name += ((CallInst*) current)->getCalledValue()->getNameStr();
        }

        for (unsigned i = 1; i < callinst->getNumOperands(); i++) {
            name += esp::parseName(callinst->getOperand(i));
        }

        name += string(")");
        continueFlag = false;
        break;
      }

      case Instruction::PHI :{
        name += string("PHI[");
        name += current->getNameStr();
        PHINode *phi = (PHINode*) current;
        for (unsigned i = 0; i < phi->getNumIncomingValues(); i++) {
            Value *incoming = phi->getIncomingValue(i);
            if (i != 0) name += ",";
            if (!hasLoop(incoming)) {
                if (!incoming->hasName()) {
                    name += esp::parseName(incoming);
                } else {
                    name += incoming->getNameStr();
                }

            }
        }

        name += std::string("]");
        continueFlag = false;
        break;
      }

      case Instruction::Select :{
        break;
      }

      case Instruction::Add :{
        name += "+";
        name += parseBinaryOpName(inst);
        break;
      }

      case Instruction::Sub :{
        name += "-";
        name += parseBinaryOpName(inst);
        break;
      }

      case Instruction::Mul :{
        name += "*";
        name += parseBinaryOpName(inst);
        break;
      }

      case Instruction::UDiv :{
        name += "/";
        name += parseBinaryOpName(inst);
        break;
      }

      case Instruction::SDiv :{
        name += "//";
        name += parseBinaryOpName(inst);
        break;
      }

      case Instruction::And :{
        name += "&";
        name += parseBinaryOpName(inst);
        break;
      }

      case Instruction::Or :{
        name += "|";
        name += parseBinaryOpName(inst);
        break;
      }

      case Instruction::Xor :{
        name += "^";
        name += parseBinaryOpName(inst);
        break;
      }

      case Instruction::Shl :{
        name += "<<";
        name += parseBinaryOpName(inst);
        break;
      }

      case Instruction::LShr :{
        name += ">>";
        name += parseBinaryOpName(inst);
        break;
      }

      case Instruction::AShr :{
        name += ">>>";
        name += parseBinaryOpName(inst);
        break;
      }

      case Instruction::ICmp :{
        ICmpInst * icmp = dyn_cast<ICmpInst>(current);
        if (isa<Constant>(icmp->getOperand(0))) {
          name += esp::parseName(icmp->getOperand(1));
          continueFlag = false;
        } else {
          name += esp::parseName(icmp->getOperand(0));
          continueFlag = false;
        }
        break;
      }

      case Instruction::Alloca :{
        name += current->getNameStr();
        break;
      }

      case Instruction::Load :{
        if (((LoadInst*) inst)->isVolatile())
          name += std::string("@VolatileLoad");
        name += "*";
        name += esp::parseName(inst->getOperand(0));
        continueFlag = false;
        break;
      }

      case Instruction::Store :{
        // need to handle
        continueFlag = false;
        break;
      }

      case Instruction::GetElementPtr :{
        GetElementPtrInst * gep = dyn_cast<GetElementPtrInst>(current);
        unsigned ops = gep->getNumOperands();
        name += "[";
        for (unsigned i = 1; i < ops; i++) {
          Value *v = gep->getOperand(i);
          if (ConstantInt * ci = dyn_cast<ConstantInt>(v)) {
            if (i == 1 && ci->equalsInt(0))
              continue;
            name += ".";
            name += ci->getValue().toString(10, false);
          } else {
            name += ".";
            name += esp::parseName(v);
          }
        }
        name += "]";
        name += esp::parseName(gep->getOperand(0));
        continueFlag = false;
        break;
      }

      case Instruction::BitCast:{
        name += esp::parseName(inst->getOperand(0));
        continueFlag = false;
        break;
      }

      default :{
        // Illegal or unsupported instruction
        name += current->getNameStr();
        break;
      }
      }

    }else if(isa<Argument>(current)){
      if (arguments.find(current) != arguments.end())
        name += std::string("$") + current->getNameStr();

    }else if(isa<GlobalValue>(current)){
      name += std::string("@") + current->getNameStr();

    }else if(isa<ConstantInt>(current)){
      ConstantInt * cint = dyn_cast<ConstantInt > (current);
      name += cint->getValue().toString(10, true);

    }else if (isa<Constant > (current)) {
      Constant *c = dyn_cast<Constant > (current);
      if (c->isNullValue()) {
          name += "null";
      }
  }else{
      // Illegal format
  }
  if(!continueFlag)
    break;
  current = parents[current];
  }while(current);
  */

  //Refactor

   do {
     if (isa<LoadInst > (current)) {
       name += "*";
       if (parents[current] == NULL)
         name += (((LoadInst*) current)->getOperand(0))->getNameStr();
       if (((LoadInst*) current)->isVolatile())
         name += std::string("@VolatileLoad");

     } else if (dyn_cast<GetElementPtrInst > (current)) {
       GetElementPtrInst * gep = dyn_cast<GetElementPtrInst > (current);
       unsigned ops = gep->getNumOperands();
       name += "[";
       for (unsigned i = 1; i < ops; i++) {
         Value *v = gep->getOperand(i);
         if (dyn_cast<ConstantInt > (current)) {
           ConstantInt * ci = dyn_cast<ConstantInt > (current);
           if (i == 1 && ci->equalsInt(0)) continue;
           name += ".";
           name += ci->getValue().toString(10, false);
         } else {
           name += ".";
           name += parseName(v);
         }

       }
       name += "]";
       name += parseName(gep->getOperand(0));
       break;

     } else if (isa<AllocaInst > (current)) {
       name += current->getNameStr();
     } else if (isa<Argument > (current)) {
       if (arguments.find(current) != arguments.end())
         name += std::string("$") + current->getNameStr();
     } else if (isa<GlobalValue > (current)) {
       name += std::string("@") + current->getNameStr();
     } else if (isa<CallInst > (current)) {
       CallInst *callinst = (CallInst*) current;

       if (((CallInst*) current)->getCalledFunction() != NULL) {
         name += std::string("@")+((CallInst*) current)->getCalledFunction()->getNameStr() + "(";
       } else {
         name += std::string("@[funcPTR](");
         name += ((CallInst*) current)->getCalledValue()->getNameStr();
       }

       for (unsigned i = 1; i < callinst->getNumOperands(); i++) {
         name += parseName(callinst->getOperand(i));
       }

       name += std::string(")");
       break;
     } else if (isa<CastInst > (current)) {
     } else if (isa<PHINode > (current)) {
       /*
       name += std::string("PHI[");
       s += parent->getNameStr();
       PHINode *phi = (PHINode*) parent;
       for (unsigned i = 0; i < phi->getNumIncomingValues(); i++) {
         //s+=phi->getIncomingBlock(i)->getNameStr();
         Value *incoming = phi->getIncomingValue(i);
         if (i != 0) s += ",";
         if (!hasLoop(incoming)) {
           DEBUG(errs() << "incoming#" << i << " no loop(i rather doubt it)\n");
           if (!incoming->hasName()) {
             s += parseName(incoming);
           } else {
             s += incoming->getNameStr();
           }

         }
       }

       // PHI nodes...ugh
       s += std::string("]");
       break;
       */

     } else if (isa<BinaryOperator > (current)) {
       BinaryOperator *bo = dyn_cast<BinaryOperator > (current);
       Instruction::BinaryOps opcode = bo->getOpcode();
       if (opcode == Instruction::Add) {
         name.append("+");
       } else if (opcode == Instruction::Sub) {
         name.append("-");
       } else if (opcode == Instruction::Or) {
         name.append("||");
       } else if (opcode == Instruction::Mul) {
         name.append("*");
       } else if (opcode == Instruction::Xor) {
         name.append("^");
       } else if (opcode == Instruction::And) {
         name.append("&&");
       } else if (opcode == Instruction::Shl) {
         name.append("<<");
       } else if (opcode == Instruction::AShr) {
         name.append(">>");
       } else if (opcode == Instruction::LShr) {
         name.append(">>>");
       }
       Value *v0 = bo->getOperand(0);
       Value *v1 = bo->getOperand(1);
       if (isa<ConstantInt > (v0)) {
         name += ((ConstantInt*) v0)->getValue().toString(10, false);
       } else if (isa<ConstantInt > (v1)) {
         name += ((ConstantInt*) v1)->getValue().toString(10, false);
       } else {
         printDebugMsg("Binary Operation between non-constants\n");
       }
     } else if (dyn_cast<GEPOperator > (current)) {
       GEPOperator * gep = dyn_cast<GEPOperator > (current);
       unsigned ops = gep->getNumOperands();
       name += "[";
       for (unsigned i = 1; i < ops; i++) {
         Value *v = gep->getOperand(i);
         if (dyn_cast<ConstantInt > (v)) {
           ConstantInt * ci = dyn_cast<ConstantInt > (v);
           if (i == 1 && ci->equalsInt(0)) continue;
           name += ".";
           name += ci->getValue().toString(10, false);
         }

       }
       name += "]";

       name += parseName(gep->getOperand(0));
       break;

     } else if (dyn_cast<ICmpInst > (current)) {
       ICmpInst * icmp = dyn_cast<ICmpInst > (current);
       if (isa<Constant > (icmp->getOperand(0))) {
         name += parseName(icmp->getOperand(1));
         break;
       } else {
         name += parseName(icmp->getOperand(0));
         break;
       }
     } else if (dyn_cast<ConstantInt > (current)) {
       ConstantInt * cint = dyn_cast<ConstantInt > (current);
       name += cint->getValue().toString(10, true);
     } else {
       name += current->getNameStr(); // might not work
     }

   } while ((current = parents[current]));

  names[value] = name;

  return name;
}

