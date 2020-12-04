#ifndef FASTBPE_FASTBPEAPI_HPP
#define FASTBPE_FASTBPEAPI_HPP

#include "utils.hpp"

#include <string>
#include <vector>
#include <unordered_map>

namespace fastBPE {



class Encoder{
public:
    Encoder(const std::string& codesPath, bool strip_aux_tags = false);

    std::vector<std::string> apply(const std::string& word)const;

private:
    using pair_t = std::pair<std::string, std::string>;
    using codes_dict_t = std::unordered_map<pair_t, double, pair_hash_t>;
    codes_dict_t codes_;
    bool strip_aux_tags_;

};


}



#endif // FASTBPE_FASTBPEAPI_HPP
