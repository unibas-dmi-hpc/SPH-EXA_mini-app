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

/*! \file
 * \brief  Exposes gather functionality to reorder arrays with a map
 *
 * \author Sebastian Keller <sebastian.f.keller@gmail.com>
 */

#include <cstdint>
#include <memory>

namespace cstone
{

template<class T, class LocalIndex> class DeviceMemory;

/*! \brief A stateful functor for reordering arrays on the gpu
 *
 * @tparam T   float or double
 * @tparam I   32- or 64-bit unsigned integer
 */
template<class T, class I>
class DeviceGather
{
public:
    //! \brief type to index node-local particles, assuming fewer than 2^32
    using LocalIndex = unsigned;
    using CodeType   = I;

    DeviceGather();

    ~DeviceGather();

    /*! \brief upload the new reorder map to the device and reallocates buffers if necessary
     *
     * If the sequence [map_first:map_last] does not contain each element [0:map_last-map_first]
     * exactly once, the behavior is undefined.
     */
    void setReorderMap(const LocalIndex* map_first, const LocalIndex* map_last);

    /*! \brief sort given Morton codes on the device and determine reorder map based on sort order
     *
     * \param[inout] codes_first   pointer to first Morton code
     * \param[inout] codes_last    pointer to last Morton code
     *
     * Precondition:
     *   - [codes_first:codes_last] is a continues sequence of accessible elements of size N
     *
     * Postcondition
     *   - [codes_first:codes_last] is sorted
     *   - subsequent calls to operator() apply a gather operation to the input sequence
     *     with the map obtained from sort_by_key with [codes_first:codes_last] as the keys
     *     and the identity permutation as the values
     *
     *  Remarks:
     *    - reallocates space on the device if necessary to fit N elements of type LocalIndex
     *      and a second buffer of size max(2N*sizeof(T), N*sizeof(I))
     */
    void setMapFromCodes(CodeType* codes_first, CodeType* codes_last);

    /*! \brief reorder the array \a values according to the reorder map provided previously
     *
     * \a values must have at least as many elements as the reorder map provided in the last call
     * to setReorderMap or setMapFromCodes, otherwise the behavior is undefined.
     */
    void operator()(T* values);

private:
    std::size_t mapSize_{0};

    std::unique_ptr<DeviceMemory<T, LocalIndex>> deviceMemory_;
};

extern template class DeviceGather<float,  unsigned>;
extern template class DeviceGather<float,  uint64_t>;
extern template class DeviceGather<double, unsigned>;
extern template class DeviceGather<double, uint64_t>;

} // namespace cstone
