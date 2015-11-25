/**
 * @file
 * @brief Edge Output for Decision Tree producing a binary edge list file 
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

#include <stxxl/bits/common/types.h>

/**
 * @brief Result of a Distribution Count
 */
template <typename T>
struct DistributionBlockDescriptor {
    using value_type = T;
    
    T value;   //!< Value represented by this contained
    stxxl::uint64 count; //!< Occurrences of this value in input
    stxxl::uint64 index; //!< Sum of occurrences reported so far
};

/**
 * @brief Generic Comparator for distribution count
 */
template <typename T>
struct DistributionEqualComparator
{
    bool operator () (const T& a, const T& b) const { return a == b; }
};

/* 
 * This stream object performs a "run-lenght encoding", i.e. it counts the number of
 * equal items in the input stream. Each block is collapsed into one output items containing
 * the value of the input item, the number of items in the block (count) as well as the number
 * of sampled input items so far (index).
 * Assuming the input is sorted (asc or desc) the output is a distribution count.
 */
template <typename InputStream, typename T=stxxl::uint64, typename Comparator=DistributionEqualComparator<T>>
class DistributionCount {
public:    
    using value_type = DistributionBlockDescriptor<T>;
    using this_type = DistributionCount<InputStream, T, Comparator>;
    
private:
    InputStream & _in_stream;
    stxxl::uint64 _items_sampled;
    Comparator _equal;
    
    value_type _current_element;
    bool _empty;
    
    void _sample_next_block() {
        if (_in_stream.empty()) {
            _empty = true;
            return;
        }
         
        T value = *_in_stream;
        stxxl::uint64 count = 0;
        
        while( !_in_stream.empty() && _equal(*_in_stream, value) ) {
            ++_in_stream;
            ++count;
        }
        
        _items_sampled += count;
        
        _current_element = {
            .value = value, .count = count, .index = _items_sampled
        };
    }
    
public:
    DistributionCount(InputStream& input, bool start = true)
        : _in_stream(input)
        , _items_sampled(0)
        , _current_element({0, 0, 0}) // prevent irrelevant warnings further downstream
        , _empty(false)
    {
       if (start)
          _sample_next_block();
       else
          _empty = true;
    }
    
    void restart() {
       _empty = false;
       _items_sampled = 0;
       _sample_next_block();
    }

/**
 * @name STXXL Streaming interface
 * @{
 */    
    const value_type & operator*() const {
        return _current_element;
    };
    
    const value_type * operator->() const {
        return &_current_element;
    };
    
    DistributionCount & operator++() {
        _sample_next_block();
        return *this;
    }
        
    bool empty() const {
        return _empty;
    };
/* @} */
};

template <typename T>
inline std::ostream &operator<<(std::ostream &os, DistributionBlockDescriptor<T> const &m) {
    return os << "{value: " << m.value << ", degree:" << m.count << ", index: " << m.index << "}";
}
