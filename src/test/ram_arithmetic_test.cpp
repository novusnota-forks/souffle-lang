/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2020, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ram_arithmetic_test.cpp
 *
 * Tests arithmetic evaluation by the Interpreter.
 *
 ***********************************************************************/

#include "DebugReport.h"
#include "ErrorReport.h"
#include "InterpreterEngine.h"
#include "RamExpression.h"
#include "RamOperation.h"
#include "RamProgram.h"
#include "RamStatement.h"
#include "RamTranslationUnit.h"
#include "SymbolTable.h"

#include "test.h"

#include <map>
#include <string>
#include <vector>

namespace souffle::test {

/** Function to evaluate a single RamExpression. */
RamDomain evalExpression(std::unique_ptr<RamExpression> expression) {
    // Set up RamProgram and translation unit
    std::vector<std::unique_ptr<RamExpression>> returnValues;
    returnValues.emplace_back(std::move(expression));

    Global::config().set("jobs", "1");
    std::unique_ptr<RamStatement> query =
            std::make_unique<RamQuery>(std::make_unique<RamSubroutineReturnValue>(std::move(returnValues)));
    std::map<std::string, std::unique_ptr<RamStatement>> subs;
    subs.insert(std::make_pair("test", std::move(query)));
    std::vector<std::unique_ptr<RamRelation>> rels;

    std::unique_ptr<RamProgram> prog =
            std::make_unique<RamProgram>(std::move(rels), std::make_unique<RamSequence>(), std::move(subs));

    SymbolTable symTab;
    ErrorReport errReport;
    DebugReport debugReport;

    RamTranslationUnit translationUnit(std::move(prog), symTab, errReport, debugReport);

    // configure and execute interpreter
    std::unique_ptr<InterpreterEngine> interpreter = std::make_unique<InterpreterEngine>(translationUnit);

    std::string name("test");
    std::vector<RamDomain> ret;
    std::vector<bool> errs;

    interpreter->executeSubroutine(name, {}, ret, errs);

    return ret.at(0);
}

TEST(RamNumber, ArithmeticEvaluation) {
    RamDomain num = 42;
    std::unique_ptr<RamExpression> expression = std::make_unique<RamNumber>(num);
    RamDomain result = evalExpression(std::move(expression));
    EXPECT_EQ(result, num);
}

TEST(RamNumber, SimpleAdd) {
    std::vector<std::unique_ptr<RamExpression>> Args;
    Args.push_back(std::make_unique<RamNumber>(1));
    Args.push_back(std::make_unique<RamNumber>(1));

    std::unique_ptr<RamExpression> expression =
            std::make_unique<RamIntrinsicOperator>(FunctorOp::ADD, std::move(Args));

    RamDomain result = evalExpression(std::move(expression));

    EXPECT_EQ(result, 2);
}

TEST(RamNumber, Neg) {
    std::vector<std::unique_ptr<RamExpression>> Args;
    Args.push_back(std::make_unique<RamNumber>(1));

    std::unique_ptr<RamExpression> expression =
            std::make_unique<RamIntrinsicOperator>(FunctorOp::NEG, std::move(Args));

    RamDomain result = evalExpression(std::move(expression));

    EXPECT_EQ(result, -1);
}

TEST(RamNumber, FloatNeg) {
    std::vector<std::unique_ptr<RamExpression>> Args;
    Args.push_back(std::make_unique<RamNumber>(ramBitCast(static_cast<RamFloat>(1))));

    std::unique_ptr<RamExpression> expression =
            std::make_unique<RamIntrinsicOperator>(FunctorOp::FNEG, std::move(Args));

    RamDomain result = evalExpression(std::move(expression));

    EXPECT_EQ(ramBitCast<RamFloat>(result), -static_cast<RamFloat>(1));
}

TEST(RamNumber, FloatNeg2) {
    RamFloat arg = -0.27;
    std::vector<std::unique_ptr<RamExpression>> Args;
    Args.push_back(std::make_unique<RamNumber>(ramBitCast(arg)));

    std::unique_ptr<RamExpression> expression =
            std::make_unique<RamIntrinsicOperator>(FunctorOp::FNEG, std::move(Args));

    RamDomain result = evalExpression(std::move(expression));

    EXPECT_EQ(ramBitCast<RamFloat>(result), -arg);
}

}  // namespace souffle::test
