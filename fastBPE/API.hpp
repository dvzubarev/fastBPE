#ifndef FASTBPE_FASTBPEAPI_HPP
#define FASTBPE_FASTBPEAPI_HPP

#include "utils.hpp"

#include <string>
#include <vector>
#include <unordered_map>

namespace fastBPE {


class Encoder{
public:
    struct variant_t{
        double score;
        std::vector<std::string> subwords;
    };
    Encoder()=default;
    Encoder(const std::string& codesPath, bool strip_aux_tags = false);

    std::vector<variant_t> apply(const std::string& word, int k)const;
    void save(std::ostream& out)const;
    void load(std::istream& in);


private:
    using pair_t = std::pair<std::string, std::string>;
    using codes_dict_t = std::unordered_map<pair_t, double, pair_hash_t>;
    codes_dict_t codes_;
    bool strip_aux_tags_;

};

std::vector<std::string> uniq_subwords(const std::vector<Encoder::variant_t>& variants,
                                       size_t min_subword_len);


}



#endif // FASTBPE_FASTBPEAPI_HPP
