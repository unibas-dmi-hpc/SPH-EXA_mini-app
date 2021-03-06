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
 * @brief Random coordinates generation for testing
 *
 * @author Sebastian Keller <sebastian.f.keller@gmail.com>
 */


#pragma once

#include <algorithm>
#include <random>
#include <vector>

#include "cstone/sfc/morton.hpp"
#include "cstone/primitives/gather.hpp"

using namespace cstone;

template<class T, class I>
class RandomCoordinates
{
public:

    RandomCoordinates(unsigned n, Box<T> box, int seed = 42)
        : box_(std::move(box)), x_(n), y_(n), z_(n), codes_(n)
    {
        //std::random_device rd;
        std::mt19937 gen(seed);
        std::uniform_real_distribution<T> disX(box_.xmin(), box_.xmax());
        std::uniform_real_distribution<T> disY(box_.ymin(), box_.ymax());
        std::uniform_real_distribution<T> disZ(box_.zmin(), box_.zmax());

        auto randX = [&disX, &gen]() { return disX(gen); };
        auto randY = [&disY, &gen]() { return disY(gen); };
        auto randZ = [&disZ, &gen]() { return disZ(gen); };

        std::generate(begin(x_), end(x_), randX);
        std::generate(begin(y_), end(y_), randY);
        std::generate(begin(z_), end(z_), randZ);

        computeMortonCodes(begin(x_), end(x_), begin(y_), begin(z_),
                                   begin(codes_), box);

        std::vector<I> mortonOrder(n);
        std::iota(begin(mortonOrder), end(mortonOrder), 0);
        sort_by_key(begin(codes_), end(codes_), begin(mortonOrder));

        reorderInPlace(mortonOrder, x_.data());
        reorderInPlace(mortonOrder, y_.data());
        reorderInPlace(mortonOrder, z_.data());
    }

    const std::vector<T>& x() const { return x_; }
    const std::vector<T>& y() const { return y_; }
    const std::vector<T>& z() const { return z_; }
    const std::vector<I>& mortonCodes() const { return codes_; }

private:

    Box<T> box_;
    std::vector<T> x_, y_, z_;
    std::vector<I> codes_;
};

template<class T, class I>
class RandomGaussianCoordinates
{
public:

    RandomGaussianCoordinates(unsigned n, Box<T> box, int seed = 42)
        : box_(std::move(box)), x_(n), y_(n), z_(n), codes_(n)
    {
        //std::random_device rd;
        std::mt19937 gen(seed);
        // random gaussian distribution at the center
        std::normal_distribution<T> disX((box_.xmax() + box_.xmin())/2, (box_.xmax() - box_.xmin())/5);
        std::normal_distribution<T> disY((box_.ymax() + box_.ymin())/2, (box_.ymax() - box_.ymin())/5);
        std::normal_distribution<T> disZ((box_.zmax() + box_.zmin())/2, (box_.zmax() - box_.zmin())/5);

        auto randX = [cmin=box_.xmin(), cmax=box_.xmax(), &disX, &gen]() { return std::max(std::min(disX(gen), cmax), cmin); };
        auto randY = [cmin=box_.ymin(), cmax=box_.ymax(), &disY, &gen]() { return std::max(std::min(disY(gen), cmax), cmin); };
        auto randZ = [cmin=box_.zmin(), cmax=box_.zmax(), &disZ, &gen]() { return std::max(std::min(disZ(gen), cmax), cmin); };

        std::generate(begin(x_), end(x_), randX);
        std::generate(begin(y_), end(y_), randY);
        std::generate(begin(z_), end(z_), randZ);

        computeMortonCodes(begin(x_), end(x_), begin(y_), begin(z_),
                                   begin(codes_), box);

        std::vector<I> mortonOrder(n);
        std::iota(begin(mortonOrder), end(mortonOrder), 0);
        sort_by_key(begin(codes_), end(codes_), begin(mortonOrder));

        reorderInPlace(mortonOrder, x_.data());
        reorderInPlace(mortonOrder, y_.data());
        reorderInPlace(mortonOrder, z_.data());
    }

    const std::vector<T>& x() const { return x_; }
    const std::vector<T>& y() const { return y_; }
    const std::vector<T>& z() const { return z_; }
    const std::vector<I>& mortonCodes() const { return codes_; }

private:

    Box<T> box_;
    std::vector<T> x_, y_, z_;
    std::vector<I> codes_;
};
