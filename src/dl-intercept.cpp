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
  // Catch the first instance of the loader trying to find our libraries
  if(flag == LA_SER_ORIG) {

  }

  return (char*)name;
}

// Populate std::map with libraries and substitues in following form:
// DL_INTERCEPT=original->substitute original2->substitute2
// The original entry is assumed to be a regex expression
// e.g. DL_INTERCEPT="*libmpi.so*->/opt/cray/blarg/libmpi.so *libmpif.so->/opt/cray/blarg/libmpif.so"
static void process_environment_variables() {
  const char *dl_cstring = std::getenv("DL_INTERCEPT");

  if(dl_cstring != NULL) {
    // Split DL_INTERCEPT on whitespace
    std::string dl_string(dl_cstring);
    std::vector<std::string> tokens;
    boost::split(tokens, dl_string, boost::is_any_of("\t "));

    // Split each token on "->" and place into substitutions map
    for(std::string token : tokens) {
      std::vector<std::string> split_token;
      split(split_token, token, boost::is_any_of("->"));
      substitutions[split_token.front()] = split_token.back();
    }
  }
}

__attribute__ ((__constructor__))
void DLI_init() {
  process_environment_variables();
}
