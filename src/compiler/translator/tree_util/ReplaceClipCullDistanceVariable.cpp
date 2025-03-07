//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ReplaceClipCullDistanceVariable.cpp: Find any references to gl_ClipDistance or gl_CullDistance
// and replace it with ANGLEClipDistance or ANGLECullDistance.
//

#include "compiler/translator/tree_util/ReplaceClipCullDistanceVariable.h"

#include "common/bitset_utils.h"
#include "common/debug.h"
#include "common/utilities.h"
#include "compiler/translator/Compiler.h"
#include "compiler/translator/SymbolTable.h"
#include "compiler/translator/tree_util/BuiltIn.h"
#include "compiler/translator/tree_util/IntermNode_util.h"
#include "compiler/translator/tree_util/IntermTraverse.h"
#include "compiler/translator/tree_util/RunAtTheBeginningOfShader.h"
#include "compiler/translator/tree_util/RunAtTheEndOfShader.h"

namespace sh
{
namespace
{

using ClipCullDistanceIdxSet = angle::BitSet<32>;

typedef TIntermNode *AssignFunc(const unsigned int index,
                                TIntermSymbol *left,
                                TIntermSymbol *right,
                                const TIntermTyped *enableFlags);

template <typename Variable>
const Variable *FindVariable(const std::vector<Variable> &mVars, const ImmutableString &name)
{
    for (const Variable &var : mVars)
    {
        if (name == var.instanceName)
        {
            return &var;
        }
    }

    return nullptr;
}

// Traverse the tree and collect the redeclaration and all constant index references of
// gl_ClipDistance/gl_CullDistance
class GLClipCullDistanceReferenceTraverser : public TIntermTraverser
{
  public:
    GLClipCullDistanceReferenceTraverser(const TIntermSymbol **redeclaredSymOut,
                                         bool *nonConstIdxUsedOut,
                                         unsigned int *maxConstIdxOut,
                                         ClipCullDistanceIdxSet *constIndicesOut,
                                         const ImmutableString &targetStr)
        : TIntermTraverser(true, false, false),
          mRedeclaredSym(redeclaredSymOut),
          mUseNonConstClipCullDistanceIndex(nonConstIdxUsedOut),
          mMaxConstClipCullDistanceIndex(maxConstIdxOut),
          mConstClipCullDistanceIndices(constIndicesOut),
          mTargetStr(targetStr)
    {
        *mRedeclaredSym                    = nullptr;
        *mUseNonConstClipCullDistanceIndex = false;
        *mMaxConstClipCullDistanceIndex    = 0;
        mConstClipCullDistanceIndices->reset();
    }

    bool visitDeclaration(Visit visit, TIntermDeclaration *node) override
    {
        // If gl_ClipDistance/gl_CullDistance is redeclared, we need to collect its information
        const TIntermSequence &sequence = *(node->getSequence());

        if (sequence.size() != 1)
        {
            return true;
        }

        TIntermTyped *variable = sequence.front()->getAsTyped();
        if (!variable->getAsSymbolNode() || variable->getAsSymbolNode()->getName() != mTargetStr)
        {
            return true;
        }

        *mRedeclaredSym = variable->getAsSymbolNode();

        return true;
    }

    bool visitBinary(Visit visit, TIntermBinary *node) override
    {
        TOperator op = node->getOp();
        if (op != EOpIndexDirect && op != EOpIndexIndirect)
        {
            return true;
        }

        // gl_ClipDistance / gl_CullDistance
        TIntermTyped *left = node->getLeft()->getAsTyped();
        if (!left)
        {
            return true;
        }

        ASSERT(op == EOpIndexDirect || op == EOpIndexIndirect);

        TIntermSymbol *clipCullDistance = left->getAsSymbolNode();
        if (!clipCullDistance)
        {
            return true;
        }
        if (clipCullDistance->getName() != mTargetStr)
        {
            return true;
        }

        const TConstantUnion *constIdx = node->getRight()->getConstantValue();
        if (!constIdx)
        {
            *mUseNonConstClipCullDistanceIndex = true;
        }
        else
        {
            unsigned int idx = 0;
            switch (constIdx->getType())
            {
                case EbtInt:
                    idx = constIdx->getIConst();
                    break;
                case EbtUInt:
                    idx = constIdx->getUConst();
                    break;
                case EbtFloat:
                    idx = static_cast<unsigned int>(constIdx->getFConst());
                    break;
                case EbtBool:
                    idx = constIdx->getBConst() ? 1 : 0;
                    break;
                default:
                    UNREACHABLE();
                    break;
            }
            ASSERT(idx < mConstClipCullDistanceIndices->size());
            mConstClipCullDistanceIndices->set(idx);

            *mMaxConstClipCullDistanceIndex = std::max(*mMaxConstClipCullDistanceIndex, idx);
        }

        return true;
    }

  private:
    const TIntermSymbol **mRedeclaredSym;
    // Flag indicating whether there is at least one reference of gl_ClipDistance with non-constant
    // index
    bool *mUseNonConstClipCullDistanceIndex;
    // Max constant index that is used to reference gl_ClipDistance
    unsigned int *mMaxConstClipCullDistanceIndex;
    // List of constant index reference of gl_ClipDistance
    ClipCullDistanceIdxSet *mConstClipCullDistanceIndices;
    // String for gl_ClipDistance/gl_CullDistance
    const ImmutableString mTargetStr;
};

// Replace all symbolic occurrences of given variables except one symbol.
class ReplaceVariableExceptOneTraverser : public TIntermTraverser
{
  public:
    ReplaceVariableExceptOneTraverser(const TVariable *toBeReplaced,
                                      const TIntermTyped *replacement,
                                      const TIntermSymbol *exception)
        : TIntermTraverser(true, false, false),
          mToBeReplaced(toBeReplaced),
          mException(exception),
          mReplacement(replacement)
    {}

    void visitSymbol(TIntermSymbol *node) override
    {
        if (&node->variable() == mToBeReplaced && node != mException)
        {
            queueReplacement(mReplacement->deepCopy(), OriginalNode::IS_DROPPED);
        }
    }

  private:
    const TVariable *const mToBeReplaced;
    const TIntermSymbol *const mException;
    const TIntermTyped *const mReplacement;
};

TIntermNode *simpleAssignFunc(const unsigned int index,
                              TIntermSymbol *leftSymbol,
                              TIntermSymbol *rightSymbol,
                              const TIntermTyped * /*enableFlags*/)
{
    // leftSymbol[index] = rightSymbol[index]
    // E.g., ANGLEClipDistance[index] = gl_ClipDistance[index]
    TIntermBinary *left =
        new TIntermBinary(EOpIndexDirect, leftSymbol->deepCopy(), CreateIndexNode(index));
    TIntermBinary *right =
        new TIntermBinary(EOpIndexDirect, rightSymbol->deepCopy(), CreateIndexNode(index));

    return new TIntermBinary(EOpAssign, left, right);
}

// This is only used for gl_Clipdistance
TIntermNode *assignFuncWithEnableFlags(const unsigned int index,
                                       TIntermSymbol *leftSymbol,
                                       TIntermSymbol *rightSymbol,
                                       const TIntermTyped *enableFlags)
{
    //  if (ANGLEUniforms.clipDistancesEnabled & (0x1 << index))
    //      gl_ClipDistance[index] = ANGLEClipDistance[index];
    //  else
    //      gl_ClipDistance[index] = 0;
    TIntermConstantUnion *bitMask = CreateUIntNode(0x1 << index);
    TIntermBinary *bitwiseAnd = new TIntermBinary(EOpBitwiseAnd, enableFlags->deepCopy(), bitMask);
    TIntermBinary *nonZero    = new TIntermBinary(EOpNotEqual, bitwiseAnd, CreateUIntNode(0));

    TIntermBinary *left =
        new TIntermBinary(EOpIndexDirect, leftSymbol->deepCopy(), CreateIndexNode(index));
    TIntermBinary *right =
        new TIntermBinary(EOpIndexDirect, rightSymbol->deepCopy(), CreateIndexNode(index));
    TIntermBinary *assignment = new TIntermBinary(EOpAssign, left, right);
    TIntermBlock *trueBlock   = new TIntermBlock();
    trueBlock->appendStatement(assignment);

    TIntermBinary *zeroAssignment =
        new TIntermBinary(EOpAssign, left->deepCopy(), CreateFloatNode(0));
    TIntermBlock *falseBlock = new TIntermBlock();
    falseBlock->appendStatement(zeroAssignment);

    return new TIntermIfElse(nonZero, trueBlock, falseBlock);
}

class ReplaceClipCullDistanceAssignments : angle::NonCopyable
{
  public:
    ReplaceClipCullDistanceAssignments(TCompiler *compiler,
                                       TIntermBlock *root,
                                       TSymbolTable *symbolTable,
                                       const TVariable *glClipCullDistanceVar,
                                       const TIntermSymbol *redeclaredGlClipDistance,
                                       const ImmutableString &angleVarName)
        : mCompiler(compiler),
          mRoot(root),
          mSymbolTable(symbolTable),
          mGlVar(glClipCullDistanceVar),
          mRedeclaredGLVar(redeclaredGlClipDistance),
          mANGLEVarName(angleVarName)
    {
        mEnabledDistances = 0;
    }

    unsigned int getEnabledClipCullDistance(const bool useNonConstIndex,
                                            const unsigned int maxConstIndex);
    const TVariable *declareANGLEVariable();
    bool assignOriginalValueToANGLEVariable(const GLenum shaderType);
    bool assignANGLEValueToOriginalVariable(const GLenum shaderType,
                                            const bool isRedeclared,
                                            const TIntermTyped *enableFlags,
                                            const ClipCullDistanceIdxSet *constIndices,
                                            AssignFunc assignFunc);

  private:
    bool assignOriginalValueToANGLEVariableImpl();
    bool assignANGLEValueToOriginalVariableImpl(const bool isRedeclared,
                                                const TIntermTyped *enableFlags,
                                                const ClipCullDistanceIdxSet *constIndices,
                                                AssignFunc assignFunc);

    // Common variables for replacing gl_Clip/CullDistances with ANGLEClip/CullDistances
    TCompiler *mCompiler;
    TIntermBlock *mRoot;
    TSymbolTable *mSymbolTable;

    const TVariable *mGlVar;
    const TIntermSymbol *mRedeclaredGLVar;
    const ImmutableString mANGLEVarName;

    unsigned int mEnabledDistances;
    const TVariable *mANGLEVar;
};

unsigned int ReplaceClipCullDistanceAssignments::getEnabledClipCullDistance(
    const bool useNonConstIndex,
    const unsigned int maxConstIndex)
{
    if (mRedeclaredGLVar)
    {
        // If array is redeclared by user, use that redeclared size.
        mEnabledDistances = mRedeclaredGLVar->getType().getOutermostArraySize();
    }
    else if (!useNonConstIndex)
    {
        ASSERT(maxConstIndex < mGlVar->getType().getOutermostArraySize());
        // Only use constant index, then use max array index used.
        mEnabledDistances = maxConstIndex + 1;
    }

    return mEnabledDistances;
}

const TVariable *ReplaceClipCullDistanceAssignments::declareANGLEVariable()
{
    ASSERT(mEnabledDistances > 0);

    TType *clipCullDistanceType = new TType(EbtFloat, EbpMedium, EvqGlobal, 1);
    clipCullDistanceType->makeArray(mEnabledDistances);

    mANGLEVar =
        new TVariable(mSymbolTable, mANGLEVarName, clipCullDistanceType, SymbolType::AngleInternal);

    TIntermSymbol *clipCullDistanceDeclarator = new TIntermSymbol(mANGLEVar);
    TIntermDeclaration *clipCullDistanceDecl  = new TIntermDeclaration;
    clipCullDistanceDecl->appendDeclarator(clipCullDistanceDeclarator);

    // Must declare ANGLEClipdistance/ANGLECullDistance before any function, since
    // gl_ClipDistance/gl_CullDistance might be accessed within a function declared before main.
    mRoot->insertStatement(0, clipCullDistanceDecl);

    return mANGLEVar;
}

bool ReplaceClipCullDistanceAssignments::assignOriginalValueToANGLEVariableImpl()
{
    ASSERT(mEnabledDistances > 0);

    TIntermBlock *readBlock                 = new TIntermBlock;
    TIntermSymbol *glClipCullDistanceSymbol = new TIntermSymbol(mGlVar);
    TIntermSymbol *clipCullDistanceSymbol   = new TIntermSymbol(mANGLEVar);

    for (unsigned int i = 0; i < mEnabledDistances; i++)
    {
        readBlock->appendStatement(
            simpleAssignFunc(i, glClipCullDistanceSymbol, clipCullDistanceSymbol, nullptr));
    }

    return RunAtTheBeginningOfShader(mCompiler, mRoot, readBlock);
}

bool ReplaceClipCullDistanceAssignments::assignANGLEValueToOriginalVariableImpl(
    const bool isRedeclared,
    const TIntermTyped *enableFlags,
    const ClipCullDistanceIdxSet *constIndices,
    AssignFunc assignFunc)
{
    ASSERT(mEnabledDistances > 0);

    TIntermBlock *assignBlock               = new TIntermBlock;
    TIntermSymbol *glClipCullDistanceSymbol = new TIntermSymbol(mGlVar);
    TIntermSymbol *clipCullDistanceSymbol   = new TIntermSymbol(mANGLEVar);

    // The array size is decided by either redeclaring the variable or accessing the variable with a
    // integral constant index. And this size is the count of the enabled value. So, if the index
    // which is greater than the array size, is used to access the variable, this access will be
    // ignored.
    if (isRedeclared || !constIndices)
    {
        for (unsigned int i = 0; i < mEnabledDistances; ++i)
        {
            assignBlock->appendStatement(
                assignFunc(i, glClipCullDistanceSymbol, clipCullDistanceSymbol, enableFlags));
        }
    }
    else
    {
        // Assign ANGLEClip/CullDistance[i]'s value to gl_Clip/CullDistance[i] if i is in the
        // constant indices list. Those elements whose index is not in the constant index list will
        // be zeroise for initialization.
        for (unsigned int i = 0; i < mEnabledDistances; ++i)
        {
            if (constIndices->test(i))
            {
                assignBlock->appendStatement(
                    assignFunc(i, glClipCullDistanceSymbol, clipCullDistanceSymbol, enableFlags));
            }
            else
            {
                // gl_Clip/CullDistance[i] = 0;
                TIntermBinary *left = new TIntermBinary(
                    EOpIndexDirect, glClipCullDistanceSymbol->deepCopy(), CreateIndexNode(i));
                TIntermBinary *zeroAssignment =
                    new TIntermBinary(EOpAssign, left, CreateFloatNode(0));
                assignBlock->appendStatement(zeroAssignment);
            }
        }
    }

    return RunAtTheEndOfShader(mCompiler, mRoot, assignBlock, mSymbolTable);
}

ANGLE_NO_DISCARD bool ReplaceClipCullDistanceAssignments::assignOriginalValueToANGLEVariable(
    const GLenum shaderType)
{
    switch (shaderType)
    {
        case GL_VERTEX_SHADER:
            // Vertex shader can use gl_Clip/CullDistance as a output only
            break;
        case GL_FRAGMENT_SHADER:
        {
            // These shader types can use gl_Clip/CullDistance as input
            if (!assignOriginalValueToANGLEVariableImpl())
            {
                return false;
            }
            break;
        }
        default:
        {
            UNREACHABLE();
            return false;
        }
    }

    return true;
}

ANGLE_NO_DISCARD bool ReplaceClipCullDistanceAssignments::assignANGLEValueToOriginalVariable(
    const GLenum shaderType,
    const bool isRedeclared,
    const TIntermTyped *enableFlags,
    const ClipCullDistanceIdxSet *constIndices,
    AssignFunc assignFunc)
{
    switch (shaderType)
    {
        case GL_VERTEX_SHADER:
        {
            // Vertex shader can use gl_Clip/CullDistance as output.
            // If the enabled gl_Clip/CullDistances are not initialized, results are undefined.
            // EXT_clip_cull_distance spec :
            // The shader must also set all values in gl_ClipDistance that have been enabled via the
            // OpenGL ES API, or results are undefined. Values written into gl_ClipDistance for
            // planes that are not enabled have no effect.
            // ...
            // Shaders writing gl_CullDistance must write all enabled distances, or culling results
            // are undefined.
            if (!assignANGLEValueToOriginalVariableImpl(isRedeclared, enableFlags, constIndices,
                                                        assignFuncWithEnableFlags))
            {
                return false;
            }
            break;
        }
        case GL_FRAGMENT_SHADER:
            // Fragment shader can use gl_Clip/CullDistance as input only
            break;
        default:
        {
            UNREACHABLE();
            return false;
        }
    }

    return true;
}

}  // anonymous namespace

ANGLE_NO_DISCARD bool ReplaceClipDistanceAssignments(TCompiler *compiler,
                                                     TIntermBlock *root,
                                                     TSymbolTable *symbolTable,
                                                     const GLenum shaderType,
                                                     const TIntermTyped *clipDistanceEnableFlags)
{
    // Collect all constant index references of gl_ClipDistance
    ImmutableString glClipDistanceName("gl_ClipDistance");
    ClipCullDistanceIdxSet constIndices;
    bool useNonConstIndex                         = false;
    const TIntermSymbol *redeclaredGlClipDistance = nullptr;
    unsigned int maxConstIndex                    = 0;
    GLClipCullDistanceReferenceTraverser indexTraverser(&redeclaredGlClipDistance,
                                                        &useNonConstIndex, &maxConstIndex,
                                                        &constIndices, glClipDistanceName);
    root->traverse(&indexTraverser);
    if (!useNonConstIndex && constIndices.none())
    {
        // No references of gl_ClipDistance
        return true;
    }

    // Retrieve gl_ClipDistance variable reference
    // Search user redeclared gl_ClipDistance first
    const TVariable *glClipDistanceVar = nullptr;
    if (redeclaredGlClipDistance)
    {
        glClipDistanceVar = &redeclaredGlClipDistance->variable();
    }
    else
    {
        // User defined not found, find in built-in table
        glClipDistanceVar = static_cast<const TVariable *>(
            symbolTable->findBuiltIn(glClipDistanceName, compiler->getShaderVersion()));
    }
    if (!glClipDistanceVar)
    {
        return false;
    }

    ReplaceClipCullDistanceAssignments replacementUtils(compiler, root, symbolTable,
                                                        glClipDistanceVar, redeclaredGlClipDistance,
                                                        ImmutableString("ANGLEClipDistance"));

    // Declare a global variable substituting gl_ClipDistance
    unsigned int enabledClipDistances =
        replacementUtils.getEnabledClipCullDistance(useNonConstIndex, maxConstIndex);
    if (!enabledClipDistances)
    {
        // Spec :
        // The gl_ClipDistance array is predeclared as unsized and must be explicitly sized by the
        // shader either redeclaring it with a size or implicitly sized by indexing it only with
        // integral constant expressions.
        return false;
    }

    const TVariable *clipDistanceVar = replacementUtils.declareANGLEVariable();

    // Replace gl_ClipDistance reference with ANGLEClipDistance, except the declaration
    ReplaceVariableExceptOneTraverser replaceTraverser(glClipDistanceVar,
                                                       new TIntermSymbol(clipDistanceVar),
                                                       /** exception */ redeclaredGlClipDistance);
    root->traverse(&replaceTraverser);
    if (!replaceTraverser.updateTree(compiler, root))
    {
        return false;
    }

    // Read gl_ClipDistance to ANGLEClipDistance for getting a original data
    if (!replacementUtils.assignOriginalValueToANGLEVariable(shaderType))
    {
        return false;
    }

    // Reassign ANGLEClipDistance to gl_ClipDistance but ignore those that are disabled
    const bool isRedeclared = (redeclaredGlClipDistance != nullptr);
    if (!replacementUtils.assignANGLEValueToOriginalVariable(shaderType, isRedeclared,
                                                             clipDistanceEnableFlags, &constIndices,
                                                             assignFuncWithEnableFlags))
    {
        return false;
    }

    return true;
}

ANGLE_NO_DISCARD bool ReplaceCullDistanceAssignments(TCompiler *compiler,
                                                     TIntermBlock *root,
                                                     TSymbolTable *symbolTable,
                                                     const GLenum shaderType)
{
    // Collect all constant index references of gl_CullDistance
    ImmutableString glCullDistanceName("gl_CullDistance");
    ClipCullDistanceIdxSet constIndices;
    bool useNonConstIndex                         = false;
    const TIntermSymbol *redeclaredGLCullDistance = nullptr;
    unsigned int maxConstIndex                    = 0;
    GLClipCullDistanceReferenceTraverser indexTraverser(&redeclaredGLCullDistance,
                                                        &useNonConstIndex, &maxConstIndex,
                                                        &constIndices, glCullDistanceName);
    root->traverse(&indexTraverser);
    if (!useNonConstIndex)
    {
        // Nothing to do
        return true;
    }

    // Retrieve gl_CullDistance variable reference
    // Search user redeclared gl_CullDistance first
    const TVariable *glCullDistanceVar = nullptr;
    if (redeclaredGLCullDistance)
    {
        glCullDistanceVar = &redeclaredGLCullDistance->variable();
    }
    else
    {
        // User defined not found, find in built-in table
        glCullDistanceVar = static_cast<const TVariable *>(
            symbolTable->findBuiltIn(glCullDistanceName, compiler->getShaderVersion()));
    }
    if (!glCullDistanceVar)
    {
        return false;
    }

    ReplaceClipCullDistanceAssignments replacementUtils(compiler, root, symbolTable,
                                                        glCullDistanceVar, redeclaredGLCullDistance,
                                                        ImmutableString("ANGLECullDistance"));

    // Declare a global variable substituting gl_CullDistance
    unsigned int enabledCullDistances =
        replacementUtils.getEnabledClipCullDistance(useNonConstIndex, maxConstIndex);
    if (!enabledCullDistances)
    {
        // Spec :
        // The gl_CullDistance array is predeclared as unsized and must be sized by the shader
        // either redeclaring it with a size or indexing it only with integral constant expressions.
        return false;
    }

    const TVariable *cullDistanceVar = replacementUtils.declareANGLEVariable();

    // Replace gl_CullDistance reference with ANGLECullDistance, except the declaration
    ReplaceVariableExceptOneTraverser replaceTraverser(glCullDistanceVar,
                                                       new TIntermSymbol(cullDistanceVar),
                                                       /** exception */ redeclaredGLCullDistance);
    root->traverse(&replaceTraverser);
    if (!replaceTraverser.updateTree(compiler, root))
    {
        return false;
    }

    // Read gl_CullDistance to ANGLECullDistance for getting a original data
    if (!replacementUtils.assignOriginalValueToANGLEVariable(shaderType))
    {
        return false;
    }

    // Reassign ANGLECullDistance to gl_CullDistance but ignore those that are disabled
    const bool isRedeclared = (redeclaredGLCullDistance != nullptr);
    if (!replacementUtils.assignANGLEValueToOriginalVariable(shaderType, isRedeclared, nullptr,
                                                             &constIndices, simpleAssignFunc))
    {
        return false;
    }

    return true;
}

}  // namespace sh
