/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2019, The Souffle Developers. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file LVMRelation.h
 *
 * Defines LVM Relations
 *
 ***********************************************************************/

#pragma once

#include "LVMIndex.h"
#include "RamIndexAnalysis.h"

namespace souffle {
/**
 * A relation, composed of a collection of indexes.
 */
class LVMRelation {
public:
    /**
     * Creates a relation, build all necessary indexes.
     */
    LVMRelation(std::size_t arity, std::size_t numberOfHeights, const std::string& name,
            const std::vector<std::string>& attributeTypes, const MinIndexSelection& orderSet,
            IndexFactory factory = &createBTreeIndex);

    LVMRelation(LVMRelation& other) = delete;

    virtual ~LVMRelation() = default;

    /**
     * Support for-each iteration for LVMRelation.
     */
    class Iterator : public std::iterator<std::forward_iterator_tag, RamDomain*> {
        std::unique_ptr<Stream> stream;

    public:
        Iterator() : stream(std::make_unique<Stream>()) {}

        Iterator(const LVMRelation& rel) : stream(std::make_unique<Stream>(rel.scan())) {}

        Iterator(const Iterator& iter) : stream(iter.stream->clone()) {}

        Iterator(Iterator&& iter) : stream(std::move(iter.stream)) {}

        Iterator& operator++() {
            ++stream->begin();
            return *this;
        }

        const RamDomain* operator*() {
            return (*stream->begin()).getBase();
        }

        bool operator!=(const Iterator& other) const {
            return stream->begin() != other.stream->begin();
        }

        bool operator==(const Iterator& other) const {
            return stream->begin() == other.stream->begin();
        }
    };

    Iterator begin() const {
        return *this;
    }

    Iterator end() const {
        return Iterator();
    }

    /**
     * Drops an index from the maintained indexes. All but one index
     * may be removed.
     */
    void removeIndex(const size_t& indexPos);

    /**
     * Add the given tuple to this relation.
     */
    virtual bool insert(const TupleRef& tuple);

    /**
     * Add the given tuple to this relation.
     */
    virtual bool insert(const RamDomain* tuple) {
        return insert(TupleRef(tuple, arity));
    }

    /**
     * Add all entries of the given relation to this relation.
     */
    void insert(const LVMRelation& other);

    /**
     * Tests whether this relation contains the given tuple.
     */
    bool contains(const TupleRef& tuple) const;

    /**
     * Obtains a stream to scan the entire relation.
     */
    Stream scan() const;

    /**
     * Obtains a stream covering the interval between the two given entries.
     */
    Stream range(const size_t& indexPos, const TupleRef& low, const TupleRef& high) const;

    /**
     * Swaps the content of this and the given relation, including the
     * installed indexes.
     */
    void swap(LVMRelation& other);

    /**
     * Set level
     */
    void setLevel(size_t level) {
        this->level = level;
    }

    /**
     * Return the level of the relation.
     */
    size_t getLevel() const;

    /**
     * Return the relation name.
     */
    const std::string& getName() const;

    /**
     * Return the attribute types
     */
    const std::vector<std::string>& getAttributeTypeQualifiers() const;

    /**
     * Return arity
     */
    size_t getArity() const;

    /**
     * Return arity
     */
    size_t getNumberOfHeights() const;

    /**
     * Return number of tuples in relation (full-order)
     */
    size_t size() const;

    /**
     * Check if the relation is empty
     */
    bool empty() const;

    /**
     * Clear all indexes
     */
    virtual void purge();

    /**
     * Check if a tuple exists in realtion
     */
    bool exists(const TupleRef& tuple) const;

    /**
     * Extend another relation
     */
    virtual void extend(const LVMRelation& rel);

protected:
    // Relation name
    std::string relName;

    // Relation Arity
    size_t arity;

    // Number of height parameters of relation
    size_t numberOfHeights;

    // Relation attributes types
    std::vector<std::string> attributeTypes;

    // a map of managed indexes
    std::vector<std::unique_ptr<LVMIndex>> indexes;

    // a pointer to the main index within the managed index
    LVMIndex* main;

    // relation level
    size_t level = 0;
};  // namespace souffle

/**
 * Interpreter Equivalence Relation
 */
class LVMEqRelation : public LVMRelation {
public:
    LVMEqRelation(size_t arity, size_t numberOfHeights, const std::string& relName,
            const std::vector<std::string>& attributeTypes, const MinIndexSelection& orderSet);

    /** Insert tuple */
    bool insert(const TupleRef& tuple) override;

    /** Find the new knowledge generated by inserting a tuple */
    std::vector<RamDomain*> extend(const TupleRef& tuple);

    /** Extend this relation with new knowledge generated by inserting all tuples from a relation */
    void extend(const LVMRelation& rel) override;
};

/**
 * Interpreter Indirect Relation
 */
class LVMIndirectRelation : public LVMRelation {
public:
    LVMIndirectRelation(size_t arity, size_t numberOfHeights, const std::string& relName,
            const std::vector<std::string>& attributeTypes, const MinIndexSelection& orderSet);
    /** Insert tuple */
    bool insert(const TupleRef& tuple) override;

    bool insert(const RamDomain* tuple) override;

    /** Clear all indexes */
    void purge() override;

private:
    /** Size of blocks containing tuples */
    static const int BLOCK_SIZE = 1024;

    std::deque<std::unique_ptr<RamDomain[]>> blockList;

    size_t num_tuples = 0;
};

}  // end of namespace souffle
