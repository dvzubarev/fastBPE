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


inline double sigmoid(double x, double x0, double k){
    return 1. / (1. + std::exp(-k * (x - x0)));
}
inline double freq_score(double x){
    //see params.py
    //log2 params
    return sigmoid(x, 17.789, 1.277);
    // return sigmoid(x, 14.842, 1.274);
    //log10 params
    // return sigmoid(x, 5.355, 4.241);
}
inline double len_score(double x){
    //see params.py
    return sigmoid(x, 4.179, 0.921);
}

const double FREQ_WEIGHT = 0.2;
const double LEN_WEIGHT = 0.8;

void score_state(process_bpe_state_t& state){
    int n = 0;
    state.score = 0.;
    for (const auto& sub : state.subwords){
        ++n;
        auto sz = token_len(sub.token);
        if(sz < 2 ){
            state.score += len_score(sz) * LEN_WEIGHT + 0.5 * FREQ_WEIGHT;
            continue;
        }

        auto fscore = freq_score(sub.freq);
        auto lscore = len_score(sz);
        state.score += FREQ_WEIGHT * fscore + LEN_WEIGHT * lscore;
    }
    state.score /= n;
}

vector<string> select_subwords(vector<process_bpe_state_t>&& states){
    for(auto& state: states)
        score_state(state);

    auto it = std::max_element(std::begin(states), std::end(states),
                               [](const auto& l, const auto& r){return l.score < r.score;});

    vector<string> strs(it->subwords.size());
    std::transform(std::begin(it->subwords), std::end(it->subwords),
                   std::begin(strs), [](const auto& sub) {return sub.token;});


    return strs;

}


constexpr size_t MAX_STATES_COUNT = 50;

vector<string>
process_bpe_full(vector<string> &subwords,
                 const unordered_map<tps, double, pair_hash_t> &codes) {

    auto heap_pred = [](const process_bpe_state_t& l, const process_bpe_state_t& r){
        return l.score > r.score;
    };

  vector<process_bpe_state_t> states;

  process_bpe_state_t best_state((vector<string>()));
  best_state.score = 0;
  unordered_set<uint64_t> seen;

  states.emplace_back(std::move(subwords));
  while(not states.empty()){
      std::pop_heap(states.begin(), states.end(), heap_pred);
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

          if(not inserted)
              //already seen state
              continue;

          score_state(new_state);

          if(states.size() < MAX_STATES_COUNT)
              states.push_back(std::move(new_state));
          else if (new_state.score > states.front().score){
              std::pop_heap(states.begin(), states.end(), heap_pred);
              states.back() = std::move(new_state);
          }
          std::push_heap(states.begin(), states.end(), heap_pred);
      }
      if(final_state)
          if (cur_state.score > best_state.score)
              best_state = std::move(cur_state);

  }

  vector<string> strs(best_state.subwords.size());
  std::transform(std::begin(best_state.subwords), std::end(best_state.subwords),
                 std::begin(strs), [](const auto& sub) {return sub.token;});
  return strs;
}


void readCodes(const char *fp,
               unordered_map<tps, double, pair_hash_t> &codes,
               bool strip_aux_tags = false) {
    ifstream file(fp);
    if (!file) {
        fprintf(stderr, "Cannot open codes file %s\n", fp);
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "Loading codes from %s ...\n", fp);
    string line;
    while (getline(file, line)) {
        vector<string> splits;
        split(splits, line, ' ');
        assert(splits.size() == 3);
        if (strip_aux_tags){
            auto ret = std::strncmp(splits[0].data(), kBegWord, kBegWordLength);
            if (ret == 0)
                splits[0] = splits[0].substr(kBegWordLength);

            if(splits[1].size() > kEndWordLength){
                ret = std::strncmp(splits[1].data() + splits[1].size() - kEndWordLength,
                                   kEndWord, kEndWordLength);
                if (ret == 0)
                    splits[1] = splits[1].substr(0, splits[1].size() - kEndWordLength);
            }
        }
        auto pair = make_pair(splits[0], splits[1]);
        auto val = std::stod(splits[2]);
        auto fit  = codes.find(pair);
        if (fit == codes.end())
            codes[pair] = val;
        else
            fit->second += val;
    }
    for (auto& p : codes)
        p.second = std::log2(p.second);

    fprintf(stderr, "Read %lu codes from the codes file.\n", codes.size());
}

Encoder::Encoder(const std::string& codes_path, bool strip_aux_tags):strip_aux_tags_(strip_aux_tags){
    readCodes(codes_path.c_str(), codes_, strip_aux_tags_);
}

std::vector<std::string>
Encoder::apply(const std::string& word)const{
    vector<string> word_bpes;
    split_word_to_chars(word, word_bpes, !strip_aux_tags_);
    auto subwords = process_bpe_full(word_bpes, codes_);

    auto ret = std::strncmp(subwords[0].data(), kBegWord, kBegWordLength);
    if (ret == 0)
        subwords[0] = subwords[0].substr(kBegWordLength);

    auto last = subwords.size() - 1;
    if(subwords[last].size() > kEndWordLength){
        ret = std::strncmp(subwords[last].data() + subwords[last].size() - kEndWordLength,
                           kEndWord, kEndWordLength);
        if (ret == 0)
            subwords[last] = subwords[last].substr(0, subwords[last].size() - kEndWordLength);
    }
    return subwords;
}

}
