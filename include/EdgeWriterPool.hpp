/**
 * @file
 * @brief Configuration and management for multi-threaded edge writers
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
#include <string>
#include <memory>

#include <regex>
#include <fstream>
#include <cstdlib>
#include <iostream>

/**
 * @addtogroup host-mp-ew
 * @{
 */

/**
 * @brief Configuration and management for multi-threaded edge writers
 * 
 * Similar to the STXXL configuration file infrastructure, this Pool
 * searches for an .pagg_out - File in the current and home directory as
 * well as in $ENV{PAGGCFG}. The file contains path (prefixes) to which 
 * the edge lists are written. Line comments start with "#" and comment
 * out the rest of the list. Empty lines as well as leading/trailing spaces
 * are ignored.
 * 
 * Given prefix $PRE the file ${PRE}graph${tid}.bin is produced (i.e. you
 * have to include a trailing slash if $PRE is an directory). The prefixes
 * are used in a round robin fashion, if more worker are requested than 
 * prefixes found.
 * 
 * If no configuration was found, all output is written into the current
 * working directory.
 * 
 * @warning The edge writer are destroyed when this pool gets destroyed.
 * There is no reference counting. The user must ensure that there are no
 * use-after-free conditions.
 * 
 * @tparam EdgeOut  Type of the edge writer. Its constructor has to accept 
 *    the path to the out file as its first parameter. Additional parameters
 *    can be forwarded through this classes constructor.
 */
template <typename EdgeOut>
class EdgeWriterPool {
   const unsigned int _number_of_writers;  //!< Number of writer requested upon construction
   
   /**
    * @name Configuration
    * @{
    */
   std::vector<std::string> _base_path; //!< All prefixes found in configuration file
   
   ///! Test whether file is readable
   bool _exist_file(const std::string& path) {
      std::ifstream in(path.c_str());
      return in.good();
   }
   
   /**
    * Check several locations for edge output configuration files:
    *  - Environment variable PAGGCFG
    *  - Current working directory
    *  - Home directory
    * 
    * At the former two places, first .paggcfg.HOSTNAME is tried, then
    * .paggcfg without host suffix.
    * 
    * @warning The hostname is gathered from the environment variable
    * HOSTNAME which has to be explicitly set in many systems by adding
    * "export HOSTNAME" to .bashrc / .profile.
    */
   void _find_config() {

      // check PAGGCFG environment path
      const char* paggcfg = getenv("PAGGCFG");
      if (paggcfg && _exist_file(paggcfg))
         return _load_config(paggcfg);

      // read environment, unix style
      const char* hostname = getenv("HOSTNAME");
      const char* home = getenv("HOME");
      const char* suffix = "";


      // check current directory
      {
         std::string basepath = "./.pagg_out";

         if (hostname && _exist_file(basepath + "." + hostname + suffix))
               return _load_config(basepath + "." + hostname + suffix);

         if (_exist_file(basepath + suffix))
               return _load_config(basepath + suffix);
      }

      // check home directory
      if (home) {
         std::string basepath = std::string(home) + "/.pagg_out";

         if (hostname && _exist_file(basepath + "." + hostname + suffix))
               return _load_config(basepath + "." + hostname + suffix);

         if (_exist_file(basepath + suffix))
               return _load_config(basepath + suffix);
      }

      STXXL_ERRMSG("Warning: no EdgeWriter configuration file found; use ./");
      
      // load default configuration
      _base_path.clear();
      _base_path.emplace_back("./");
   }
   
   /**
    * Read in configuration file
    */
   void _load_config(const std::string & filename) {
      std::ifstream infile( filename );
      _base_path.clear();
      std::regex comment_exp("#.*$");
      std::regex e("^\\s*(\\S+)\\s*$");
      for(std::string line; std::getline(infile, line);) {
         line = std::regex_replace(line, comment_exp, "");
         line = std::regex_replace(line, e, "$1");
      
         if (line.empty())
            continue;
         
         _base_path.push_back(line);
      }
      
      for(auto p : _base_path)
         STXXL_VERBOSE2("Use base-path: " << p);
   }
   /** @} */
   
   /**
    * @name Writer management
    */
   std::vector<std::unique_ptr<EdgeOut>> _writers;
   
public:
   /**
    * Load configuration and construct all edge writers.
    * Arbitrary arguments can be passed on to the EdgeOut's constructor
    */
   template<typename... EdgeOutArgs>
   explicit EdgeWriterPool(unsigned int number_of_writer, EdgeOutArgs... args)
      : _number_of_writers(number_of_writer) 
   {
   // read base paths
      _find_config();

   // construct all writers
      assert(_base_path.size() > 0);
      _writers.clear();
      for(unsigned int i=0; i < _number_of_writers; i++) {
         std::stringstream ss;
         ss << _base_path[i % _base_path.size()] << "graph" << i << ".bin";
         _writers.emplace_back(new EdgeOut(ss.str(), args...));
      }
   }
   
   ~EdgeWriterPool() = default;
   
   /**
    * Return reference to write assigned to worker @p idx
    */
   EdgeOut & operator[] (unsigned int idx) {
      return *(_writers.at(idx));
   }
   
   /**
    * Sums the edgesWritten property of all writers.
    */
   uint64_t totalEdgesWritten() const {
      uint64_t edges_written = 0;
      for(auto & o : _writers)
         edges_written += o->edgesWritten();
      return edges_written;
   }
};

/** @} */