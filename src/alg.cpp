#include <algorithm>
#include <set>
#include <string>
#include <vector>
#include <utility>
#include <functional>
namespace alg {
	struct WcComparator {
		inline bool operator()(std::string const& lhs, std::string const& rhs) const {
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
	std::set<std::string> TDR = {
		#include "../algs/TDR.txt"
	};

	auto CLLtest = [](Cube const& c) {
		for (auto it : { 1, 2, 3, 4 })
			if (c[it][0][0] != c[it][0][2])
				return false;
		return c[0][0][0] == c[0][0][2] && c[0][0][0] == c[0][2][0] && c[0][0][0] == c[0][2][2];
	};

	auto OLLtest = [](Cube const& c) {
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				if (c[0][i][j] != c[0][1][1])
					return false;
		return true;
	};

	auto PLLtest = [](Cube const& c) {
		for (int i = 1; i <= 4; ++i)
			for (int j = 0; j < 3; ++j)
				if (c[i][0][j] != c[i][1][1])
					return false;
		return true;
	};

	auto LLtest = [](Cube const& c) {
		return OLLtest(c) && PLLtest(c);
	};
	
	auto TDRtest = [](Cube const& c) {
		return c[2][2][2] == c[2][2][1] && c[5][0][2] == c[5][0][1]
		    && c[4][2][2] == c[4][2][1] && c[5][2][2] == c[5][2][1]
		    && c[5][1][1] == c[5][1][2] && c[3][2][1] == c[3][2][2];
	};

	auto pair_count = [](Cube const& c) {
		int count = 0;
		for (auto it : std::vector<std::pair<int, int> >({ {1, 2}, {2, 3}, {3, 4}, {4, 1} })) {
			auto i = it.first;
			auto j = it.second;
			if (c[i][1][2] == c[i][1][1]
			&&  c[i][2][2] == c[i][2][1]
			&&  c[j][1][0] == c[j][1][1]
			&&  c[j][2][0] == c[j][2][1])
				++count;
		}
		return count;
	};

	std::string brute_force(Cube& c, std::set<std::string> const& algset, state_condition const& goal, std::string const& step_name, bool auf = true) {
		if (goal(c))
			return "";
		std::set<std::string, WcComparator> AUF = {""};
		if (auf) AUF = {"", "U", "U2", "U'"};
		for (auto it : algset) {
			for (auto pre : AUF) {
				for (auto post : AUF) {
					++cnt;
					auto bak = c;
					auto alg_w_auf = pre + std::string(" ") + it+" "+post;
					auto moves = bak.eval(alg_w_auf);
					last_etm += moves;
					if (goal(bak)) {
						c = bak;
						return flatten(alg_w_auf, step_name);
					}
					last_etm -= moves;
				}
			}
		}
		// Control flow is guaranteed to not reach this point.
		return "";
	}

	inline std::string brute_force_f2l(Cube c) {
		static std::set<std::string> AUF = { " ", "U ", "U' ", "U2 " };
		static std::set<std::string> ADF = { " ", "D ", "D' ", "D2 " };
		static std::set<std::string> rotations = { " ", "y ", "y' "/*, "y2 " nah */ };

		struct Result {
			int etm;
			std::string sol;
		};
		std::function<Result(int, int, Cube const&, std::string)> greedy_rec;
		
		greedy_rec = [&](int pc, int curr_etm, Cube const& c, std::string solve) {
			struct Candidate {
				std::string sol;
				int pcc, mvc;
				bool pseudo;
			};
			std::vector<Candidate> cands; 
			Cube k;
			if (pc == 4) return Result{curr_etm, solve};
			for (auto const& auf : AUF)
			for (auto const& adf : ADF)
			for (auto const& pair : F2L)
			for (auto const& rot : rotations) {
				if (pair.empty())
					continue;
				std::string sol = rot + adf + auf + pair;
				k = c;
				k.eval(sol);
				int pcc = pair_count(k) - pc, mvc = wc(sol);
				bool pseudo = adf[0] == 'D';
				if (pcc <= 0) continue;
				cands.push_back({sol, pcc, mvc, pseudo});
			}
			std::sort(cands.begin(), cands.end(), [](Candidate const& lhs, Candidate const& rhs) {
				if (lhs.pcc != rhs.pcc)
					return lhs.pcc > rhs.pcc;
				return lhs.mvc < rhs.mvc;
			});
			int min_mvc = 100;
			std::string best_sol;
			for (size_t i = 0; i < std::min(F2L_BRANCH, cands.size()); ++i) {
				int mvc = wc(cands[i].sol);
				k = c;
				k.eval(cands[i].sol);
				int new_pc = pair_count(k);
				std::string step = "F2L";
				if (new_pc - pc < 2)
					step += std::to_string(new_pc);
				else step += std::to_string(new_pc-1) + "&" + std::to_string(new_pc);
				if (cands[i].pseudo)
					step = "Pseudo " + step;
				last_etm += mvc;
				Result r = greedy_rec(new_pc, curr_etm + mvc, k, solve + flatten(cands[i].sol, step));
				last_etm -= mvc;
				if (r.etm < min_mvc) {
					min_mvc = r.etm;
					best_sol = r.sol;
				}
			}
			return Result{min_mvc, best_sol};
		};
		auto found = greedy_rec(0, 0, c, "");
		last_etm += found.etm;
		return found.sol;
	}
};
