#include <iostream>
#include <vector>
#include <random>

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution dis(0.0f, 1.0f);

const int ROCK = 0, PAPER = 1, SCISSORS = 2, NUM_ACTIONS = 3;
std::vector<float> regret_sum(NUM_ACTIONS, 0.0f);
std::vector<float> strategy_sum(NUM_ACTIONS, 0.0f);
std::vector<float> strategy(NUM_ACTIONS, 0.0f);

std::vector<float> get_strategy() {
	float normalizing_sum = 0.0f;
	for (int a = 0; a < NUM_ACTIONS; a++) {
		strategy[a] = regret_sum[a] > 0 ? regret_sum[a] : 0;
		normalizing_sum += strategy[a];
	}
	for (int a = 0; a < NUM_ACTIONS; a++) {
		if (normalizing_sum > 0) {
			strategy[a] /= normalizing_sum;
		} else {
			strategy[a] = 1.0f / NUM_ACTIONS;
		}
		strategy_sum[a] += strategy[a];
	}
	return strategy;
}

int get_move(std::vector<float> strategy) {
	float cumulative_distribution = 0.0f;
	int a = 0;
	float r = dis(gen);
	while (a < NUM_ACTIONS - 1) {
		cumulative_distribution += strategy[a];
		if (r < cumulative_distribution) {
			break;
		}
		a++;
	}
	return a;
}

void train(int iterations) {
	std::vector<float> utilities(NUM_ACTIONS, 0.0f);
	for (int i = 0; i < iterations; i++) {
		strategy = get_strategy();
		int my_move = get_move(strategy);
		int opp_move = get_move(strategy);
		
		utilities[opp_move] = 0;
		utilities[opp_move == NUM_ACTIONS - 1 ? 0 : opp_move + 1] = 1;
		utilities[opp_move == 0 ? NUM_ACTIONS - 1 : opp_move - 1] = -1;

		for (int a = 0; a < NUM_ACTIONS; a++) {
			regret_sum[a] += utilities[a] - utilities[my_move];
		}
	}
}

std::vector<float> get_avg_strategy() {
	std::vector<float> avg_strategy(NUM_ACTIONS, 0.0f);
	float normalizing_sum = 0.0f;
	for (int a = 0; a < NUM_ACTIONS; a++) {
		normalizing_sum += strategy_sum[a];
	}
	for (int a = 0; a < NUM_ACTIONS; a++) {
		avg_strategy[a] = strategy_sum[a] / normalizing_sum;
	}
	return avg_strategy;
}

int main(int argc, char** argv) {
	if (argc != 2) return -1;
	int iters = atoi(argv[1]);
	train(iters);
	std::vector<float> avg_strategy = get_avg_strategy();
	std::cout << "ROCK: " << avg_strategy[0] << "\n";
	std::cout << "PAPER: " << avg_strategy[1] << "\n";
	std::cout << "SCISSORS: " << avg_strategy[2] << "\n";
	return 0;
}
