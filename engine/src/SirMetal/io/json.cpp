#include "SirMetal/io/json.h"

#include <exception>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "SirMetal//io/fileUtils.h"
#include "nlohmann/json.hpp"

namespace SirMetal {

std::string getValueIfInJson(const nlohmann::json &data, const std::string &key,
                             const std::string &defValue) {
  if (data.find(key) != data.end()) {
    return data[key].get<std::string>();
  }
  return defValue;
}

unsigned int getValueIfInJson(const nlohmann::json &data,
                              const std::string &key,
                              const unsigned int &defValue) {
  if (data.find(key) != data.end()) {
    return data[key].get<unsigned int>();
  }
  return defValue;
}
int getValueIfInJson(const nlohmann::json &data, const std::string &key,
                     const int &defValue) {
  if (data.find(key) != data.end()) {
    return data[key].get<unsigned int>();
  }
  return defValue;
}

void assertInJson(const nlohmann::json &jobj, const std::string &key) {
  const auto found = jobj.find(key);
  assert(found != jobj.end());
}

void getJsonObj(const std::string &path, nlohmann::json &outJson) {
  bool res = fileExists(path);
  if (res) {
    // let s open the stream
    std::ifstream st(path);
    std::stringstream sBuffer;
    sBuffer << st.rdbuf();
    std::string sBuffStr = sBuffer.str();

    try {
      // try to parse
      nlohmann::json jObj = nlohmann::json::parse(sBuffStr);
      outJson = jObj;
    } catch (...) {
      // if not lets throw an error
      auto ex = std::current_exception();
      printf("ERROR, in parsing json file at path:%s : \nexception %s", path.c_str(),
             path.c_str());
      assert(0);
      outJson = nlohmann::json();
    }
  } else {
    assert(0);
    outJson = nlohmann::json();
  }
}
} // namespace SirMetal