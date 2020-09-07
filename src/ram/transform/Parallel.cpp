/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2018, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file Parallel.cpp
 *
 ***********************************************************************/

#include "ram/transform/Parallel.h"
#include "ram/Condition.h"
#include "ram/Expression.h"
#include "ram/Node.h"
#include "ram/Operation.h"
#include "ram/Program.h"
#include "ram/Relation.h"
#include "ram/Statement.h"
#include "ram/Visitor.h"
#include "souffle/utility/MiscUtil.h"
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace souffle {

bool ParallelTransformer::parallelizeOperations(RamProgram& program) {
    bool changed = false;

    // parallelize the most outer loop only
    // most outer loops can be scan/choice/indexScan/indexChoice
    visitDepthFirst(program, [&](const RamQuery& query) {
        std::function<Own<RamNode>(Own<RamNode>)> parallelRewriter = [&](Own<RamNode> node) -> Own<RamNode> {
            if (const RamScan* scan = dynamic_cast<RamScan*>(node.get())) {
                if (scan->getTupleId() == 0 && scan->getRelation().getArity() > 0) {
                    if (!isA<RamProject>(&scan->getOperation())) {
                        changed = true;
                        return std::make_unique<RamParallelScan>(
                                std::make_unique<RamRelationReference>(&scan->getRelation()),
                                scan->getTupleId(), souffle::clone(&scan->getOperation()),
                                scan->getProfileText());
                    }
                }
            } else if (const RamChoice* choice = dynamic_cast<RamChoice*>(node.get())) {
                if (choice->getTupleId() == 0) {
                    changed = true;
                    return std::make_unique<RamParallelChoice>(
                            std::make_unique<RamRelationReference>(&choice->getRelation()),
                            choice->getTupleId(), souffle::clone(&choice->getCondition()),
                            souffle::clone(&choice->getOperation()), choice->getProfileText());
                }
            } else if (const RamIndexScan* indexScan = dynamic_cast<RamIndexScan*>(node.get())) {
                if (indexScan->getTupleId() == 0) {
                    changed = true;
                    const RamRelation& rel = indexScan->getRelation();
                    RamPattern queryPattern = clone(indexScan->getRangePattern());
                    return std::make_unique<RamParallelIndexScan>(
                            std::make_unique<RamRelationReference>(&rel), indexScan->getTupleId(),
                            std::move(queryPattern), souffle::clone(&indexScan->getOperation()),
                            indexScan->getProfileText());
                }
            } else if (const RamIndexChoice* indexChoice = dynamic_cast<RamIndexChoice*>(node.get())) {
                if (indexChoice->getTupleId() == 0) {
                    changed = true;
                    const RamRelation& rel = indexChoice->getRelation();
                    RamPattern queryPattern = clone(indexChoice->getRangePattern());
                    return std::make_unique<RamParallelIndexChoice>(
                            std::make_unique<RamRelationReference>(&rel), indexChoice->getTupleId(),
                            souffle::clone(&indexChoice->getCondition()), std::move(queryPattern),
                            souffle::clone(&indexChoice->getOperation()), indexChoice->getProfileText());
                }
            } else if (const RamAggregate* aggregate = dynamic_cast<RamAggregate*>(node.get())) {
                if (aggregate->getTupleId() == 0 && !aggregate->getRelation().isNullary()) {
                    changed = true;
                    const RamRelation& rel = aggregate->getRelation();
                    return std::make_unique<RamParallelAggregate>(
                            Own<RamOperation>(aggregate->getOperation().clone()), aggregate->getFunction(),
                            std::make_unique<RamRelationReference>(&rel),
                            Own<RamExpression>(aggregate->getExpression().clone()),
                            Own<RamCondition>(aggregate->getCondition().clone()), aggregate->getTupleId());
                }
            } else if (const RamIndexAggregate* indexAggregate =
                               dynamic_cast<RamIndexAggregate*>(node.get())) {
                if (indexAggregate->getTupleId() == 0 && !indexAggregate->getRelation().isNullary()) {
                    changed = true;
                    const RamRelation& rel = indexAggregate->getRelation();
                    RamPattern queryPattern = clone(indexAggregate->getRangePattern());
                    return std::make_unique<RamParallelIndexAggregate>(
                            Own<RamOperation>(indexAggregate->getOperation().clone()),
                            indexAggregate->getFunction(), std::make_unique<RamRelationReference>(&rel),
                            Own<RamExpression>(indexAggregate->getExpression().clone()),
                            Own<RamCondition>(indexAggregate->getCondition().clone()),
                            std::move(queryPattern), indexAggregate->getTupleId());
                }
            }
            node->apply(makeLambdaRamMapper(parallelRewriter));
            return node;
        };
        const_cast<RamQuery*>(&query)->apply(makeLambdaRamMapper(parallelRewriter));
    });
    return changed;
}

}  // end of namespace souffle
