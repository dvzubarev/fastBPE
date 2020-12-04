#include "API.hpp"
#include <boost/algorithm/string.hpp>

#include <iostream>

int main(int argc, char* argv[]){

    fastBPE::Encoder enc(argv[1], true);
    auto bpes = enc.apply(argv[2]);

    std::cout<<boost::join(bpes, " ")<<std::endl;

    return 0;
}
