#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#define NUM_ACTIONS 2

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution real_dis(0.0f, 1.0f);

enum class TerminalState {
        SHOWDOWN,
        RELINQUISHED,
        NOT_TERMINAL
};

struct InformationSet {
        std::string full;
        std::string p1;
        std::string p2;

        explicit InformationSet(char c1, char c2) {
                full += c1;
                full += c2;
                p1 += c1;
                p2 += c2;
        }

        explicit InformationSet(std::string s) {
                full = s;
                set_from_full();
        }

        void add(char a) {
                full += a;
                p1 += a;
                p2 += a;
        }

        void add(const std::string& a) {
                full += a;
                p1 += a;
                p2 += a;
        }

        void add(int a) {
                assert(a >= 0 && a < 10);
                char c = static_cast<char>(a + '0');
                full += c;
                p1 += c;
                p2 += c;
        }

        std::string get_action_seq() const {
                return full.substr(2, full.length());
        }

        std::string player(bool player) const {
                return player ? p1 : p2;
        }

        void set_from_full() {
                assert(full.length() == 2 && p1.length() == 0 && p2.length() == 0);
                p1 += full[0];
                p2 += full[1];
        }

        template <typename T>
        InformationSet& operator+=(T a) { add(a); return *this; }
};

TerminalState is_terminal(const InformationSet& is) {
        size_t n = is.full.length();
        if (n <= 2) {
                return TerminalState::NOT_TERMINAL;
        } else if (isdigit(is.full[n - 2]) && isdigit(is.full[n - 1]) && is.full[n - 2] != '0' && is.full[n - 1] == '0') {
                return TerminalState::RELINQUISHED;
        } else if (isdigit(is.full[n - 2]) && isdigit(is.full[n - 1]) && is.full[n - 2] == is.full[n - 1]) {
                return TerminalState::SHOWDOWN;
        }
        return TerminalState::NOT_TERMINAL;
}

struct Tree {
        std::unordered_map<std::string, std::vector<float> > m_regret_sum;
        std::unordered_map<std::string, std::vector<float> > m_strategy_sum;
        std::unordered_map<std::string, std::vector<float> > m_strategy;

        bool is_initialized_ = false;

        void initialize(InformationSet is) {

                if (is_terminal(is) != TerminalState::NOT_TERMINAL) {
                        return;
                }

                for (int b = 0; b < 2; b++) {
                        m_regret_sum[is.player(b)] = std::vector<float>(NUM_ACTIONS, 0.0f);
                        m_strategy_sum[is.player(b)] = std::vector<float>(NUM_ACTIONS, 0.0f);
                        m_strategy[is.player(b)] = std::vector<float>(NUM_ACTIONS, 0.0f);
                }

                for (int a = 0; a < NUM_ACTIONS; a++) {
                        InformationSet newis = is;
                        newis.add(a);
                        initialize(newis);
                }

                is_initialized_ = true;
        }
};

Tree tree;

void update_strategy(const InformationSet& is, bool is_p1) {
        double normalizing_sum = 0;
        std::string pis = is.player(is_p1);

        for (int a = 0; a < NUM_ACTIONS; a++) {
                tree.m_strategy[pis][a] = tree.m_regret_sum[pis][a] > 0 ? tree.m_regret_sum[pis][a] : 0;
                normalizing_sum += tree.m_strategy[pis][a];
        }

        for (int a = 0; a < NUM_ACTIONS; a++) {
                if (normalizing_sum > 0) {
                        tree.m_strategy[pis][a] = tree.m_strategy[pis][a] / normalizing_sum;
                } else {
                        tree.m_strategy[pis][a] = 1.0f / NUM_ACTIONS;
                }
                tree.m_strategy_sum[pis][a] += tree.m_strategy[pis][a];
        }
}

float get_bet_size(char bet) {
        switch (bet) {
                case '0': return 0.0f;
                case '1': return 1.0f;
                default:
                        throw std::logic_error("unsupported betsize: " + std::string(1, bet));
        }
}

std::pair<float, float> get_contributions(const InformationSet& is) {
    float p1c = 1.0f, p2c = 1.0f;
    std::string actions = is.get_action_seq();
    for (size_t i = 0; i < actions.length(); i++) {
        if (i % 2 == 0) {
                p1c += get_bet_size(actions[i]);
        } else {
                p2c += get_bet_size(actions[i]);
        }
    }
    return {p1c, p2c};
}

float calc_pot(const InformationSet& is) {
        float pot = 2.0f;
        std::string actions = is.get_action_seq();
        for (char c : actions) {
                pot += get_bet_size(c);
        }
        return pot;
}

float calc_showdown_pot(const InformationSet& is) {
        auto [p1c, p2c] = get_contributions(is);
        if (is.p1[0] > is.p2[0]) {
                return p2c; // what player 2 gave me
        } else if (is.p1[0] < is.p2[0]) { 
                return -p1c; // what i gave player 2
        }
        return 0;
}

/**
 * @brief
 * Gets the utility at a particular node
 */
float get_utility(const InformationSet& is, bool is_p1) {

        if (is_terminal(is) == TerminalState::RELINQUISHED) {
                auto [p1c, p2c] = get_contributions(is);
                return is_p1 ? p2c : -p1c;
        } else if (is_terminal(is) == TerminalState::SHOWDOWN) {
                return calc_showdown_pot(is);
        }

        float utility = 0;

        assert(tree.is_initialized_);

        for (int a = 0; a < NUM_ACTIONS; a++) {
                InformationSet newis = is;
                newis.add(a);
                utility += get_utility(newis, !is_p1) * tree.m_strategy[is.player(is_p1)][a];
        }

        return utility;
}

int get_action(const InformationSet& is, bool is_p1) {
        float choice = real_dis(gen);
        float distribution_sum = 0.0f;
        int count = 0;

        while (count < NUM_ACTIONS - 1) {
                distribution_sum += tree.m_strategy[is.player(is_p1)][count];
                if (choice < distribution_sum) {
                        break;
                }
                count++;
        }

        return count;
}

void cfr(const InformationSet& is, bool is_p1) {
        if (is_terminal(is) != TerminalState::NOT_TERMINAL) {
                return;
        }

        float node_utility = get_utility(is, is_p1);

        for (int a = 0; a < NUM_ACTIONS; a++) {
                InformationSet isa = is;
                isa.add(a);
                float action_util = get_utility(isa, is_p1);
                tree.m_regret_sum[is.player(is_p1)][a] += is_p1 ?
                        (action_util - node_utility) :
                        (node_utility - action_util);
                cfr(isa, !is_p1);
        }

        update_strategy(is, is_p1);
}

void train(int iterations) {
        std::vector<char> deck{'A', 'B', 'C'};
        for (size_t i = 0; i < deck.size() - 1; i++) {
                for (size_t j = i + 1; j < deck.size(); j++) {
                        tree.initialize(InformationSet(deck[i], deck[j]));
                }
        }
        for (int i = 0; i < iterations; i++) {
                std::shuffle(deck.begin(), deck.end(), gen);
                InformationSet is(std::string(deck.begin(), deck.begin() + 2));
                cfr(is, true);
        }
}

std::vector<float> get_average_strategy_h(std::vector<float>& strategy_sum) {
        double normalizing_sum = 0;
        std::vector<float> avg_strategy(NUM_ACTIONS, 0.0f);
        for (uint32_t a = 0; a < NUM_ACTIONS; a++) {
                avg_strategy[a] = strategy_sum[a] > 0 ? strategy_sum[a] : 0;
                normalizing_sum += avg_strategy[a];
        }
        for (uint32_t a = 0; a < NUM_ACTIONS; a++) {
                if (normalizing_sum > 0) {
                        avg_strategy[a] = avg_strategy[a] / normalizing_sum;
                } else {
                        avg_strategy[a] = 1.0f / NUM_ACTIONS;
                }
        }
        assert(avg_strategy.size() == 2);
        return avg_strategy;
}

std::unordered_map<std::string, std::vector<float> > get_average_strategy() {
        std::unordered_map<std::string, std::vector<float> > result;
        for (auto [key, value] : tree.m_strategy_sum) {
                result[key] = get_average_strategy_h(value);
        }
        return result;
}

float my_round(float val) {
        if (val + 0.01f > 1.0f) return 1.0f;
        if (val - 0.01f < 0.0f) return 0.0f;
        return val;
}

void print_map(std::unordered_map<std::string, std::vector<float> >& to_print) {
        for (auto [key, value] : to_print) {
                std::cout << key << ":";
                for (float val : value) {
                        std::cout << my_round(val) << ", ";
                }
                std::cout << std::endl;
        }
}

int main(int argc, char** argv) {
        if (argc != 2) {
                std::cout << "usage: ./<prog> <iters>";
                return -1;
        }
        int iterations = atoi(argv[1]);
        train(iterations);
        auto map = get_average_strategy();
        print_map(map);
        return 0;
}
