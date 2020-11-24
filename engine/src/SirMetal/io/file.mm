#import <unordered_map>

#include "SirMetal/io/file.h"


namespace SirMetal{

    const std::unordered_map<std::string, FILE_EXT> stringToExt =
            {
                    {".obj", FILE_EXT::OBJ},
                    {".metal", FILE_EXT::METAL},
            };

    FILE_EXT getFileExtFromStr(const std::string &ext) {
        auto found = stringToExt.find(ext);
        if (found != stringToExt.end()) {return found->second;}
        return FILE_EXT::NONE;
    }


}
