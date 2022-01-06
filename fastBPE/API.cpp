#include "API.hpp"
#include "fastBPE.hpp"

namespace fastBPE {



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



double Scorer::score_subwords(const std::vector<subword_t>& subwords)const{
    int n = 0;
    double score = 0.;
    for (const auto& sub : subwords){
        ++n;
        auto sz = token_len(sub.token);
        if(sz < 2 ){
            score += len_score(sz) * LEN_WEIGHT + 0.5 * FREQ_WEIGHT;
            continue;
        }

        auto fscore = freq_score(sub.freq);
        auto lscore = len_score(sz);
        score += FREQ_WEIGHT * fscore + LEN_WEIGHT * lscore;
    }
    score /= n;
    return score;

}


vector<string> select_subwords(vector<process_bpe_state_t>&& states, const Scorer& scorer){
    for(auto& state: states)
        state.score = scorer.score_subwords(state.subwords);

    auto it = std::max_element(std::begin(states), std::end(states),
                               [](const auto& l, const auto& r){return l.score < r.score;});

    vector<string> strs(it->subwords.size());
    std::transform(std::begin(it->subwords), std::end(it->subwords),
                   std::begin(strs), [](const auto& sub) {return sub.token;});


    return strs;

}


constexpr size_t MAX_STATES_COUNT = 15;


vector<process_bpe_state_t>
process_bpe_full(vector<string> &subwords,
                 const unordered_map<tps, double, pair_hash_t> &codes,
                 const Scorer& scorer,
                 int k) {

    auto heap_pred = [](const process_bpe_state_t& l, const process_bpe_state_t& r){
        return l.score > r.score;
    };

    vector<process_bpe_state_t> states;

    vector<process_bpe_state_t> new_states;
    vector<process_bpe_state_t> best_states;
    unordered_set<uint64_t> seen;

    states.emplace_back(std::move(subwords));
    while (true){
        if (states.empty() and new_states.empty())
            break;
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

                if(not inserted)
                    //already seen state
                    continue;

                new_state.score = scorer.score_subwords(new_state.subwords);
                bool added = false;

                if(new_states.size() < MAX_STATES_COUNT){
                    new_states.push_back(std::move(new_state));
                    added = true;
                }
                else if (new_state.score > new_states.front().score){
                    std::pop_heap(new_states.begin(), new_states.end(), heap_pred);
                    new_states.back() = std::move(new_state);
                    added = true;
                }
                if (added)
                    std::push_heap(new_states.begin(), new_states.end(), heap_pred);
            }

            if(final_state){
                if(best_states.size() < k)
                    best_states.push_back(std::move(cur_state));
                else if(cur_state.score > best_states.front().score){
                    std::pop_heap(best_states.begin(), best_states.end(), heap_pred);
                    best_states.back() = std::move(cur_state);
                }
                std::push_heap(best_states.begin(), best_states.end(), heap_pred);
            }

        }
        states = std::move(new_states);
        new_states.clear();
    }

    std::sort_heap(best_states.begin(), best_states.end(), heap_pred);
    return best_states;
}


double readCodes(const char *fp,
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
    double max_freq = 0.;
    for (auto& p : codes){
        p.second = std::log(p.second);
        max_freq = std::max(max_freq, p.second);
    }

    fprintf(stderr, "Read %lu codes from the codes file, max val %f.\n", codes.size(), max_freq);
    return max_freq;
}

Encoder::Encoder(const std::string& codes_path, bool strip_aux_tags):strip_aux_tags_(strip_aux_tags){
    double max_freq = readCodes(codes_path.c_str(), codes_, strip_aux_tags_);
    scorer_ = Scorer(max_freq);
}

void strip_aux_tags(vector<string>& subwords){
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
}

std::vector<Encoder::variant_t>
Encoder::apply(const std::string& word, int k)const{
    vector<string> word_bpes;
    split_word_to_chars(word, word_bpes, !strip_aux_tags_);
    auto best_states = process_bpe_full(word_bpes, codes_, scorer_, k);

    std::vector<Encoder::variant_t> results(best_states.size());
    auto rit = results.begin();
    for(auto& state : best_states){
        rit->subwords.resize(state.subwords.size());

        std::transform(std::begin(state.subwords), std::end(state.subwords),
                       std::begin(rit->subwords), [](const auto& sub) {return sub.token;});
        //TODO should it be conditional based on strip_aux_tags_?
        strip_aux_tags(rit->subwords);
        rit++->score = state.score;
    }


    return results;
}

void Encoder::save(std::ostream &out)const{

    size_t sz = codes_.size();
    out.write((char*)&sz, sizeof(size_t));
    out.write((char*)&strip_aux_tags_, sizeof(bool));
    for (const auto& [p, count] : codes_) {
        out.write(p.first.data(), p.first.size() * sizeof(char));
        out.put(0);
        out.write(p.second.data(), p.second.size() * sizeof(char));
        out.put(0);
        out.write((char*)&count, sizeof(double));
    }
}

void Encoder::load(std::istream &in){
    codes_.clear();
    size_t sz;
    in.read((char*)&sz, sizeof(size_t));
    in.read((char*)&strip_aux_tags_, sizeof(bool));
    double max_freq = 0.;
    for (int32_t i = 0; i < sz; i++) {
        char c;
        pair_t p;
        double count;
        while ((c = in.get()) != 0)
            p.first.push_back(c);
        while ((c = in.get()) != 0)
            p.second.push_back(c);
        in.read((char*)&count, sizeof(double));
        codes_[p] = count;
        max_freq = std::max(max_freq, count);
    }
    scorer_ = Scorer(max_freq);
}

std::vector<std::string> uniq_subwords(const std::vector<Encoder::variant_t>& variants,
                                       size_t min_subword_len){
    std::unordered_set<std::string> seen_subwords;
    for(const auto& var : variants)
        for(const auto& subword : var.subwords){
            if(token_len(subword) >= min_subword_len)
                seen_subwords.insert(subword);
        }
    return std::vector(seen_subwords.begin(), seen_subwords.end());
}
}
