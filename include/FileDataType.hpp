/**
 * @file
 * @brief Default definition for various output data types
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

#include <stxxl/bits/common/uint_types.h>

template <typename T>
struct FileDataType {
   using data_type = T;
   using vector_type = typename stxxl::VECTOR_GENERATOR<T, 4u, 8u>::result;

   static uint64_t  toInternal(const data_type&  x) {return x;}
   static data_type fromInternal(const uint64_t& x) {return data_type(x);}
};

template <>
struct FileDataType<stxxl::uint40> {
   using data_type = stxxl::uint40;
   using vector_type = typename stxxl::VECTOR_GENERATOR<data_type, 4u, 8u, 10485760u>::result;

   static uint64_t  toInternal  (const data_type& x) {return x.u64();}
   static data_type fromInternal(const uint64_t&  x) {return data_type(stxxl::uint64(x));}
};

template <>
struct FileDataType<stxxl::uint48> {
   using data_type = stxxl::uint48;
   using vector_type = typename stxxl::VECTOR_GENERATOR<data_type, 4u, 8u, 10485760u>::result;

   static uint64_t  toInternal  (const data_type& x) {return x.u64();}
   static data_type fromInternal(const uint64_t&  x) {return data_type(stxxl::uint64(x));}
};

#if FILE_DATA_WIDTH == 32
   using DefaultFileDataType = FileDataType<uint32_t>;
#else
   #if FILE_DATA_WIDTH == 40
      using DefaultFileDataType = FileDataType<stxxl::uint40>;
      #else
         #if FILE_DATA_WIDTH == 48
         using DefaultFileDataType = FileDataType<stxxl::uint48>;
         #else
         using DefaultFileDataType = FileDataType<uint64_t>;
         #endif
   #endif
#endif

template <class Stream, typename FileT = DefaultFileDataType::data_type, typename T = uint64_t>
class FileDataTypeReader {
public:
   using value_type = T;

protected:
   Stream & _stream;
   T _current;

public:
   FileDataTypeReader(Stream & stream)
         : _stream(stream)
   { _current = T(FileDataType<FileT>::toInternal(*_stream)); }

   FileDataTypeReader& operator++() {
      ++_stream;
      if (LIKELY(!_stream.empty()))
         _current = T(FileDataType<FileT>::toInternal(*_stream));
      return *this;
   }

   bool empty() const{return _stream.empty();}

   const value_type & operator*() const {return _current;}
};
