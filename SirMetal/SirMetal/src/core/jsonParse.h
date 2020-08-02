#include "../vendors/json/json.hpp"
#include "file.h"
#include "log.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace SirMetal {
    template<typename T>
    inline T getValueIfInJson(const nlohmann::json &data, const std::string &key,
            const T &defaultValue) {
        if (data.find(key) != data.end()) {
            return data[key].get<T>();
        }
        return defaultValue;
    }

    inline void assertInJson(const nlohmann::json &jobj, const std::string &key) {
        const auto found = jobj.find(key);
        assert(found != jobj.end());
    }

    inline bool inJson(const nlohmann::json &jobj, const std::string &key) {
        const auto found = jobj.find(key);
        return found != jobj.end();
    }

    inline nlohmann::json getJsonObj(std::string path) {
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
                return jObj;
            } catch (...) {
                // if not lets throw an error
                SIR_CORE_ERROR("Error in parsing json file at path: \n{}",path);
                auto ex = std::current_exception();
                std::rethrow_exception(ex);
            }
        } else {
            SIR_CORE_ERROR("Provided file path does not exists:{}",path);
            return nlohmann::json();
        }
    }
}