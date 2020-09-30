#pragma once

#include <array>
#include <cassert>

namespace sphexa
{

namespace detail
{

//! \brief Expands a 10-bit integer into 30 bits by inserting 2 zeros after each bit.
inline unsigned expandBits(unsigned v)
{
    v &= 0x000003ff;
    v = (v * 0x00010001u) & 0xFF0000FFu;
    v = (v * 0x00000101u) & 0x0F00F00Fu;
    v = (v * 0x00000011u) & 0xC30C30C3u;
    v = (v * 0x00000005u) & 0x49249249u;
    return v;
}

//! \brief Expands a 21-bit integer into 63 bits by inserting 2 zeros after each bit.
inline std::size_t expandBits(std::size_t v)
{
    std::size_t x = v & 0x1fffffu; // get first 21 bits
    x = (x | x << 32u) & 0x1f00000000fffflu;
    x = (x | x << 16u) & 0x1f0000ff0000fflu;
    x = (x | x << 8u) & 0x100f00f00f00f00flu;
    x = (x | x << 4u) & 0x10c30c30c30c30c3lu;
    x = (x | x << 2u) & 0x1249249249249249lu;
    return x;
}

/*! \brief Calculates a 30-bit Morton code for a 3D point
 *
 * \param[in] x,y,z input coordinates within the unit cube [0,1]^3
 */
template <class T>
unsigned int morton3D_(T x, T y, T z, [[maybe_unused]] unsigned tag)
{
    assert(x >= 0.0 && x <= 1.0);
    assert(y >= 0.0 && y <= 1.0);
    assert(z >= 0.0 && z <= 1.0);

    // normalize floating point numbers
    // 1024 = 2^10, so we map the floating point numbers
    // in [0,1] to [0,1023] and convert to integers
    x = std::min(std::max(x * T(1024.0), T(0.0)), T(1023.0));
    y = std::min(std::max(y * T(1024.0), T(0.0)), T(1023.0));
    z = std::min(std::max(z * T(1024.0), T(0.0)), T(1023.0));
    unsigned int xx = detail::expandBits((unsigned int)x);
    unsigned int yy = detail::expandBits((unsigned int)y);
    unsigned int zz = detail::expandBits((unsigned int)z);

    // interleave the x, y, z components
    return xx * 4 + yy * 2 + zz;
}

/*! \brief Calculates a 63-bit Morton code for a 3D point
 *
 * \param[in] x,y,z input coordinates within the unit cube [0,1]^3
 */
template <class T>
std::size_t morton3D_(T x, T y, T z, [[maybe_unused]] std::size_t tag)
{
    assert(x >= 0.0 && x <= 1.0);
    assert(y >= 0.0 && y <= 1.0);
    assert(z >= 0.0 && z <= 1.0);

    // normalize floating point numbers
    // 2097152 = 2^21, so we map the floating point numbers
    // in [0,1] to [0,2097152-1] and convert to integers
    x = std::min(std::max(x * T(2097152.0), T(0.0)), T(2097151.0));
    y = std::min(std::max(y * T(2097152.0), T(0.0)), T(2097151.0));
    z = std::min(std::max(z * T(2097152.0), T(0.0)), T(2097151.0));
    std::size_t xx = detail::expandBits((std::size_t)x);
    std::size_t yy = detail::expandBits((std::size_t)y);
    std::size_t zz = detail::expandBits((std::size_t)z);

    // interleave the x, y, z components
    return xx * 4 + yy * 2 + zz;
}

} // namespace detail

/*! \brief Calculates a Morton code for a 3D point
 *
 * \tparam I specify either a 32 or 64 bit unsigned integer to select
 *           the precision.
 *           Note: needs to be specified explicitly.
 *
 * \param[in] x,y,z input coordinates within the unit cube [0,1]^3
 */
template <class I, class T>
inline I morton3D(T x, T y, T z)
{
    return detail::morton3D_(x,y,z, I{});
}


/*! \brief transfer a series of octree indices into a morton code
 *
 * \param indices indices[0] contains the octree index 0-7 for the top-level,
 *                 indices[1] reference to the first subdivision, etc
 *                 a 32-bit integer can resolve up to 10 layers
 * \return the morton code
 */
static unsigned mortonFromIndices(std::array<unsigned char, 10> indices)
{
    unsigned ret = 0;
    ret += indices[0] << 27u;
    ret += indices[1] << 24u;
    ret += indices[2] << 21u;
    ret += indices[3] << 18u;
    ret += indices[4] << 15u;
    ret += indices[5] << 12u;
    ret += indices[6] << 9u;
    ret += indices[7] << 6u;
    ret += indices[8] << 3u;
    ret += indices[9];

    return ret;
}

namespace detail {

template<class T>
static inline T normalize(T d, T min, T max) { return (d - min) / (max - min); }

} // namespace detail

/*! \brief compute the Morton codes for the input coordinate arrays
 *
 * \param[in]  [x,y,z][Begin, End] (const) input iterators for coordinate arrays
 * \param[out] order[Begin, End]  output for morton codes
 * \param[in]  [x,y,z][min, max]  coordinate bounding box
 */
template<class InputIterator, class OutputIterator, class T>
void computeMortonCodes(InputIterator  xBegin,
                        InputIterator  xEnd,
                        InputIterator  yBegin,
                        InputIterator  zBegin,
                        OutputIterator codesBegin,
                        T xmin, T xmax, T ymin, T ymax, T zmin, T zmax)
{
    using detail::normalize;
    using Integer = std::decay_t<decltype(*codesBegin)>;

    while (xBegin != xEnd)
    {
        *codesBegin++ = morton3D<Integer>(normalize(*xBegin++, xmin, xmax),
                                          normalize(*yBegin++, ymin, ymax),
                                          normalize(*zBegin++, zmin, zmax));
    }
}

} // namespace sphexa
