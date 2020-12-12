#include "API.hpp"
#include <boost/algorithm/string.hpp>

#include <iostream>
#include <fstream>

int main(int argc, char* argv[]){

    fastBPE::Encoder enc(argv[1], true);

    auto bpes = enc.apply(argv[2], 3);
    for(const auto& r : bpes)
        std::cout<<boost::join(r.subwords, " ")<<" ("<<r.score<<");"<<std::endl;

    std::cout<<"unique subwords: "<< boost::join(uniq_subwords(bpes, 3), " ")<<std::endl;

    // std::ofstream fout ("/tmp/codes");
    // if(!fout.is_open())
    //     std::cerr<<"failed to open "<<std::endl;
    // enc.save(fout);

    return 0;
}
