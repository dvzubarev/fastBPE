#include "utils.hpp"

namespace fastBPE {

std::size_t token_len(std::string s){

    std::size_t realLength = 0;
    int pos = 0, end = s.size();
    if (s.size() > kBegWordLength){
        auto ret = std::strncmp(s.data(), kBegWord, kBegWordLength);
        if (ret == 0)
            pos = kBegWordLength;
    }

    if(s.size() > kEndWordLength){
        auto ret = std::strncmp(s.data() + s.size() - kEndWordLength,
                                kEndWord, kEndWordLength);
        if (ret == 0)
            end -= kEndWordLength;
    }

    while (pos < end) {
        bool newChar = (s[pos] & 0xc0) != 0x80; // not a continuation byte
        realLength += newChar;
        ++pos;
    }
    return realLength;
}

}
