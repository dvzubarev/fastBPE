#ifndef FASTBPE_FASTBPEAPI_HPP
#define FASTBPE_FASTBPEAPI_HPP

#include "utils.hpp"

#include <string>
#include <vector>
#include <unordered_map>

namespace fastBPE {

struct subword_t{
    subword_t(std::string&& s, double f = 0.):token(std::move(s)),freq(f){}
    std::string token;
    double freq;
};

class Scorer{
    public:
    constexpr static double FREQ_WEIGHT = 0.1;
    constexpr static double LEN_WEIGHT = 0.9;

    Scorer(double max_freq=1):max_freq_(max_freq){}

    inline double freq_score(const double x)const {

        // return std::log(x) / std::log(34'478'712);
        return x / max_freq_;
    }
    inline double len_score(const double x)const{
        return std::log(x+1)/std::log(9);
    }

    double score_subwords(const std::vector<subword_t>& subwords)const;


    private:
    double max_freq_;
};

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

    Scorer scorer_;

};

std::vector<std::string> uniq_subwords(const std::vector<Encoder::variant_t>& variants,
                                       size_t min_subword_len);


}



#endif // FASTBPE_FASTBPEAPI_HPP
