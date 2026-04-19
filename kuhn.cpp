#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <random>
#include <string>
#include <vector>

#define NUM_ACTIONS 2

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution real_dis(0.0f, 1.0f);

struct Tree {
        std::unordered_map<std::string, std::vector<float> > m_regret_sum;
        std::unordered_map<std::string, std::vector<float> > m_strategy_sum;
        std::unordered_map<std::string, std::vector<float> > m_strategy;
};

void update_strategy(std::string& is, Tree& tree) {
        double normalizing_sum = 0;

        for (int a = 0; a < NUM_ACTIONS; a++) {
                tree.m_strategy[is][a] = tree.m_regret_sum[is][a] > 0 ? tree.m_regret_sum[is][a] : 0;
                normalizing_sum += tree.m_strategy[is][a];
        }

        for (int a = 0; a < NUM_ACTIONS; a++) {
                if (normalizing_sum > 0) {
                        tree.m_strategy[is][a] = tree.m_strategy[is][a] / normalizing_sum;
                } else {
                        tree.m_strategy[is][a] = 1.0f / NUM_ACTIONS;
                }
                tree.m_strategy_sum[is][a] += tree.m_strategy[is][a];
        }
}

void initialize(std::string& is, Tree& tree) {
        if (tree.m_regret_sum.find(is) == tree.m_regret_sum.end()) {
                tree.m_regret_sum[is] = std::vector<float>(NUM_ACTIONS, 0.0f);
        }
        if (tree.m_strategy_sum.find(is) == tree.m_strategy_sum.end()) {
                tree.m_strategy_sum[is] = std::vector<float>(NUM_ACTIONS, 0.0f);
        }
        if (tree.m_strategy.find(is) == tree.m_strategy.end()) {
                tree.m_strategy[is] = std::vector<float>(NUM_ACTIONS, 0.0f);
        }
        update_strategy(is, tree);
}

uint32_t get_move(std::string& is, Tree& tree) {
        float choice = real_dis(gen);
        float distribution_sum = 0;
        uint32_t count = 0;

        while (count < NUM_ACTIONS - 1) {
                distribution_sum += tree.m_strategy[is][count];
                if (distribution_sum > choice) {
                        break;
                }
                count++;
        }

        return count;
}

enum class TerminalState {
	NOT_TERMINAL,
	NO_SHOWDOWN,
	SHOWDOWN,
};

TerminalState is_terminal(std::string& is) {
        size_t n = is.length();
	if (n < 2) {
		throw std::logic_error("bad moves \\o/");
	}
	if (isdigit(is[n - 2]) && isdigit(is[n - 1]) && is[n - 2] == is[n - 1]) {
		return TerminalState::SHOWDOWN;
	} else if (isdigit(is[n - 2]) && is[n - 2] != '0' && is[n - 1] == '0') {
		return TerminalState::NO_SHOWDOWN;
	}
	return TerminalState::NOT_TERMINAL;
}

float calc_pot(std::string& is) {

        // TODO
        return 10.0f;
}

float sd_util(std::string& is) {
        float pot = calc_pot(is);
        char card1 = is[0], card2 = is[1];
        if (card1 == card2) {
                return pot / 2.0f;
        } if (card1 > card2) {
                return pot;
        } else {
                return -1.0f * pot;
        }
}

/**
 * @brief
 * Get utility recursively by accumulating weighted utility of child nodes
 * @warning 
 * makes assumption about utility size < MAX_FLOAT
 */
float get_utility(std::string& is, Tree& tree) {

        if (is_terminal(is) == TerminalState::SHOWDOWN) {
                return sd_util(is);
        } else if (is_terminal(is) == TerminalState::NO_SHOWDOWN) {
                return calc_pot(is);
        }

        initialize(is, tree);

        float utility = 0;
        for (size_t a = 0; a < NUM_ACTIONS; a++) {
                std::string action_set = is + std::to_string(a);
                utility += get_utility(action_set, tree) * tree.m_strategy[is][a];
        }

        if (is.length() % 2) {
                utility *= -1;
        }
        return utility;
}

void cfr(std::string& is, Tree& tree) {
        TerminalState term = is_terminal(is);
        if (term == TerminalState::NO_SHOWDOWN || term == TerminalState::SHOWDOWN) {
                return;
        }

        initialize(is, tree);

        uint32_t mymove = get_move(is, tree);
        std::string myset = is + std::to_string(mymove);

        for (size_t a = 0; a < NUM_ACTIONS; a++) {
                std::string otherset = is + std::to_string(a);
                tree.m_regret_sum[is][a] += get_utility(otherset, tree) - get_utility(myset, tree);
                cfr(otherset, tree);
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

std::unordered_map<std::string, std::vector<float> > get_average_strategy(Tree& tree) {
        std::unordered_map<std::string, std::vector<float> > result;
        for (auto [key, value] : tree.m_strategy_sum) {
                result[key] = get_average_strategy_h(value);
        }
        return result;
}

std::unordered_map<std::string, std::vector<float> > train(uint32_t iterations) {
        std::vector<char> deck{'A', 'B', 'C'}; // JACK, QUEEN, KING Respectively, kept as written for ordering
        Tree tree;
        for (uint32_t i = 0; i < iterations; i++) {
                std::shuffle(deck.begin(), deck.end(), gen);
                std::string is(deck.begin(), deck.begin() + 2); // makes string from first two elements of deck
                cfr(is, tree);
        }
        return get_average_strategy(tree);
}

void print_map(std::unordered_map<std::string, std::vector<float> >& to_print) {
        for (auto [key, value] : to_print) {
                std::cout << key << ":";
                for (float val : value) {
                        std::cout << val << ", ";
                }
                std::cout << std::endl;
        }
}

int main(int argc, char** argv) {
        if (argc != 2) {
                return -1;
        }
        int iterations = atoi(argv[1]);
        auto result = train(iterations);
        print_map(result);
        return 0;
}
