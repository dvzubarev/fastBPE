#include "API.hpp"
#include <boost/algorithm/string.hpp>

#include <iostream>
#include <fstream>

int main(int argc, char* argv[]){

    bool strip_aux_tags = false;
    fastBPE::Encoder enc(argv[1], strip_aux_tags);
    std::ifstream inf(argv[2]);
    std::string line;
    constexpr std::string_view delim{" "};

    while (getline(inf, line)) {
        std::vector<std::string> str_vec;
        boost::algorithm::split(str_vec,line,boost::is_any_of(" "),boost::token_compress_on);

        for (const auto& word : str_vec) {
            std::cout<<"word: "<<word<<'\n';
            auto bpes = enc.apply(word, 3);
            for(const auto& r : bpes)
                std::cout<<boost::join(r.subwords, " ")<<" ("<<r.score<<");"<<std::endl;

            auto unique_subwords = boost::join(uniq_subwords(bpes, 3), " ");
            std::cout<<"unique subwords: "<<unique_subwords <<std::endl;
        }
    }


    return 0;
}
