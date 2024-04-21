#include <functional>
#include <set>
#include <string>
#include <vector>
#include <utility>
namespace alg
{
	std::set<std::string> ZBLL = {
		#include "../algs/ZBLLAS.txt"
		#include "../algs/ZBLLS.txt"
		#include "../algs/ZBLLH.txt"
		#include "../algs/ZBLLPi.txt"
		#include "../algs/ZBLLT.txt"
		#include "../algs/ZBLLU.txt"
		#include "../algs/ZBLLL.txt"
	};
	std::set<std::string> CMLL = {
		#include "../algs/CMLL.txt"
	};
	std::set<std::string> F2L = {
		#include "../algs/F2L.txt"
		#include "../algs/AdvancedF2L.txt"
		"" //Pair skip
	};
	std::set<std::string> OLL = {
		#include "../algs/OLL.txt"
	};
	std::set<std::string> PLL = {
		#include "../algs/PLL.txt"
	};

	auto CLLtest = [](Cube const& c) {
		for (auto it : { 1, 2, 3, 4 })
			if (c.mat[it][0][0] != c.mat[it][0][2])
				return false;
		return c.mat[0][0][0] == c.mat[0][0][2] && c.mat[0][0][0] == c.mat[0][2][0] && c.mat[0][0][0] == c.mat[0][2][2];
	};

	auto OLLtest = [](Cube const& c) {
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				if (c.mat[0][i][j] != c.mat[0][1][1])
					return false;
		return true;
	};

	auto PLLtest = [](Cube const& c) {
		for (int i = 1; i <= 4; ++i)
			for (int j = 0; j < 3; ++j)
				if (c.mat[i][0][j] != c.mat[i][1][1])
					return false;
		return true;
	};

	auto LLtest = [](Cube const& c) {
		return OLLtest(c) && PLLtest(c);
	};

	std::string
	brute_force(Cube& c, std::set<std::string> const& algset, state_condition const& goal, std::string const& step_name)
	{
		static std::set<std::string> AUF = { "U", "U2", "U'", "" };
		for (auto it : algset) {
			for (auto pre : AUF) {
				for (auto post : AUF) {
					++cnt;
					auto bak = c;
					auto alg_w_auf = pre + (pre.empty() ? "" : " ") + it+" "+post;
					auto moves = bak.eval(alg_w_auf);
					last_etm += moves;
					if (goal(bak)) {
						c = bak;
						return alg_w_auf + " // " + step_name+"\n";
					}
					last_etm -= moves;
				}
			}
		}
		// Control flow is guaranteed to not reach this point.
		return "";
	}

	inline std::string
	brute_force_f2l(Cube& c)
	{
		static std::set<std::string> AUF = { "U ", "U2 ", "U' ", " " };
		static std::set<std::string> rotations = { "y ", "y2 ", "y' ", " " };
		std::string solve;

		static auto pair_count = [](Cube const& c) {
			int count = 0;
			for (auto it : std::vector<std::pair<int, int> >({ {1, 2}, {2, 3}, {3, 4}, {4, 1} })) {
				auto i = it.first;
				auto j = it.second;
				if (c.mat[i][1][2] == c.mat[i][1][1]
				&&  c.mat[i][2][2] == c.mat[i][1][1]
				&&  c.mat[j][1][0] == c.mat[j][1][1]
				&&  c.mat[j][2][0] == c.mat[j][1][1])
					++count;
			}
			return count;
		};
#define BACKTRACK_F2L(P, A, R, OLD_CUBE, NEW_CUBE, STEP, STMT) \
	for (auto& R : rotations) {\
		for (auto& A : AUF) {\
			for (auto& P : F2L) {\
				Cube NEW_CUBE = OLD_CUBE;\
				auto moves = R+A+P;\
				NEW_CUBE.eval(moves);\
				if (pair_count(NEW_CUBE) > pair_count(OLD_CUBE)) {\
					solve += trim(moves)+" // "+#STEP+"\n";\
					last_etm += wc(moves);\
					STMT;\
				}\
			}\
		}\
	}
		BACKTRACK_F2L(P1, A1, R1, c, copy1, F2L1,
			BACKTRACK_F2L(P2, A2, R2, copy1, copy2, F2L2,
				BACKTRACK_F2L(P3, A3, R3, copy2, copy3, F2L3,
					BACKTRACK_F2L(P4, A4, R4, copy3, copy4, F2L4,
						return solve;
					)
				)
			)
		);
		return "";
	}
};
