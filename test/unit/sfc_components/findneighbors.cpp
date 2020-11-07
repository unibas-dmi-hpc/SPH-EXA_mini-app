#include <iostream>
#include <vector>

#include "gtest/gtest.h"

#include "sfc/findneighbors.hpp"

#include "coord_samples/random.hpp"

//! \brief simple N^2 all-to-all neighbor search
template<class T>
void all2allNeighbors(const T* x, const T* y, const T* z, int n, T radius,
                      int *neighbors, int *neighborsCount, int ngmax)
{
    T r2 = radius * radius;
    for (int i = 0; i < n; ++i)
    {
        T xi = x[i], yi = y[i], zi = z[i];

        int ngcount = 0;
        for (int j = 0; j < n; ++j)
        {
            if (j == i) { continue; }
            if (ngcount < ngmax && sphexa::distancesq(xi, yi, zi, x[j], y[j], z[j]) < r2)
            {
                neighbors[i * ngmax + ngcount++] = j;
            }
        }
        neighborsCount[i] = ngcount;
    }
}

void sortNeighbors(int *neighbors, int *neighborsCount, int n, int ngmax)
{
    for (int i = 0; i < n; ++i)
    {
        std::sort(neighbors + i*ngmax, neighbors + i*ngmax + neighborsCount[i]);
    }
}

TEST(FindNeighbors, treeLevel)
{
    EXPECT_EQ(3, sphexa::radiusToTreeLevel(0.124, 1.));
    EXPECT_EQ(2, sphexa::radiusToTreeLevel(0.126, 1.));
}


template<class T, class I>
class NeighborCheck
{
public:
    NeighborCheck(T r, int np, sphexa::Box<T> b) : radius(r), n(np), box(b) {}

    void check()
    {
        using real = T;
        using CodeType = I;

        int ngmax = 100;

        real minRange = std::min(std::min(box.xmax()-box.xmin(), box.ymax()-box.ymin()),
                                 box.zmax()-box.zmin());
        RandomCoordinates<real, CodeType> coords(n, box);

        std::vector<int> neighborsRef(n * ngmax), neighborsCountRef(n);
        all2allNeighbors(coords.x().data(), coords.y().data(), coords.z().data(), n, radius,
                         neighborsRef.data(), neighborsCountRef.data(), ngmax);
        sortNeighbors(neighborsRef.data(), neighborsCountRef.data(), n, ngmax);

        std::vector<int> neighborsProbe(n * ngmax), neighborsCountProbe(n);
        for (int i = 0; i < n; ++i)
        {
            sphexa::findNeighbors(i, coords.x().data(), coords.y().data(), coords.z().data(),
                                  radius, minRange, coords.mortonCodes().data(),
                                  neighborsProbe.data(), neighborsCountProbe.data(), n, ngmax);
        }
        sortNeighbors(neighborsProbe.data(), neighborsCountProbe.data(), n, ngmax);

        EXPECT_EQ(neighborsRef, neighborsProbe);
        EXPECT_EQ(neighborsCountRef, neighborsCountProbe);
    }

private:
    T   radius;
    int n;
    sphexa::Box<T> box;
};

class FindNeighborsRandomUniform : public testing::TestWithParam<std::tuple<double, int, sphexa::Box<double>>>
{
public:
    template<class I>
    void check()
    {
        double radius     = std::get<0>(GetParam());
        int    nParticles = std::get<1>(GetParam());
        sphexa::Box<double> box = std::get<2>(GetParam());
        {
            NeighborCheck<double, I> chk(radius, nParticles, box);
            chk.check();
        }
    }
};

TEST_P(FindNeighborsRandomUniform, all2allComparison32bit)
{
    check<uint32_t>();
}

TEST_P(FindNeighborsRandomUniform, all2allComparison64bit)
{
    check<uint64_t>();
}

std::array<double, 2> radii{0.124, 0.0624};
std::array<int, 1>    nParticles{5000};
std::array<sphexa::Box<double>, 2> boxes{{ {0.,1.,0.,1.,0.,1.},
                                           {-1.2, 0.23, -0.213, 3.213, -5.1, 1.23} }};

INSTANTIATE_TEST_SUITE_P(RandomUniformNeighbors,
                         FindNeighborsRandomUniform,
                         testing::Combine(testing::ValuesIn(radii),
                                          testing::ValuesIn(nParticles),
                                          testing::ValuesIn(boxes)));