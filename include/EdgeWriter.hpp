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

#include <string>
#include <stxxl/io>
#include <stxxl/vector>
#include <stxxl/bits/common/uint_types.h>
#include <stxxl/bits/unused.h>

#include <FileDataType.hpp>

/**
 * @brief Edge Output for Decision Tree producing a binary edge list file
 *
 * @tparam out_type Type for vertex indices in the output file.
 *               A cast from std::uint64 to out_type has to be possible.
 */
class EdgeWriter {
   using out_type = DefaultFileDataType::data_type;
   using vector_type = DefaultFileDataType::vector_type;
   using bufwriter_type = typename vector_type::bufwriter_type;

   stxxl::linuxaio_file _file;
   vector_type _vector;
   bufwriter_type _writer;

   uint64_t _edges_written;

   bool _disable_output;

public:
   EdgeWriter() = delete;
   EdgeWriter(const EdgeWriter &) = delete;

   /**
    * Constructor.
    * @param[in] filename      Path to edge list. In case DISABLE_OUTPUT==true, an arbitrary value can be provided
    * @param[in] expected_num_elems An (over-estimation) of the number of edges produced.
    *
    * @note The initial output filesize is computed based on expected_num_elems.
    * If the value is to small, the file size has to be increased which may result in reduced performance.
    * However, there are no implications to the correctness.
    */
   EdgeWriter(const std::string & filename, uint64_t expected_num_elems = 0)
         : _file(filename, stxxl::file::DIRECT | stxxl::file::RDWR | stxxl::file::CREAT | stxxl::file::TRUNC)
         , _vector(&_file)
         , _writer(_vector.begin())
         , _edges_written(0)
         , _disable_output(false)
   {
      if (expected_num_elems) {
         _vector.resize(expected_num_elems);
      }

      STXXL_VERBOSE0(
            "EdgeWriter with " << sizeof(out_type) << "b per node to "
            << filename << " initialised; Expect " << expected_num_elems << " elements"
      );
   }

   //! Only after the destructor was called the output file is complete and has the correct size
   ~EdgeWriter() {
      if (LIKELY(!_disable_output)) {
         _writer.finish();
         _vector.resize( 2*_edges_written );
      }
   }

   //! Disable the writing to file
   void setDisableOutput(bool v) {
      _disable_output = v;
   }

   //! Materialize stream of vertices into file
   template <typename Stream>
   void writeVertices (Stream & stream) {
      if (_disable_output) {
         for(; !stream.empty(); ++stream);
         return;
      }

      uint64_t vertices = 0;
      for(; !stream.empty(); ++stream) {
         _writer << DefaultFileDataType::fromInternal(*stream);
         vertices++;
      }
      _edges_written += vertices / 2;
   }

   //! Materialize stream of pairs of vertices into file
   template <typename Stream>
   void writeEdges (Stream & stream) {
      if (_disable_output) {
         for(; !stream.empty(); ++stream);
         return;
      }

      for(; !stream.empty(); ++stream) {
         auto pair = *stream;
         _writer << DefaultFileDataType::fromInternal(pair.first)
                 << DefaultFileDataType::fromInternal(pair.second);
         _edges_written++;
      }
   }


   //! Returns the number of bytes per vertex used in the output file.
   //! If I/O is disabled 0 is returned.
   size_t bytesPerVertex() const {
      return (!_disable_output) * sizeof(out_type);
   }

   //! Returns the corrected file size (i.e. the file's size if the destructor were called at the moment in time).
   //! If I/O is disabled 0 is returned.
   size_t bytesFilesize() const {
      return 2 * edgesWritten() * bytesPerVertex();
   }


   //! Return the number of time operator() has been called
   uint64_t edgesWritten() const {
      return _edges_written;
   }
};
