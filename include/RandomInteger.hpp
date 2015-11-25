/**
 * @file
 * @brief Wrapper to random number generator enabling templated access
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

#include <cstdint>
#define RAND_STXXL

#ifdef RAND_STXXL
   #include <stxxl/random>
   /**
   * @brief Simple trait wrapper to STXXL PRNG based on the integer size.
   * 
   * In general the 64 bit generator is used; a faster specialization for 32 bit is provided.
   */
   template <size_t size>
   struct RandomInteger {
      /**
      * Uniformly draw a random number form the interval [0; supremum[.
      */
      inline static uint64_t randint(uint64_t supremum) {
         static stxxl::random_number64 rand;
         static_assert(size <= 8, "RandomInteger only supports integers up to 64bit");
         return rand(supremum);
      }
   };

   //! @copydoc RandomInteger
   template <>
   struct RandomInteger<4> {
      //! @copydoc RandomInteger::randint
      inline static uint32_t randint(uint32_t supremum) {
         static stxxl::random_number32 rand;
         return rand(supremum);
      }
   };

#else
   #include <random>
   template <size_t size>
   struct RandomInteger {
      /**
      * Uniformly draw a random number form the interval [0; supremum[.
      */
      FORCE_INLINE static uint64_t randint(uint64_t supremum) {
         static std::mt19937_64 generator;
         static std::uniform_int_distribution<uint64_t> distribution(0,supremum-1);
         
         if (distribution.b() != supremum + 1)
            distribution = std::uniform_int_distribution<uint64_t>(0,supremum-1);
         
         return distribution(generator);
      }
   };

   //! @copydoc RandomInteger
   template <>
   struct RandomInteger<4> {
      //! @copydoc RandomInteger::randint
      FORCE_INLINE static uint32_t randint(uint32_t supremum) {
         static std::mt19937 generator;
         static std::uniform_int_distribution<uint32_t> distribution(0,supremum-1);
         
         if (distribution.b() != supremum + 1)
            distribution = std::uniform_int_distribution<uint32_t>(0,supremum-1);
         
         return distribution(generator);
      }
   };
#endif
