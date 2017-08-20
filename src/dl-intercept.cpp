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
#include <vector>
#include "boost/algorithm/string.hpp"

static void process_environment_variables();

static std::unordered_map<std::string, std::string> substitutions;
static bool initialized = false;

// Required by audit interface
extern "C" unsigned int la_version(unsigned int version) {
    return version;
}

// Identify requests for any matching substitution key and replace with matching map value
extern "C" char *la_objsearch(const char *name, uintptr_t *cookie, unsigned int flag) {
  // Catch the initial name of the object the loader is looking for
  if(flag == LA_SER_ORIG) {
    // Initialize environment variables
    if(!initialized) {
      process_environment_variables();
      initialized = true;
    }

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
  // RTLD_SUBSTITUTIONS=original:substitute, original2:substitute2
  // e.g. RTLD_SUBSTITUTIONS="libmpi.so:/opt/cray/blarg/libmpi.so, libmpif.so:/opt/cray/blarg/libmpif.so"
  // -OR-
  // From a file with each library substituion pair on a new line
  const char *dl_cstring = std::getenv("RTLD_SUBSTITUTIONS");

  if(dl_cstring != NULL) {
    std::string dl_substitutions(dl_cstring);
    std::vector<std::string> substitution_pairs;

    // Extract substitution pairs into vector
    if(dl_substitutions.find(':') == std::string::npos) { // If a filename was provided or malformed pair
      // Read substitution pairs from file line by line
      std::ifstream substitutions_file;
      substitutions_file.open(dl_substitutions, std::ifstream::in);
      if(substitutions_file.bad()) {
        std::cout<<"ERROR: Failure to open substitutions file " << dl_substitutions << std::endl;
        exit(1);
      }

      std::string line;

      while(getline(substitutions_file, line)) {
        // remove all white spaces
        boost::erase_all(line, " ");

        // Ignore line starting with '#' to allow comments
        // And only add strings which contain a ":"
        if(line.length() <= 3 || line.at(0) == '#' || line.find(':') == std::string::npos) {
          continue;
        }
        else {
          substitution_pairs.push_back(line);
        }
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
        split_pair.front();
        split_pair.back();

        // Place substitutions into map
        substitutions[split_pair.front()] = split_pair.back();
      }
      else {
        std::cout<<"Formatting error in substitution pair: "<<pair<<std::endl;
      }
    }
  }
}
