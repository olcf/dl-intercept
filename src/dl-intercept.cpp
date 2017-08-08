#ifndef _GNU_SOURCE
  #define _GNU_SOURCE
#endif

#include <link.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <unordered_map>
#include <sstream>
#include <iostream>
#include <fstream>
#include "boost/algorithm/string.hpp"

static std::unordered_map<std::string, std::string> substitutions;

// Required by audit interface
extern "C" unsigned int la_version(unsigned int version) {
    return version;
}

// Identify requests for any matching substitution key and replace with matching map value
extern "C" char *la_objsearch(const char *name, uintptr_t *cookie, unsigned int flag) {
  // Catch the first instance of the loader trying to find the library
  if(flag == LA_SER_ORIG) {
    std::string obj_name(name);

    for(auto const& kv : substitutions) {
      // If a searched name matches a key, return the mapped value
      // If the key is an absolute filepath the loader will not search anywhere else
      if(obj_name.find(kv.first) != std::string::npos) {
          const char *blarg = std::getenv("LD_LIBRARY_PATH");
          return (char*)kv.second.c_str();
      }
    }
  }

  return (char*)name;
}

static void process_environment_variables() {
  // Populate std::map with libraries and substitues in following form:
  // DL_INTERCEPT=original:substitute, original2:substitute2
  // e.g. DL_INTERCEPT="libmpi.so:/opt/cray/blarg/libmpi.so, libmpif.so:/opt/cray/blarg/libmpif.so"
  // -OR-
  // From a file with each library substituion pair on a new line
  const char *dl_cstring = std::getenv("DL_SUBSTITUTIONS");

  std::vector<std::string> substituion_pairs;

  if(dl_cstring != NULL) {
    std::string dl_substitutions(dl_cstring);

    std::vector<std::string> substitution_pairs;

    // Extract substitution pairs into vector
    if(dl_substitutions.find(":") == std::string::npos) {
      // Read substitution pairs line by line
      std::ifstream substitutions_file;
      substitutions_file.open(dl_substitutions, std::ifstream::in);
      std::string line;
      while(getline(substitutions_file, line)) {
        substitution_pairs.push_back(line);
      }
      substitutions_file.close();
    }
    else {
      // Split substitution pairs on ","
      boost::split(substitution_pairs, dl_substitutions, boost::is_any_of(","));
    }  

    // Split each substitution pair on ":"
    for(std::string const& pair : substitution_pairs) {
      std::vector<std::string> split_pair;
      boost::split(split_pair, pair, boost::is_any_of(":"));

      if(split_pair.size() == 2) {
        // Remove leading/trailing whitespace
        boost::trim(split_pair.front());
        boost::trim(split_pair.back());

        // Place substitutions into map
        substitutions[split_pair.front()] = split_pair.back();
      }
    }
  }
}

__attribute__ ((__constructor__))
void DLI_init() {
  process_environment_variables();
}
