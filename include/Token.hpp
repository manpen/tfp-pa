/**
 * @file
 * @brief TFP-Token
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
#include <limits>

/**
 * @brief TFP-Token
 *
 * A token is a 3-tuple (bool query, int idx, int value).
 * A sequence of tokens is used to describe the actions required to generate
 * a preferential attachment graph. The exact structure of this sequence is
 * determined by the PA model used.
 *
 * A token defines a default comparator (lexicographically) and supports
 * STXXL comparators (which include infimum and supremum).
 */
template <typename T>
class Token {
public:
   using value_type = T;

protected:
   value_type _id;
   value_type _value;

public:
   struct ComparatorAsc {
      bool operator() (const Token & a, const Token & b) const {return a < b;}

      Token min_value() const {
         return Token(false, std::numeric_limits<T>::min(), std::numeric_limits<T>::min());
      }

      Token max_value() const {
         return Token(true, std::numeric_limits<T>::max(), std::numeric_limits<T>::max());
      }
   };

   struct ComparatorDesc {
      bool operator() (const Token & a, const Token & b) const {return b < a;}

      Token min_value() const {
         return Token(true, std::numeric_limits<T>::max(), std::numeric_limits<T>::max());
      }

      Token max_value() const {
         return Token(false, std::numeric_limits<T>::min(), std::numeric_limits<T>::min());
      }
   };

   //! Default constructor required from STXXL; does not initialise anything
   Token() {}

   //! Constructor based on token type
   Token(bool query, const value_type & id, const value_type & value) :
      _id( (id << 1) + query ), _value(value) {}

   //! Returns index (without type)
   value_type id() const { return  _id >> 1; }

   //! Returns a const reference to the type
   const value_type & value() const {return _value;}

   //! Returns a reference to the value
   value_type & value() {return _value;}

   //! Compare lexicographically by index, type, value
   bool operator<(const Token & other) const {
      return _id < other._id || (_id == other._id && _value < other._value);
   }

   //! True if it is a query token
   bool query() const {
      return _id & 1;
   }
};

template <typename T>
std::ostream& operator << (std::ostream& stream, const Token<T> &q) {
   stream << "<Token " << (q.query() ? "query" : "link ") << " Id: " << q.id() << " Value: " << q.value() << ">";
   return stream;
}

//! Token type using
using Token64 = Token<uint64_t>;
