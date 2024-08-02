#pragma GCC optimize("Ofast")
#include <algorithm>
#include <functional>
#include <string>
#include <iostream>
#include <vector>
#include <cstring>
#include <ctime>
#include <chrono>

#include <signal.h>
#include <termios.h>
#include <unistd.h>

#include "cube.cpp"
#include "config.hpp"
#define UNKNOWN_MAX 20

int last_etm;
uint_fast64_t cnt;
typedef std::function<bool(Cube const&)> state_condition;
typedef std::vector<Cube::TurnType> moveset;
typedef std::vector<Cube::TurnType> moves;

static inline Cube::TurnType
undo_move(Cube::TurnType turn)
{
	if ((turn.value+1) % 3 == 0)
		return turn;
	if ((turn.value+1) % 3 == 2)
		return static_cast<Cube::TurnType::Enum>(turn.value-1);
	return static_cast<Cube::TurnType::Enum>(turn.value+1);
}

static inline std::string
flatten(moves const& vec, std::string const& step_name = "")
{
	last_etm += vec.size();
	std::string res;
	for (auto it : vec)
		res += (std::string)it + " ";
	return res.empty() ? "" : res + "// " + step_name + "\n";
}

static inline int
wc(std::string str)
{
	std::stringstream ss{str};
	int count = 0;
	while (ss >> str)
		++count;
	return count;
}

static inline std::string
trim(std::string const& str)
{
	bool ok = false;
	std::string result;
	for (auto ch : str) {
		if (!isspace(ch))
			ok = true;
		if (ok)
			result += ch;
	}
	return result;
}

#include "alg.cpp"

namespace solve
{
	moves solution;
	using C = Cube::TurnType::Enum;

	static inline void
	DLS(Cube& c, int depth, moveset& ms, state_condition const& goal, moves& curr)
	{
		if (depth < 0)
			return;
		if (goal(c)) {
			solution = curr;
			return;
		}
		for (auto const& it : ms) {
			if (!curr.empty()) {
				if (it.overall_type() == curr.back().overall_type())
					continue;
				if (curr.size() > 1 && it.overall_type() == curr[curr.size()-2].overall_type() && it.opposite(curr.back()))
					continue;
			}
			++cnt;
			c.do_turn(it);
			curr.push_back(it);
			DLS(c, depth-1, ms, goal, curr);
			c.do_turn(undo_move(it));
			curr.pop_back();
			if (!solution.empty())
				return;
		}
	}

	static inline moves
	IDDFS(Cube& c, int max_depth, moveset& ms, state_condition const& goal)
	{
		if (goal(c))
			return {};
		solution.clear();
		moves curr;
		curr.reserve(max_depth);
		for (int i = 0; i <= max_depth; ++i)
			DLS(c, i, ms, goal, curr);
		return solution;
	}

	static inline void
	CFOP(Cube& c)
	{
		//---------
		// 1. Cross
		//---------
		{
			moveset ms;
			for (auto p : c.turn)
				if (!p.first.is_one_of({ C::M, C::S, C::E, C::r, C::l, C::u, C::d, C::f, C::b, C::x, C::y, C::z }))
					ms.push_back(p.first);
			auto solve = flatten(IDDFS(c, 8, ms, [](Cube const& c) {
				if (c.mat[5][0][1] != c.mat[5][1][0]
				||  c.mat[5][1][0] != c.mat[5][1][2]
				||  c.mat[5][1][2] != c.mat[5][2][1]
				||  c.mat[5][2][1] != c.mat[5][1][1])
					return false;
				// Matching the centres
				for (int i = 1; i < 5; ++i)
					if (c.mat[i][1][1] != c.mat[i][2][1])
						return false;
				return true;
			}), "Cross");
			c.eval(solve);
			std::cout << solve;
		}

		//-------
		// 2. F2L
		//-------
		{
			auto solve = alg::brute_force_f2l(c);
			c.eval(solve);
			std::cout << solve;
		}

		//---------------
		// 3. OLL, 4. PLL
		//---------------
		{	
			std::cout << alg::brute_force(c, alg::OLL, alg::OLLtest, "OLL");
			std::cout << alg::brute_force(c, alg::PLL, alg::PLLtest, "PLL");
		}
	}

	static inline void
	Roux(Cube& c)
	{
		//---------------
		// 1. First block
		//---------------
		{
			moveset ms;
			for (auto p : c.turn)
			if (!p.first.is_one_of({ C::r, C::u, C::l, C::d, C::f, C::b, C::x, C::y, C::z }))
				ms.push_back(p.first);
			auto solve = flatten(IDDFS(c, UNKNOWN_MAX, ms, [](Cube const& c) {
				if (c.mat[2][1][0] != c.mat[2][2][0] || c.mat[4][1][2] != c.mat[4][2][2])
					return false;
				for (int i = 0; i < 3; ++i)
					if (c.mat[5][i][0] != c.mat[5][1][0])
						return false;
				for (int i = 1; i < 3; ++i)
					for (int j = 0; j < 3; ++j)
						if (c.mat[1][i][j] != c.mat[1][1][1])
							return false;
				return true;
			}), "FB");
			c.eval(solve);
			std::cout << solve;
		}

		//----------------
		// 2. Second block
		//----------------
		{
			moveset ms;
			for (auto p : c.turn)
				if (p.first.is_one_of({ C::R, C::r, C::U, C::M }))
					ms.push_back(p.first);
			auto solve = flatten(IDDFS(c, UNKNOWN_MAX, ms, [](Cube const& c) {
				if (c.mat[2][1][2] != c.mat[2][2][2] || c.mat[4][1][0] != c.mat[4][2][0])
					return false;
				for (int i = 0; i < 3; ++i)
					if (c.mat[5][i][2] != c.mat[5][1][0])
						return false;
				for (int i = 1; i < 3; ++i)
					for (int j = 0; j < 3; ++j)
						if (c.mat[3][i][j] != c.mat[3][1][1])
							return false;
				return c.mat[0][1][1] == c.mat[5][1][0] || c.mat[5][1][1] == c.mat[5][1][0];
			}), "SB");
			c.eval(solve);
			std::cout << solve;
		}

		//--------
		// 3. CMLL
		//--------
		{
			std::cout << alg::brute_force(c, alg::CMLL, alg::CLLtest, "CMLL");
		}

		//-------
		// 4. LSE
		//-------
		{
			moveset ms;
			for (auto p : c.turn)
				if (p.first.is_one_of({ C::M, C::U }))
					ms.push_back(p.first);
			auto solve = flatten(IDDFS(c, UNKNOWN_MAX, ms, [](Cube const& c) {
				for (int i = 0; i < 3; ++i)
					for (int j = 0; j < 3; ++j)
						for (auto k : { 0, 2, 4, 5 })
							if (c.mat[k][i][j] != c.mat[k][0][0])
								return false;
				return true;
			}), "LSE");
			c.eval(solve);
			std::cout << solve;
		}
	}

	static inline void
	ZZ(Cube& c)
	{
		//----------
		// 1. EOLine
		//----------
		{
			moveset ms;
			for (auto p : c.turn)
				if (!p.first.is_one_of({ C::M, C::S, C::E, C::r, C::u, C::l, C::d, C::f, C::b, C::x, C::y, C::z }))
					ms.push_back(p.first);
			auto solve = flatten(IDDFS(c, 9, ms, [](Cube const& c) {
			static auto is_col = [&](std::vector<Cube::Facelet> const& f, Cube::Facelet fl1, Cube::Facelet fl2) {
					for (auto it : f)
						if (it == fl1 || it == fl2)
							return true;
					return false;
				};
				if (c.mat[2][2][1] != c.mat[2][1][1]
				||  c.mat[5][0][1] != c.mat[5][1][1]
				||  c.mat[5][2][1] != c.mat[5][1][1]
				||  c.mat[4][2][1] != c.mat[4][1][1])
					return false; // Line is not solved;
				if (is_col({
					 c.mat[0][0][1], c.mat[0][1][0], c.mat[0][1][2], c.mat[0][2][1],
					 c.mat[5][0][1], c.mat[5][1][0], c.mat[5][1][2], c.mat[5][2][1],
					 c.mat[2][1][0], c.mat[2][1][2],
					 c.mat[4][1][0], c.mat[4][1][2]
				}, c.mat[1][1][1], c.mat[3][1][1]))
					return false; // EO: Side colour on the top, bottom or front/back two edges.
				if (is_col({
					 c.mat[1][0][1], c.mat[1][1][0], c.mat[1][1][2], c.mat[1][2][1],
					 c.mat[3][0][1], c.mat[3][1][0], c.mat[3][1][2], c.mat[3][2][1],
					 c.mat[1][0][1], c.mat[1][2][1],
					 c.mat[4][0][1], c.mat[4][2][1]
				}, c.mat[0][1][1], c.mat[5][1][1]))
					return false; // EO: Top or bottom colour on the side edges.
				return true;
			}), "EOLine");
			c.eval(solve);
			std::cout << solve;
		}
		//---------
		// 2. ZZF2L
		//---------
		/**************** LEFT BLOCK ****************/ {
			moveset ms;
			for (auto p : c.turn)
				if (p.first.is_one_of({ C::R, C::U, C::L }))
					ms.push_back(p.first);
			auto solve = flatten(IDDFS(c, 13, ms, [](Cube const& c) {
				if (c.mat[2][1][0] != c.mat[2][1][1] || c.mat[4][1][2] != c.mat[4][1][1]
				||  c.mat[2][2][0] != c.mat[2][1][1] || c.mat[4][2][2] != c.mat[4][1][1])
					return false;
				for (int i = 0; i < 3; ++i)
					if (c.mat[5][i][0] != c.mat[5][1][1])
						return false;
				for (int i = 1; i < 3; ++i)
					for (int j = 0; j < 3; ++j)
						if (c.mat[1][i][j] != c.mat[1][1][1])
							return false;
				return true;
			}), "LB");
			c.eval(solve);
			std::cout << solve;
		} /**************** RIGHT BLOCK ****************/ {
			moveset ms;
			for (auto p : c.turn)
				if (p.first.is_one_of({ C::R, C::U }))
					ms.push_back(p.first);
			auto solve = flatten(IDDFS(c, 14, ms, [](Cube const& c) {
				if (c.mat[2][1][2] != c.mat[2][1][1] || c.mat[4][1][0] != c.mat[4][1][1]
				||  c.mat[2][2][2] != c.mat[2][1][1] || c.mat[4][2][0] != c.mat[4][1][1])
					return false;
				for (int i = 0; i < 3; ++i)
					if (c.mat[5][i][2] != c.mat[5][1][1])
						return false;
				for (int i = 1; i < 3; ++i)
					for (int j = 0; j < 3; ++j)
						if (c.mat[3][i][j] != c.mat[3][1][1])
							return false;
				return true;
			}), "RB");
			c.eval(solve);
			std::cout << solve;
		}
		//--------
		// 3. ZBLL
		//--------
		{
			auto pll = alg::brute_force(c, alg::PLL, alg::PLLtest, "PLL");
			if (!pll.empty()) {
				std::cout << pll;
				return;
			}
			auto zbll = alg::brute_force(c, alg::ZBLL, alg::LLtest, "ZBLL");
			if (!zbll.empty()) {
				std::cout << zbll;
				return;
			}
		}
	}
}

static inline void
usage()
{
	std::cerr << "usage: cubeterm [-v]|[-m CFOP|Roux|ZZ] -s <scramble>|-r <random_length>\n";
	std::exit(EXIT_FAILURE);
}

static inline std::string
randscram(int len)
{
	// Generates a random scramble.
	std::srand(std::time(NULL));
	std::string s;
	auto last = 0, turn = 0;
	for (int i = 0; i < len; ++i) {
		do turn = std::rand() % 6 * 3; while (turn == last);
		last = turn;
		s += std::string(Cube::TurnType(turn+(std::rand() % 3)));
		if (i < len-1)
			s += " ";
	}
	return s;
}


/* Simulator :) */
volatile sig_atomic_t cont;

static inline void
handle_int(int x)
{
	cont = 1;
}

static inline void
sim(Cube& c)
{
	std::string keyboard = "\n1234567890\n"
	                       "qwertyuiop\n"
                         "asdfghjkl;\n"
                         "zxcvbnm,./\n\n";

	std::cout << "\033[?25lControls:";
	for (auto const& ch : keyboard) {
		if (ch == '\n')
			std::cout << '\n';
		else if (sim_keys.find(ch) != sim_keys.end())
			std::cout << "\033[1;7m" << ch << " = " << sim_keys.at(ch) << "\033[m  ";
	}
	
	struct termios term, old;
	tcgetattr(STDIN_FILENO, &term);
	old = term;
	term.c_lflag &= ~ICANON;
	term.c_lflag &= ~ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &term);
	signal(SIGINT, handle_int);

	char ch;
	for (;;) {
		std::cout << c;
		std::cin >> ch; ch = std::tolower(ch);
		if (sim_keys.find(ch) != sim_keys.end())
			c.eval(sim_keys.at(ch));

		if (cont)
			break;
		std::cout << "\033[9A\r";
	}
	tcsetattr(STDIN_FILENO, TCSANOW, &old);
	std::cout << "\033[?25h";
	return;
}

int
main(int argc, char *argv[])
{
	int opt;
	std::string scram, method = default_method;
	bool vc = false;
	Cube c;
	while ((opt = getopt(argc, argv, "m:s:r:v")) != -1) {
		switch (opt) {
		case 's':
			scram = optarg;
			break;
		case 'm':
			method = optarg;
			break;
		case 'r':
			scram = randscram(std::atoi(optarg));
			break;
		case 'v':
			vc = true;
			break;
		case '?':
			usage();
		}
	}
	c.eval(scram);

	if (vc) {
		sim(c);
		return 0;
	}
	if (argc < 3)
		usage();
	
	std::cout << "Scramble: " << scram << '\n' << c;
	
	using namespace std::chrono;
	auto t1 = high_resolution_clock::now();

	if (method == "CFOP")
		solve::CFOP(c);
	if (method == "Roux")
		solve::Roux(c);
	if (method == "ZZ")
		solve::ZZ(c);

	auto t2 = high_resolution_clock::now();
	auto sec = duration<double>(t2-t1);
	std::cout << "\nDone! Time spent finding the solution: ~" << sec.count() << "s";
	std::cout << "\nMethod used: " << method;
	std::cout << "\nEfficiency: " << last_etm << "ETM";
	std::cout << "\nBlasted through: " << cnt << " attempts\n";
	return 0;
}
