#pragma GCC optimize("Ofast")
#include <functional>
#include <string>
#include <iostream>
#include <vector>
#include <cstring>
#include <ctime>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <set>

#include "cube.cpp"
#include "config.hpp"
#define UNKNOWN_MAX 20

int last_etm;
std::atomic<uint_fast64_t> cnt;
typedef std::function<bool(Cube const&)> state_condition;
typedef std::vector<Cube::TurnType> moveset;
typedef std::vector<Cube::TurnType> moves;
typedef std::set<Cube::Facelet> corner;

inline Cube::TurnType undo_move(Cube::TurnType turn)
{
	if ((turn.value+1) % 3 == 0)
		return turn;
	if ((turn.value+1) % 3 == 2)
		return static_cast<Cube::TurnType::Enum>(turn.value-1);
	return static_cast<Cube::TurnType::Enum>(turn.value+1);
}

std::string flatten(moves const& vec, std::string step_name = "", bool count = true)
{
	if (count)
		last_etm += vec.size();
	std::string res;
	for (auto it : vec)
		res += (std::string)it + " ";
	if (res.empty())
		return "";
	std::string ret;
	ret += res + "// " + step_name + " ";
	ret += "(" + std::to_string(vec.size());
	ret += "/" + std::to_string(last_etm) + ")\n";
	return ret;
}

std::string flatten(std::string str, std::string step_name = "", bool count = false)
{
	std::stringstream ss{str};
	moves vec;
	while (ss >> str)
		vec.push_back(str);
	return flatten(vec, step_name, count);
}

int wc(std::string str)
{
	std::stringstream ss{str};
	int count = 0;
	while (ss >> str)
		++count;
	return count;
}

#include "alg.cpp"
std::vector<std::thread> threads;
std::mutex mut;

static inline void DLS(Cube& c, int depth, moveset const& ms, state_condition const& goal,
		moves& curr,
		moves& solution,
		std::atomic<bool>& alarm)
{
	if (depth < 0)
		return;
	if (goal(c)) {
		solution = curr;
		alarm = true;
		return;
	}
	for (auto const& it : ms) {
		if (!curr.empty()) {
			if (it.overall_type() == curr.back().overall_type())
				continue;
			if (curr.size() > 1 && it.overall_type() == curr[curr.size()-2].overall_type() && it.opposite(curr.back()))
				continue;
		}
		c.do_turn(it);
		curr.push_back(it);
		DLS(c, depth-1, ms, goal, curr, solution, alarm);
		c.do_turn(undo_move(it));
		curr.pop_back();
		if (alarm)
			return;
		
		++cnt;
	}
}

static inline moves IDDFS(Cube& c, int max_depth, moveset const& ms, state_condition const& goal, bool thread = use_multithreading)
{
	if (goal(c))
		return {};

	std::atomic<bool> found_solution = false;
	moves last_step_solution = {};

	// Classic IDDFS
	if (!thread) {
		for (int i = 0; i < max_depth; ++i) {
			moves solution, curr;
			DLS(c, i, ms, goal, curr, solution, found_solution);
			if (found_solution && !solution.empty())
				return solution;
		}
	}

	// Multithreaded IDDFS (woaj so fast)
	for (auto& it : ms) {
		threads.emplace_back(std::thread([&]() {
			Cube copy = c;
			moves solution, curr;

			curr.reserve(max_depth);
			curr.push_back(it.value);
			copy.do_turn(it.value);

			for (int i = 0; i < max_depth; ++i)
				DLS(copy, i, ms, goal, curr, solution, found_solution);
			mut.lock();
			if (found_solution && !solution.empty())
				last_step_solution = solution;
			mut.unlock();
		}));
	}
	for (auto& thr : threads)
		thr.join();
	threads.clear();
	return last_step_solution;
}

namespace util
{
	using C = Cube::TurnType::Enum;
	/*********************************
	 *                               *
	 * Utility functions for methods *
	 *                               *
	 *********************************/
	
	bool eo_checker(Cube const& c)
	{
		static auto is_col = [](std::vector<Cube::Facelet> const& f, Cube::Facelet fl1, Cube::Facelet fl2) {
			for (auto it : f)
				if (it == fl1 || it == fl2)
				return true;
			return false;
		};
		if (is_col({
		   c[0][0][1], c[0][1][0], c[0][1][2], c[0][2][1],
		   c[5][0][1], c[5][1][0], c[5][1][2], c[5][2][1],
		   c[2][1][0], c[2][1][2],
		   c[4][1][0], c[4][1][2]
		}, c[1][1][1], c[3][1][1]))
			return false; // EO: Side colour on the top, bottom or front/back two edges.
		if (is_col({
		   c[1][0][1], c[1][1][0], c[1][1][2], c[1][2][1],
		   c[3][0][1], c[3][1][0], c[3][1][2], c[3][2][1],
		   c[2][0][1], c[2][2][1],
		   c[4][0][1], c[4][2][1]
		}, c[0][1][1], c[5][1][1]))
			return false; // EO: Top or bottom colour on the side edges.
		return true;
	};

	std::string solve_right_block (Cube& c, std::string const& step_name)
	{
		moveset ms;
		for (auto p : c.turn)
			if (p.first.is_one_of({ C::R, C::U }))
				ms.push_back(p.first);
		return flatten(IDDFS(c, 14, ms, [](Cube const& c) {
			if (c[2][1][2] != c[2][1][1] || c[4][1][0] != c[4][1][1]
			||  c[2][2][2] != c[2][1][1] || c[4][2][0] != c[4][1][1])
				return false;
			for (int i = 0; i < 3; ++i)
				if (c[5][i][2] != c[5][1][1])
					return false;
			for (int i = 1; i < 3; ++i)
				for (int j = 0; j < 3; ++j)
					if (c[3][i][j] != c[3][1][1])
						return false;
			return true;
		}), step_name);
	};

	static auto block_2x2x2 = [](Cube const& c) {
		return c[1][1][0] == c[1][1][1] && c[1][2][0] == c[1][1][1] && c[1][2][1] == c[1][1][1]
		    && c[4][1][2] == c[4][1][1] && c[4][2][2] == c[4][1][1] && c[4][2][1] == c[4][1][1]
		    && c[5][1][0] == c[5][1][1] && c[5][2][0] == c[5][1][1] && c[5][2][1] == c[5][1][1];
	};	
	static auto block_2x2x1 = [](Cube const& c) {
		return c[3][1][2] == c[3][1][1] && c[3][2][2] == c[3][1][1] && c[3][2][1] == c[3][1][1]
		    && c[4][1][0] == c[4][1][1] && c[4][2][0] == c[4][1][1]
		    && c[5][2][2] == c[5][1][1] && c[5][1][2] == c[5][1][1];
	};
}


namespace solve
{
	using C = Cube::TurnType::Enum;
	/**************************
	 *                        *
	 * Method implementations *
	 *                        *
	 **************************/
	static inline void CFOP(Cube& c)
	{
		//---------
		// 1. Cross
		//---------
		{
			moveset ms;
			for (auto p : c.turn)
				if (!p.first.is_one_of({ C::M, C::S, C::E, C::r, C::l, C::u, C::d, C::f, C::b, C::x, C::y, C::z }))
					ms.push_back(p.first);
			auto solve = IDDFS(c, 8, ms, [](Cube const& c) {
				if (c[5][0][1] != c[5][1][0]
				||  c[5][1][0] != c[5][1][2]
				||  c[5][1][2] != c[5][2][1]
				||  c[5][2][1] != c[5][1][1])
					return false;
				// Matching the centres
				for (int i = 1; i < 5; ++i)
					if (c[i][1][1] != c[i][2][1])
						return false;
				return true;
			});
			c.eval(solve);

			int pc = alg::pair_count(c);
			std::string cross_string;

			// Check for XCrosses
			if (pc == 4)
				cross_string = "F2L";
			else {
				for (int i = 0; i < pc; ++i)
					cross_string += 'X';
				cross_string += "Cross";
			}
			std::cout << flatten(solve, cross_string);
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

	static inline void Roux(Cube& c)
	{
		//------
		// 1. FB
		//------
		{
			moveset ms;
			for (auto p : c.turn)
				if (!p.first.is_one_of({ /*C::M, C::S, C::E,*/ C::r, C::u, C::l, C::d, C::f, C::b, C::x, C::y, C::z }))
					ms.push_back(p.first);
			auto solve = flatten(IDDFS(c, UNKNOWN_MAX, ms, [](Cube const& c) {
				if (c[2][1][0] != c[2][2][0] || c[4][1][2] != c[4][2][2])
					return false;
				for (int i = 0; i < 3; ++i)
					if (c[5][i][0] != c[5][1][0])
						return false;
				for (int i = 1; i < 3; ++i)
					for (int j = 0; j < 3; ++j)
						if (c[1][i][j] != c[1][1][1])
							return false;
				return true;
			}), "FB");
			c.eval(solve);
			std::cout << solve;
		}

		//----------------
		// 2. SB (SS + LP)
		//----------------
		{
			moveset ms;
			for (auto p : c.turn)
				if (p.first.is_one_of({ C::R, C::r, C::U, C::M }))
					ms.push_back(p.first);
			auto solve_1x2x2 = [](Cube const& c) {
				if (c[3][1][1] != c[3][1][2] || c[3][1][1] != c[3][2][1] || c[3][1][1] != c[3][2][2])
					return false; // Side
				if (c[4][1][0] != c[4][1][2] || c[4][2][0] != c[4][1][2])
					return false; // Back
				if (c[5][2][2] != c[5][2][0] || c[5][1][2] != c[5][2][0])
					return false; // Bottom
				return true;
			};
			auto solve_1x1x2 = [](Cube const& c) {
				if (c[3][1][0] != c[3][1][1])
					return false; // Side
				if (c[2][1][2] != c[2][1][0] || c[2][2][2] != c[2][1][0])
					return false; // Front
				if (c[5][0][2] != c[5][0][0])
					return false; // Bottom
				return true;
			};
			auto part1 = flatten(IDDFS(c, UNKNOWN_MAX, ms, [&](Cube const& c) {
				return solve_1x2x2(c);
			}), "SB: SS");
			c.eval(part1);
			std::cout << part1;
			auto part2 = flatten(IDDFS(c, UNKNOWN_MAX, ms, [&](Cube const& c) {
				return solve_1x1x2(c) && solve_1x2x2(c);
			}), "SB: LP");
			c.eval(part2);
			std::cout << part2;

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
							if (c[k][i][j] != c[k][1][0])
								return false;
				return true;
			}), "LSE");
			c.eval(solve);
			std::cout << solve;
		}
	}

	static inline void ZZ(Cube& c)
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
				if (c[2][2][1] != c[2][1][1]
				||  c[5][0][1] != c[5][1][1]
				||  c[5][2][1] != c[5][1][1]
				||  c[4][2][1] != c[4][1][1])
					return false; // Line is not solved;
				return util::eo_checker(c);
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
				if (c[2][1][0] != c[2][1][1] || c[4][1][2] != c[4][1][1]
				||  c[2][2][0] != c[2][1][1] || c[4][2][2] != c[4][1][1])
					return false;
				for (int i = 0; i < 3; ++i)
					if (c[5][i][0] != c[5][1][1])
						return false;
				for (int i = 1; i < 3; ++i)
					for (int j = 0; j < 3; ++j)
						if (c[1][i][j] != c[1][1][1])
							return false;
				return true;
			}), "LB");
			c.eval(solve);
			std::cout << solve;
		} /**************** RIGHT BLOCK ****************/ {
			auto solve = util::solve_right_block(c, "RB");
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

	static inline void Petrus(Cube& c)
	{
		//---------------
		// 1. 2x2x2 (DBL)
		//---------------
		{
			moveset ms;
			for (auto p : c.turn)
				if (!p.first.is_one_of({ C::M, C::S, C::E, C::r, C::u, C::l, C::d, C::f, C::b, C::x, C::y, C::z }))
					ms.push_back(p.first);
			auto solve = flatten(IDDFS(c, UNKNOWN_MAX, ms, util::block_2x2x2), "2x2x2");
			c.eval(solve);
			std::cout << solve;
		}

		//---------
		// 2. 2x2x3
		//---------
		{
			moveset ms;
			for (auto p : c.turn)
				if (p.first.is_one_of({ C::R, C::U, C::F }))
					ms.push_back(p.first);
			auto solve = flatten(IDDFS(c, UNKNOWN_MAX, ms, util::block_2x2x1), "2x2x3");
			c.eval(solve);
			std::cout << solve;
		}

		//------
		// 3. EO
		//------
		{
			moveset ms;
			for (auto p : c.turn)
				if (p.first.is_one_of({ C::R, C::U, C::L, C::F }))
					ms.push_back(p.first);
			auto solve = flatten(IDDFS(c, UNKNOWN_MAX, ms, [](Cube const& c) {
				Cube copy = c;
				copy.do_turn(C::yp);
				if (!util::eo_checker(copy))
					return false;
				return util::block_2x2x2(c) && util::block_2x2x1(c);
			}), "EO");
			c.eval(solve);
			std::cout << solve;
		}

		//-------
		// 4. F2L
		//-------
		{
			c.do_turn(C::yp), std::cout << "y' ";
			auto solve = util::solve_right_block(c, "F2L");
			c.eval(solve);
			std::cout << solve;
		}

		//-----------------
		// 5. COLL, 4. EPLL
		//-----------------
		{
			std::cout << alg::brute_force(c, alg::COLL, alg::CLLtest, "COLL");
			std::cout << alg::brute_force(c, alg::PLL, alg::PLLtest, alg::CLLtest(c) ? "EPLL" : "PLL");
		}
	}

	static inline void _2GR(Cube& c)
	{
		auto eopair = [](Cube const& c) {
			return c[1][2][0] == c[1][1][1] && c[1][2][1] == c[1][1][1]
			&&     c[5][2][0] == c[5][1][1] && c[5][1][0] == c[5][1][1]
			&&     util::eo_checker(c);
		};
		//----------
		// 1. EOPair
		//----------
		{
			moveset ms;
			for (auto p : c.turn)
					if (!p.first.is_one_of({ C::M, C::S, C::E, C::r, C::u, C::l, C::d, C::f, C::b, C::x, C::y, C::z }))
						ms.push_back(p.first);
			auto solve = flatten(IDDFS(c, UNKNOWN_MAX, ms, eopair), "EOPair");
			c.eval(solve);
			std::cout << solve;
		}

		//----------
		// 2. CPLine
		//----------
		{
			moveset ms;
			for (auto p : c.turn)
				if (!p.first.is_one_of({ C::M, C::S, C::E, C::r, C::u, C::l, C::d, C::f, C::b, C::x, C::y, C::z }))
					ms.push_back(p.first);
			auto solve = flatten(IDDFS(c, UNKNOWN_MAX, ms, [&](Cube const& c) {
				if (!eopair(c) || c[1][2][2] != c[1][2][1] || c[5][0][0] != c[5][1][0])
					return false;
				
				static auto corn_check = [](Cube const& c) {
					corner c1 = { c[2][2][2], c[3][2][0], c[5][0][2] };
					corner k1 = { c[2][1][1], c[3][1][1], c[5][1][1] };
					if (c1 != k1)
						return false;
					corner c2 = { c[3][2][2], c[4][2][0], c[5][2][2] };
					corner k2 = { c[3][1][1], c[4][1][1], c[5][1][1] };
					if (c2 != k2)
						return false;
					return true;
				};

				Cube copy = c;
				moveset ms;
				for (auto p : c.turn)
					if (p.first.is_one_of({ C::R, C::U }))
						ms.push_back(p.first);
				auto solve = flatten(IDDFS(copy, UNKNOWN_MAX,  ms, corn_check, false), "", 0);
				copy.eval(solve);
				if (!corn_check(copy))
					return false;

				// Placed down DR corners, now "twist" the remaining ones
				for (int i = 0; i < 4; ++i) {
					bool ok = true;
					corner c1 = { copy[0][0][0], copy[1][0][0], copy[4][0][2] };
					corner k1 = { copy[0][1][1], copy[1][1][1], copy[4][1][1] };
					ok &= (c1 == k1);
					corner c2 = { copy[0][0][2], copy[3][0][2], copy[4][0][0] };
					corner k2 = { copy[0][1][1], copy[3][1][1], copy[4][1][1] };
					ok &= (c2 == k2);
					corner c3 = { copy[0][2][0], copy[1][0][2], copy[2][0][0] };
					corner k3 = { copy[0][1][1], copy[1][1][1], copy[2][1][1] };
					ok &= (c3 == k3);
					corner c4 = { copy[0][2][2], copy[2][0][2], copy[3][0][0] };
					corner k4 = { copy[0][1][1], copy[2][1][1], copy[3][1][1] };
					ok &= (c4 == k4);
					if (ok)
						return true;
					copy.do_turn(C::U);
				}
				return false;
			}), "CPLine");
			c.eval(solve);
			std::cout << solve;
		}

		//---------
		// 3. Block
		//---------
		{
			moveset ms = { C::r2, C::u2, C::R, C::Rp, C::R2, C::U, C::Up, C::U2 };
			auto solve = flatten(IDDFS(c, UNKNOWN_MAX, ms, [](Cube const& c) {
				Cube copy = c;
				copy.do_turn(C::y);
				return util::block_2x2x2(copy) && util::block_2x2x1(copy);
			}), "Block");
			c.eval(solve);
			std::cout << solve;
		}

		//-------
		// 4. F2L
		//-------
		{
			auto solve = util::solve_right_block(c, "F2L");
			c.eval(solve);
			std::cout << solve;
		}

		//--------
		// 5. 2GLL
		//--------
		{
			auto epll = alg::brute_force(c, alg::PLL, alg::PLLtest, "EPLL");
			if (!epll.empty()) {
				std::cout << epll;
				return;
			}
			auto _2gll = alg::brute_force(c, alg::ZBLL, alg::LLtest, "2GLL");
			if (!_2gll.empty()) {
				std::cout << _2gll;
				return;
			}
		}	
	}
}

void usage()
{
	std::cerr << "usage: cubeterm [-v]|[-i]|[-m CFOP|Roux|ZZ|Petrus|2GR][-o <file>] -s <scramble>|-r <random_length> [-t 1|0]\n";
	std::exit(EXIT_FAILURE);
}

std::string randscram(int len)
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

static inline void handle_int(int x)
{
	cont = 1;
}

class RawInputMode
{
private:
	struct termios term, old;
public:
	RawInputMode()
	{
		tcgetattr(STDIN_FILENO, &term);
		old = term;
		term.c_lflag &= ~ICANON;
		term.c_lflag &= ~ECHO;
		tcsetattr(STDIN_FILENO, TCSANOW, &term);
	}

	~RawInputMode()
	{
		tcsetattr(STDIN_FILENO, TCSANOW, &old);
		std::cout << "\033[?25h";
	}
};

void sim(Cube& c)
{
	std::string keyboard = "\n1234567890-=!@#$%^&*()_+\n"
	                       "qwertyuiop[]{}\n"
	                       "asdfghjkl;':\"\n"
	                       "zxcvbnm,./<>?\n\n";
	for (auto const& ch : keyboard) {
		if (ch == '\n')
			std::cout << '\n';
		else if (sim_keys.find(ch) != sim_keys.end())
			std::cout << "\033[1;7m" << ch << " = " << sim_keys.at(ch) << "\033[m  ";
	}
	std::cout << "\033[1;7mCtrl-C = Confirm choice\033[m\n\n"; 
	
	RawInputMode rim;
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
	return;
}

void int_input(Cube& c)
{
	RawInputMode rim;
	char ch;
	signal(SIGINT, handle_int);
	
	std::cout << "\033[?25lControls:\n"
	             "\033[1;7m↑ = Up\033[m "
	             "\033[1;7m← = Left\033[m "
	             "\033[1;7m→ = Right\033[m "
	             "\033[1;7m↓ = Down\033[m\n"
	             "\033[1;7mW = Set to White\033[m\n"
	             "\033[1;7mO = Set to Orange\033[m\n"
	             "\033[1;7mG = Set to Green\033[m\n"
	             "\033[1;7mR = Set to Red\033[m\n"
	             "\033[1;7mB = Set to Blue\033[m\n"
	             "\033[1;7mY = Set to Yellow\033[m\n"
	             "\033[1;7mQ = Confirm choice\033[m\n\n";
	
	int face = 0, i = 0, j = 0;
	c.mark(0, 0, 0);
	for (;;) {
		std::cout << c;
		std::cin >> ch;
		ch = toupper(ch);
		switch (ch) {
			case '\033':
				std::cin >> ch >> ch; // Assume [ch
				switch (toupper(ch)) {
#if defined(_WIN32)
					case 'H': --i; break;
					case 'P': ++i; break;
					case 'K': ++j; break;
					case 'M': --j; break;
#else
					case 'A': --i; break;
					case 'B': ++i; break;
					case 'C': ++j; break;
					case 'D': --j; break;
#endif
				}
				break;
			case 'Q':
				return;
			default:
				c.setcol(face, i, j, ch);
		}
		// Clamp
		switch (face) {
			case 0:
				if (i < 0) i = 0;
				if (j < 0) j = 0;
				if (i > 2) i = 0, face = 2;
				if (j > 2) j = 2;
				break;
			case 1:
				if (i < 0) i = 0;
				if (j < 0) j = 0;
				if (i > 2) i = 2;
				if (j > 2) j = 0, face = 2;
				break;
			case 2:
				if (i < 0) i = 2, face = 0;
				if (j < 0) j = 2, face = 1;
				if (i > 2) i = 0, face = 5;
				if (j > 2) j = 0, face = 3;
				break;
			case 3:
				if (i < 0) i = 0;
				if (j < 0) j = 2, face = 2;
				if (i > 2) i = 2;
				if (j > 2) j = 0, face = 4;
				break;
			case 4:
				if (i < 0) i = 0;
				if (j < 0) j = 2, face = 3;
				if (i > 2) i = 2;
				if (j > 2) j = 2;
				break;
			case 5:
				if (i < 0) i = 2, face = 2;
				if (j < 0) j = 0;
				if (i > 2) i = 2;
				if (j > 2) j = 2;
				break;
		}
		c.mark(face, i, j);
		if (cont)
			break;
		std::cout << "\033[9A\r";
	}
}

int main(int argc, char *argv[])
{
	int opt;
	std::string scram;
	const char *method = default_method;
	bool vc = false, i = false;
	Cube c;
	while ((opt = getopt(argc, argv, "m:s:r:vo:t:i")) != -1) {
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
		case 'i':
			i = true;
			break;
		case 'v':
			vc = true;
			break;
		case 'o':
			{
				int fd = open(optarg, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR);
				if (fd < 0) {
					perror(optarg);
					std::exit(EXIT_FAILURE);
				}
				dup2(fd, STDOUT_FILENO);
			}
			break;
		case 't':
			use_multithreading = std::atoi(optarg);
			break;
		case '?':
			usage();
		}
	}
	c.eval(scram);

	if (vc)
		sim(c);
	else if (i)
		int_input(c);
	else {
		if (argc < 3)
			usage();
		std::cout << "Scramble: " << scram << '\n' << c;
	}

	using namespace std::chrono;
	auto t1 = high_resolution_clock::now();

	if (!strcasecmp(method, "CFOP"))
		solve::CFOP(c);
	else if (!strcasecmp(method, "Roux"))
		solve::Roux(c);
	else if (!strcasecmp(method, "ZZ"))
		solve::ZZ(c);
	else if (!strcasecmp(method, "Petrus"))
		solve::Petrus(c);
	else if (!strcasecmp(method, "2GR"))
		solve::_2GR(c);
	else
		usage();

	// Don't forget the AUF
	std::cout << alg::brute_force(c, {""}, alg::LLtest, "AUF");

	auto t2 = high_resolution_clock::now();
	auto sec = duration<double>(t2-t1);
	std::cout << "\nDone! Time spent finding the solution: ~" << sec.count() << "s";
	std::cout << "\nMethod used: " << method;
	std::cout << "\nEfficiency: " << last_etm << "ETM";
	std::cout << "\nBlasted through: " << cnt << " attempts\n";
	return 0;
}
