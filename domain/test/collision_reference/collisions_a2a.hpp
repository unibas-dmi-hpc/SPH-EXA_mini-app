/*
 * MIT License
 *
 * Copyright (c) 2021 CSCS, ETH Zurich
 *               2021 University of Basel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*! @file
 * @brief Naive collision detection implementation for validation and testing
 *
 * @author Sebastian Keller <sebastian.f.keller@gmail.com>
 */

#pragma once

#include "cstone/halos/btreetraversal.hpp"
#include "cstone/tree/octree_util.hpp"

namespace cstone
{
/*! @brief to-all implementation of findCollisions
 *
 * @tparam I   32- or 64-bit unsigned integer
 * @param[in]  tree           octree leaf nodes in cornerstone format
 * @param[out] collisionList  output list of indices of colliding nodes
 * @param[in]  collisionBox   query box to look for collisions
 *                            with leaf nodes
 *
 * Naive implementation without tree traversal for reference
 * and testing purposes
 */
template <class I>
void findCollisions2All(const std::vector<I>& tree, CollisionList& collisionList,
                        const IBox& collisionBox)
{
    for (std::size_t nodeIndex = 0; nodeIndex < nNodes(tree); ++nodeIndex)
    {
        int prefixBits = treeLevel(tree[nodeIndex+1] - tree[nodeIndex]) * 3;
        if (overlap(tree[nodeIndex], prefixBits, collisionBox))
            collisionList.add((int)nodeIndex);
    }
}

//! @brief all-to-all implementation of findAllCollisions
template<class I, class T>
std::vector<CollisionList> findCollisionsAll2all(const std::vector<I>& tree, const std::vector<T>& haloRadii,
                                                 const Box<T>& globalBox)
{
    std::vector<CollisionList> collisions(tree.size() - 1);

    for (std::size_t leafIdx = 0; leafIdx < nNodes(tree); ++leafIdx)
    {
        T radius = haloRadii[leafIdx];

        IBox haloBox = makeHaloBox(tree[leafIdx], tree[leafIdx + 1], radius, globalBox);
        findCollisions2All(tree, collisions[leafIdx], haloBox);
    }

    return collisions;
}

} // namespace cstone