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

#ifndef EXPR_H_
#define EXPR_H_

#ifdef USE_SOLVER
#include "../stp/c_interface/c_interface.h"
#endif
#include <string>

using namespace std;

namespace esp{

class Expression{
public:
  string name;      /* name of the expression */
  unsigned width;   /* width of the expression.*/

#ifdef USE_SOLVER
  Expr expr;    //STP-style Expression

  Expression(string _name, unsigned _width, Expr _expr)
            :name(_name), width(_width), expr(_expr){};
#else
  bool value;       /* */

  /*
   * Constructor
   * @Param
   * _name: name of the boolean expression
   * _width:width of the boolean expression
   * _value:value of the boolean expression
   */
  Expression(string _name, unsigned _width, bool _value)
            :name(_name), width(_width), value(_value){};
#endif

};

}


#endif /* EXPR_H_ */
