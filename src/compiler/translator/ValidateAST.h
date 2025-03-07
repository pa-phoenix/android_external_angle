//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TRANSLATOR_VALIDATEAST_H_
#define COMPILER_TRANSLATOR_VALIDATEAST_H_

#include "compiler/translator/BaseTypes.h"
#include "compiler/translator/Common.h"

namespace sh
{
class TDiagnostics;
class TIntermNode;

// The following options (stored in Compiler) tell the validator what to validate.  Some validations
// are conditional to certain passes.
struct ValidateASTOptions
{
    // TODO: add support for the flags marked with TODO. http://anglebug.com/2733

    // Check that every node always has only one parent,
    bool validateSingleParent = true;
    // Check that all symbols reference TVariables that have been declared.  For built-ins, this
    // makes sure that the same GLSL built-in uses the same TVariable consistently.
    bool validateVariableReferences = true;
    // Whether validateVariableReferences should also include specialization constants.  Their
    // declaration is output after their usage is discovered, so this is disabled until then.
    bool validateSpecConstReferences = false;
    // Check that TIntermUnary and TIntermAggregate nodes with a built-in op reference a function
    // with said op.
    bool validateBuiltInOps = true;
    // Check that all EOpCallFunctionInAST have their corresponding function definitions in the AST,
    // with matching symbol ids. There should also be at least a prototype declaration before the
    // function is called.
    bool validateFunctionCall = true;
    // Check that EOpCallInternalRawFunction is not used.  This OP is deprecated and needs to be
    // removed.  http://anglebug.com/6059
    bool validateNoRawFunctionCalls = true;
    // Check that there are no null nodes where they are not allowed, for example as children of
    // TIntermDeclaration or TIntermBlock.
    bool validateNullNodes = true;
    // Check that symbols that reference variables have consistent qualifiers and symbol ids with
    // the variable declaration.  The following needs to be validated:
    //
    // Implemented:
    //
    //  - Function parameters having one of EvqParam* qualifiers.
    //
    // TODO:
    //
    //  - Function-local variables must have the EvqTemporary qualifier.
    //  - Symbol references and declarations have identical qualifiers.
    bool validateQualifiers = true;
    // Check that variable declarations that can't have initializers don't have initializers
    // (varyings, uniforms for example).
    bool validateInitializers = true;  // TODO
    // Check that there is only one TFunction with each function name referenced in the nodes (no
    // two TFunctions with the same name, taking internal/non-internal namespaces into account).
    bool validateUniqueFunctions = true;  // TODO
    // Check that references to structs are matched with the corresponding struct declaration.  This
    // is only done for references to structs inside other struct or interface blocks declarations,
    // as validateVariableReferences already ensures other references to the struct match the
    // declaration.
    bool validateStructUsage = true;
    // Check that expression nodes have the correct type considering their operand(s).  The
    // following validation is possible:
    //
    // Implemented:
    //
    //  - Binary node that indexes T[] should have type T
    //
    // TODO:
    //
    //  - Function calls (including built-ins) have the same return type in the node and function.
    //  - Unary and binary operators have the correct type based on operands
    //  - Swizzle result has same type as the operand except for vector size
    //  - Ternary operator has the same type as the operands
    bool validateExpressionTypes = true;
    // If SeparateDeclarations has been run, check for the absence of multi declarations as well.
    bool validateMultiDeclarations = false;
};

// Check for errors and output error messages on the context.
// Returns true if there are no errors.
bool ValidateAST(TIntermNode *root, TDiagnostics *diagnostics, const ValidateASTOptions &options);

}  // namespace sh

#endif  // COMPILER_TRANSLATOR_VALIDATESWITCH_H_
