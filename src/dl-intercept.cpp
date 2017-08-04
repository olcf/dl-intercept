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
#include "boost/algorithm/string.hpp"

static std::unordered_map<std::string, std::string> substitutions;

// Required
extern "C" unsigned int la_version(unsigned int version) {
    return version;
}

// Identify requests to find libmpi* and switch out for system specific libraries
extern "C" char *la_objsearch(const char *name, uintptr_t *cookie, unsigned int flag) {
  // Catch the first instance of the loader trying to find the library
  if(flag == LA_SER_ORIG) {
    std::string obj_name(name);

    for(auto const& kv : substitutions) {
      // If a searched name matches a key, return the mapped value
      if(obj_name.find(kv.first) != std::string::npos) {
          return (char*)kv.second.c_str();
      }
    }
  }

  return (char*)name;
}

// Populate std::map with libraries and substitues in following form:
// DL_INTERCEPT=original->substitute original2->substitute2
// e.g. DL_INTERCEPT="libmpi.so:/opt/cray/blarg/libmpi.so,libmpif.so:/opt/cray/blarg/libmpif.so"
static void process_environment_variables() {
  const char *dl_cstring = std::getenv("DL_INTERCEPT");

  if(dl_cstring != NULL) {
    // Split DL_INTERCEPT on ":"
    std::string dl_string(dl_cstring);
    std::vector<std::string> tokens;
    boost::split(tokens, dl_string, boost::is_any_of(","));

    // Split each token on "->" and place into substitutions map
    for(std::string const& token : tokens) {
      std::vector<std::string> split_token;
      boost::split(split_token, token, boost::is_any_of(":"));

      // Remove leading/trailing whitespace
      boost::trim(split_token.front());
      boost::trim(split_token.back());

      substitutions[split_token.front()] = split_token.back();
    }
  }
}

__attribute__ ((__constructor__))
void DLI_init() {
  process_environment_variables();
}
