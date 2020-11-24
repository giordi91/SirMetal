#pragma once
#include "nlohmann/json_fwd.hpp"

namespace SirMetal {

std::string getValueIfInJson(const nlohmann::json &data, const std::string &key,
                             const std::string &defValue);

unsigned int getValueIfInJson(const nlohmann::json &data,
                              const std::string &key,
                              const unsigned int &defValue);

int getValueIfInJson(const nlohmann::json &data, const std::string &key,
                     const int &defValue);

void assertInJson(const nlohmann::json &jobj, const std::string &key);

void getJsonObj(const std::string &path, nlohmann::json &outJson);
} // namespace SirMetal
