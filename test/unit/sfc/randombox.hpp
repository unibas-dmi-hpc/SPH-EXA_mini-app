#pragma once

#include <algorithm>
#include <random>
#include <vector>

#include "sfc/mortoncode.hpp"
#include "sfc/zorder.hpp"

template<class T, class I>
class RandomCoordinates
{
public:

    RandomCoordinates(unsigned n, sphexa::Box<T> box)
        : box_(std::move(box)), x_(n), y_(n), z_(n), codes_(n)
    {
        //std::random_device rd;
        std::mt19937 gen(42);
        std::uniform_real_distribution<T> disX(box_.xmin(), box_.xmax());
        std::uniform_real_distribution<T> disY(box_.ymin(), box_.ymax());
        std::uniform_real_distribution<T> disZ(box_.zmin(), box_.zmax());

        auto randX = [&disX, &gen]() { return disX(gen); };
        auto randY = [&disY, &gen]() { return disY(gen); };
        auto randZ = [&disZ, &gen]() { return disZ(gen); };

        std::generate(begin(x_), end(x_), randX);
        std::generate(begin(y_), end(y_), randY);
        std::generate(begin(z_), end(z_), randZ);

        sphexa::computeMortonCodes(begin(x_), end(x_), begin(y_), begin(z_),
                                   begin(codes_), box);

        std::vector<I> mortonOrder(n);
        sphexa::sort_invert(cbegin(codes_), cend(codes_), begin(mortonOrder));

        sphexa::reorder(mortonOrder, codes_);
        sphexa::reorder(mortonOrder, x_);
        sphexa::reorder(mortonOrder, y_);
        sphexa::reorder(mortonOrder, z_);
    }

    const std::vector<T>& x() const { return x_; }
    const std::vector<T>& y() const { return y_; }
    const std::vector<T>& z() const { return z_; }
    const std::vector<I>& mortonCodes() const { return codes_; }

private:

    sphexa::Box<T> box_;
    std::vector<T> x_, y_, z_;
    std::vector<I> codes_;
};

template<class T, class I>
class RandomGaussianCoordinates
{
public:

    RandomGaussianCoordinates(unsigned n, sphexa::Box<T> box)
        : box_(std::move(box)), x_(n), y_(n), z_(n), codes_(n)
    {
        //std::random_device rd;
        std::mt19937 gen(42);
        // random gaussian distribution at the center
        std::normal_distribution<T> disX((box_.xmax() - box_.xmin())/2, (box_.xmax() - box_.xmin())/5);
        std::normal_distribution<T> disY((box_.ymax() - box_.ymin())/2, (box_.ymax() - box_.ymin())/5);
        std::normal_distribution<T> disZ((box_.zmax() - box_.zmin())/2, (box_.zmax() - box_.zmin())/5);

        auto randX = [cmin=box_.xmin(), cmax=box_.xmax(), &disX, &gen]() { return std::max(std::min(disX(gen), cmax), cmin); };
        auto randY = [cmin=box_.ymin(), cmax=box_.ymax(), &disY, &gen]() { return std::max(std::min(disY(gen), cmax), cmin); };
        auto randZ = [cmin=box_.zmin(), cmax=box_.zmax(), &disZ, &gen]() { return std::max(std::min(disZ(gen), cmax), cmin); };

        std::generate(begin(x_), end(x_), randX);
        std::generate(begin(y_), end(y_), randY);
        std::generate(begin(z_), end(z_), randZ);

        sphexa::computeMortonCodes(begin(x_), end(x_), begin(y_), begin(z_),
                                   begin(codes_), box);

        std::vector<I> mortonOrder(n);
        sphexa::sort_invert(cbegin(codes_), cend(codes_), begin(mortonOrder));

        sphexa::reorder(mortonOrder, codes_);
        sphexa::reorder(mortonOrder, x_);
        sphexa::reorder(mortonOrder, y_);
        sphexa::reorder(mortonOrder, z_);
    }

    const std::vector<T>& x() const { return x_; }
    const std::vector<T>& y() const { return y_; }
    const std::vector<T>& z() const { return z_; }
    const std::vector<I>& mortonCodes() const { return codes_; }

private:

    sphexa::Box<T> box_;
    std::vector<T> x_, y_, z_;
    std::vector<I> codes_;
};
