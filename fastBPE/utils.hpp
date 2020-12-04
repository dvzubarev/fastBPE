#ifndef FASTBPE_UTILS_HPP
#define FASTBPE_UTILS_HPP

#include <utility>
#include <string>

#include <boost/container_hash/hash.hpp>

namespace fastBPE {

struct pair_hash_t{
    template <class T1, class T2>
    size_t operator()(const std::pair<T1, T2> &p) const {
        size_t h (0);
        boost::hash_combine(h, p.first);
        boost::hash_combine(h, p.second);
        return h;
    }

};

const static char *kBegWord = "<w>";
const static size_t kBegWordLength = 3;
const static char *kEndWord = "</w>";
const static size_t kEndWordLength = 4;
const static char *kTokenDelim = "@@";
const static size_t kTokenDelimLength = 2;

std::size_t token_len(std::string s);

}

#endif // FASTBPE_UTILS_HPP
