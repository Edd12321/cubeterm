#include <set>
#include <string>
#include <vector>
#include <utility>
namespace alg
{
	struct WcComparator {
		inline bool operator()(std::string const& lhs, std::string const& rhs) const
		{
			auto wc_l = wc(lhs);
			auto wc_r = wc(rhs);
			if (wc_l == wc_r)
				return lhs < rhs;

			return wc_l < wc_r;
		}
	};

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
	std::set<std::string, WcComparator> F2L = {
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
	std::set<std::string> COLL = {
		#include "../algs/COLL.txt"
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

	std::string brute_force(Cube& c, std::set<std::string> const& algset, state_condition const& goal, std::string const& step_name)
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

	inline std::string brute_force_f2l(Cube& c)
	{
		static std::set<std::string> AUF = { " ", "U ", "U' ", "U2 " };
		static std::set<std::string> rotations = { " ", "y ", "y' ", "y2 " };
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

		// Display cross step not shown before
		int pc = pair_count(c);
		if (pc == 4)
			std::cout << "F2L\n";
		else {
			for (int i = 0; i < pc; ++i)
				std::cout << 'X';
			std::cout << "Cross\n";
		}
#define BACKTRACK_F2L(OLD_CUBE, NEW_CUBE, STMT) {                                                                             \
                                                _try_again_##OLD_CUBE:                                                        \
                                                          bool ok = false;                                                    \
                                                          if (pair_count(OLD_CUBE) == 4)                                      \
                                                            goto _skip;                                                       \
                                                          for (auto& R : rotations) {                                         \
                                                            for (auto& A : AUF) {                                             \
                                                              for (auto& P : F2L) {                                           \
                                                                Cube NEW_CUBE = OLD_CUBE;                                     \
                                                                auto moves = R+A+P;                                           \
                                                                NEW_CUBE.eval(moves);                                         \
                                                                int npc = pair_count(NEW_CUBE);                               \
                                                                if (npc > pc) {                                               \
                                                                  ok = true, pc = npc;                                        \
                                                                  solve += trim(moves)+" // F2L" + std::to_string(pc) +"\n";  \
                                                                  last_etm += wc(moves);                                      \
                                                                  STMT;                                                       \
                                                                }                                                             \
                                                              }                                                               \
                                                            }                                                                 \
                                                          }                                                                   \
                                                          /* SCDB has no algs for 2 trapped pieces in different slots */      \
                                                          if (!ok) {                                                          \
                                                            for (auto& R : rotations) {                                       \
                                                              Cube NEW_CUBE = OLD_CUBE;                                       \
                                                              auto moves = R + "R U R'";                                      \
                                                              NEW_CUBE.eval(moves);                                           \
                                                              if (pair_count(NEW_CUBE) == pc) {                               \
                                                                OLD_CUBE = NEW_CUBE;                                          \
                                                                solve += trim(moves) + ' ';                                   \
                                                                goto _try_again_##OLD_CUBE;                                   \
                                                              }                                                               \
                                                            }                                                                 \
                                                          }                                                                   \
                                                        }
		BACKTRACK_F2L(c, copy1,
			BACKTRACK_F2L(copy1, copy2,
				BACKTRACK_F2L(copy2, copy3,
					BACKTRACK_F2L(copy3, copy4,
						goto _skip;
					)
				)
			)
		);
_skip:
		return solve;
	}
};
