#include "API.hpp"
#include "fastBPE.hpp"

namespace fastBPE {


struct subword_t{
    subword_t(std::string&& s, double f = 0.):token(std::move(s)),freq(f){}
    string token;
    double freq;
};

struct process_bpe_state_t{
    process_bpe_state_t(vector<string>&& s): hash(0) {
        subwords.reserve(s.size());
        for(auto& w : s){
            boost::hash_combine(hash, w);
            subwords.emplace_back(std::move(w));
        }

    }
    process_bpe_state_t(vector<subword_t>&& s): subwords(std::move(s)),
                                                hash(0) {
        for(auto& w : subwords)
            boost::hash_combine(hash, w.token);
    }

    process_bpe_state_t(vector<subword_t>&& s, uint64_t h):subwords(std::move(s)),
                                                           hash(h){
    }

    vector<subword_t> subwords;
    uint64_t hash;
    double score;

    process_bpe_state_t create_new_state(const tps& p, int start_idx, double freq) {
      vector<subword_t> new_subwords;
      new_subwords.insert(new_subwords.end(), subwords.begin(), subwords.begin() + start_idx);
      new_subwords.emplace_back(p.first + p.second, freq);
      new_subwords.insert(new_subwords.end(), subwords.begin() + start_idx + 2, subwords.end());
      return process_bpe_state_t{std::move(new_subwords)};
    }
};


std::size_t token_len(string s){

    std::size_t realLength = 0;
    int pos = 0, end = s.size();
    auto ret = std::strncmp(s.data(), kBegWord, std::min(s.size(), kBegWordLength));
    if (ret == 0)
        pos = kBegWordLength;

    if(s.size() > kEndWordLength){
        ret = std::strncmp(s.data() + s.size() - kEndWordLength,
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

inline double sigmoid(double x, double x0, double k){
    return 1. / (1. + std::exp(-k * (x - x0)));
}
inline double freq_score(double x){
    //see params.py
    //log2 params
    return sigmoid(x, 17.789, 1.277);
    //log10 params
    // return sigmoid(x, 5.355, 4.241);
}
inline double len_score(double x){
    //see params.py
    return sigmoid(x, 4.179, 0.921);
}

const double FREQ_WEIGHT = 0.3;
const double LEN_WEIGHT = 0.7;

vector<string> select_subwords(vector<process_bpe_state_t>&& states){
    for(auto& state: states){
        int n = 0;
        state.score = 0.;
        for (const auto& sub : state.subwords){
            ++n;
            auto sz = token_len(sub.token);
            if(sz < 2 ){
                state.score += len_score(sz) * LEN_WEIGHT + FREQ_WEIGHT;
                continue;
            }

            auto fscore = freq_score(sub.freq);
            auto lscore = len_score(sz);
            state.score += FREQ_WEIGHT * fscore + LEN_WEIGHT * lscore;
        }
        state.score /= n;
    }
    auto it = std::max_element(std::begin(states), std::end(states),
                               [](const auto& l, const auto& r){return l.score < r.score;});

    vector<string> strs(it->subwords.size());
    std::transform(std::begin(it->subwords), std::end(it->subwords),
                   std::begin(strs), [](const auto& sub) {return sub.token;});


    return strs;

}



vector<string>
process_bpe_full(vector<string> &subwords,
                 const unordered_map<tps, double, pair_hash_t> &codes) {

  vector<process_bpe_state_t> states;

  vector<process_bpe_state_t> final_subwords;
  unordered_set<uint64_t> seen;

  states.emplace_back(std::move(subwords));
  while(not states.empty()){
      auto cur_state = states.back();
      states.pop_back();
      bool final_state = true;
      for (size_t i = 0; i < cur_state.subwords.size() - 1; i++) {
          auto pair = make_pair(cur_state.subwords[i].token,
                                cur_state.subwords[i + 1].token);
          auto it = codes.find(pair);
          if (it == codes.end())
              continue;

          final_state = false;

          auto new_state = cur_state.create_new_state(pair, i, it->second);
          const auto [t, inserted] = seen.insert(new_state.hash);
          if (inserted)
              states.push_back(std::move(new_state));
      }
      if (final_state)
          final_subwords.push_back(std::move(cur_state));
  }

  //choose one
  auto new_subwords = select_subwords(std::move(final_subwords));

  return new_subwords;
}


Encoder::Encoder(const std::string& codes_path){
    unordered_map<string, tps> reversed_codes;
    readCodes(codes_path.c_str(), codes_, reversed_codes);
}

std::vector<std::string>
Encoder::apply(const std::string& word)const{
    vector<string> word_bpes;
    split_word_to_chars(word, word_bpes);
    return process_bpe_full(word_bpes, codes_);
}

}
