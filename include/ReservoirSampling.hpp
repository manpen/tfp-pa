/**
 * @file
 * @brief Implementation of reservoir sampling to sample k elements
 * from a stream of unknown size.
 *
 * @author Manuel Penschuck
 * @copyright
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * @copyright
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once
#include <vector>
#include <stxxl/bits/common/utils.h>
#include <RandomInteger.hpp>

template <typename T, typename IndexType=uint64_t>
/***
 * @brief Implementation of reservoir sampling to sample k elements
 * from a stream of unknown size. Supports erase.
 */
class ReservoirSampling {
public:
    using value_type = T; //! Type of the items sampled
    using iterator = typename std::vector<T>::iterator;

protected:
    using Random = RandomInteger<sizeof(IndexType)>;

    std::vector<T> _reservoir;
    size_t _reservoir_target_size;
    IndexType _elements_pushed;

public:
    //! Initialize and allocate a reservoir of requested size.
    //! @param reservoir_size has to be positive
    ReservoirSampling(size_t reservoir_size)
        : _reservoir(reservoir_size)
        , _reservoir_target_size(reservoir_size)
        , _elements_pushed(0)
    {
        assert(reservoir_size > 0);
    }

    //! Add element to reservoir with probability of reservoir_size / n,
    //! where n-1 is the number of elements previously pushed.
    void push(const T& d) {
        ++_elements_pushed;
        if (UNLIKELY(_reservoir_target_size <= _elements_pushed)) {
            // Initial fill
            _reservoir.push_back(d);
        } else {
            IndexType r = Random::randint(_elements_pushed);

            // Do not sample
            if (LIKELY(r >= _reservoir_target_size))
                return;

            if (LIKELY(r < _reservoir.size())) {
                // Sample by replacement
                _reservoir[r] = d;
            } else {
                // Sample by adding (in case reservoir grew smaller)
                _reservoir.push_back(d);
            }
        }
    }

    //! Returns true iff no item is in reservoir
    bool empty() const {
        return _reservoir.empty();
    }

    //! Returns an iterator to a sample drawn uniformly from reservoir
    const iterator sample() const {
        assert(!empty());
        IndexType r = Random::randint(_reservoir.size());
        return _reservoir.begin() + r;

    }

    //! Removes the element stored at the provided position
    void erase(const iterator & it) {
        assert(!empty());

        // ensure iterator is valid
        assert(_reservoir.begin() <= it);
        assert(it - _reservoir.begin() < _reservoir.size());

        // swap with last valid element and remove last one
        auto last = _reservoir.begin() + _reservoir.size() - 1;
        if (LIKELY(last != it)) std::swap(*it, *last);
        _reservoir.pop_back();
    }

    //! Executes erase(it) with probability 1 - reservoir_size/n, where n is
    //! the number of elements pushed. This function can be used to cancel
    //! the increased probability to draw *it, once it is revealed that *it is
    //! in the reservoir.
    void eraseMaybe(const iterator & it) {
        if(UNLIKELY(Random::randint(_elements_pushed) < _reservoir.size()))
            return;

        erase(it);
    }

    //! Iterator to first element of reservoir.
    iterator begin() {return _reservoir.begin();}

    //! Iterator to first element of reservoir.
    const iterator begin() const {return _reservoir.begin();}

    //! Iterator to past-the-end element of reservoir.
    iterator end() {return _reservoir.end();}

    //! Iterator to past-the-end element of reservoir.
    const iterator end() const {return _reservoir.end();}

};
