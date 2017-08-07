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
  const char *dl_cstring = std::getenv("DL_INTERCEPT");

  if(dl_cstring != NULL) {
    // Split DL_INTERCEPT substitution pairs on ","
    std::vector<std::string> tokens;
    boost::split(tokens, dl_cstring, boost::is_any_of(","));

    // Split each substitution pair on ":"
    for(std::string const& token : tokens) {
      std::vector<std::string> split_token;
      boost::split(split_token, token, boost::is_any_of(":"));

      // Remove leading/trailing whitespace
      boost::trim(split_token.front());
      boost::trim(split_token.back());

      // Place substitutions into map
      substitutions[split_token.front()] = split_token.back();
    }
  }

  // prepend DL_INTERCEPT_LD_LIBRARY_PATH and DL_PATH to LD_LIBRARY_PATH and PATH
  // This is required as Singularity currently has an issue with docker container env variables
  // see https://github.com/singularityware/singularity/issues/860
  const char *dl_library = std::getenv("DL_INTERCEPT_LD_LIBRARY_PATH");
  if(dl_library != NULL) {
    std::string new_library(dl_library);
 
    // Append DL_INTERCEPT_LD_LIBRARY_PATH to LD_LIBRARY_PATH
    const char *ld_library = std::getenv("LD_LIBRARY_PATH");
    if(ld_library != NULL) {
      new_library += ':';
      new_library += ld_library;
    }
    setenv("LD_LIBRARY_PATH", new_library.c_str(), 1);
  }
  const char *dl_path = std::getenv("DL_INTERCEPT_PATH");
  if(dl_path != NULL) {
    std::string new_path(dl_path);

    // Append DL_INTERCEPT_PATH to PATH
    const char *path = std::getenv("PATH");
    if(path != NULL) {
      new_path += ':';
      new_path += path;
    }
    setenv("PATH", new_path.c_str(), 1);
  }

}

__attribute__ ((__constructor__))
void DLI_init() {
  process_environment_variables();
}
