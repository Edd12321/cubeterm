#include <map>
#include <string>
#include <sstream>
#include <ostream>
#include <iomanip>
#include <iostream>
#include <array>
#include <cinttypes>

class Cube
{
public:
	
	enum Facelet : std::int_fast8_t { WHITE, ORANGE, GREEN, RED, BLUE, YELLOW };
	enum Side    : std::int_fast8_t { U, L, F, R, B, D };

	/* Store moves as one byte each */
	class TurnType
	{
	public:
		enum Enum : std::int_fast8_t {
			R, Rp, R2, L, Lp, L2, U, Up, U2, D, Dp, D2, F, Fp, F2, B, Bp, B2,
			r, rp, r2, l, lp, l2, u, up, u2, d, dp, d2, f, fp, f2, b, bp, b2,
			M, Mp, M2, S, Sp, S2, E, Ep, E2,
			x, xp, x2, y, yp, y2, z, zp, z2
		} value;
		bool
		is_one_of(std::vector<Enum> const& turns) const
		{
			for (auto it : turns)
				if (value-it >= 0 && value-it <= 2)
					return true;
			return false;
		}

		// The same kinds of moves: R,R',R2
		inline int overall_type() const { return value / 3 * 3; }
		// Comparison
		inline bool operator>(const TurnType& t) const { return value > t.value; }
		inline bool operator<(const TurnType& t) const { return value < t.value; }

		inline bool
		opposite(TurnType const& t2) const
		{
			auto T1 = overall_type(), T2 = t2.overall_type();
			if (T1 > T2)
				std::swap(T1, T2);
			return (T1 == R && T2 == L) || (T1 == U && T2 == D) || (T1 == F && T2 == B);
		}

		operator std::string() const
		{
#define T2STR(X) { X, #X }, { X##p, #X "'" }, { X##2, #X "2" },
			static std::map<Enum, std::string> turn2str = {
				T2STR(R) T2STR(L) T2STR(U) T2STR(D) T2STR(F) T2STR(B)
				T2STR(r) T2STR(l) T2STR(u) T2STR(d) T2STR(f) T2STR(b)
				T2STR(M) T2STR(S) T2STR(E)
				T2STR(x) T2STR(y) T2STR(z)
			};
			return turn2str[value];
		}
		template<typename T>
		inline TurnType(T const& t)
		{
			this->value = static_cast<Enum>(t);
		}
		TurnType(std::string const& str)
		{
#define STR2T(X) { #X, X }, { #X "'", X##p }, { #X "2", X##2 },
			static std::map<std::string, Enum> str2turn = {
				STR2T(R) STR2T(L) STR2T(U) STR2T(D) STR2T(F) STR2T(B)
				STR2T(r) STR2T(l) STR2T(u) STR2T(d) STR2T(f) STR2T(b)
				STR2T(M) STR2T(S) STR2T(E)
				STR2T(x) STR2T(y) STR2T(z)
			};
			this->value = str2turn[str];
		}
		~TurnType(){}
	};
private:
	// <=> std::int_fast8_t old[6][3][3];
	using Face = std::array<std::array<Facelet, 3>, 3>;
	using CubeRaw = std::array<Face, 6>;
	CubeRaw old;
	const char *ansicol[6] = {
		"\33[107m  \33[0m", // White
		"\33[101m  \33[0m", // Orange
		"\33[42m  \33[0m",  // Green
		"\33[41m  \33[0m",  // Red
		"\33[44m  \33[0m",  // Blue
		"\33[103m  \33[0m"  // Yellow
	};
	inline void rot90cw(Face&);
	inline void rot90acw(Face&);
	inline void rot180(Face&);

public:
	// <=> std::int_fast8_t mat[6][3][3];
	CubeRaw mat;
	
	int eval(std::string const&);
	inline void do_turn(TurnType const&);
	friend std::ostream& operator <<(std::ostream&, const Cube&);
	Cube();
	~Cube();

/* Syntax:
	"Define a move X,
		which cycles the pieces CYC4->CYC3->CYC2->CYC1",
	"Define a move Xprime,
		which cycles the pieces CYC1->CYC2->CYC3->CYC4",
	"Define a move X2,
		which swaps CYC1,CYC3 and cycles CYC2,CYC4".
*/
#define SINGLE(X)    X##single
#define PRIME(X)     X##prime
#define DUB(X)       X##2

#define CYCLE(CYC1, CYC2, CYC3, CYC4) for (int i = 0; i < 3; ++i) mat CYC1 = old CYC2, mat CYC2 = old CYC3, mat CYC3 = old CYC4, mat CYC4 = old CYC1; 

#define MOVE(X, CYC1, CYC2, CYC3, CYC4) \
	void  SINGLE(X)() { old = mat; this->rot90cw (mat[X]); CYCLE(CYC1, CYC2, CYC3, CYC4) }\
	void   PRIME(X)() { old = mat; this->rot90acw(mat[X]); CYCLE(CYC4, CYC3, CYC2, CYC1) }\
	void     DUB(X)() { /* ... */; this->rot180  (mat[X]); for (int i = 0; i < 3; ++i) std::swap(mat CYC1, mat CYC3), std::swap(mat CYC2, mat CYC4); }

#define SLICE_MOVE(X, CYC1, CYC2, CYC3, CYC4) \
	void  SINGLE(X)() { old = mat; CYCLE(CYC1, CYC2, CYC3, CYC4) }\
	void   PRIME(X)() { old = mat; CYCLE(CYC4, CYC3, CYC2, CYC1) }\
	void     DUB(X)() { /* ... */; for (int i = 0; i < 3; ++i) std::swap(mat CYC1, mat CYC3), std::swap(mat CYC2, mat CYC4); }

#define CMPND_MOVE(X, OUTER, INNER, INNERPRIME, INNERDUB) \
	void SINGLE(X)() { this->do_turn(TurnType::Enum::OUTER   ); this->do_turn(TurnType::Enum::INNER     ); }\
	void  PRIME(X)() { this->do_turn(TurnType::Enum::OUTER##p); this->do_turn(TurnType::Enum::INNERPRIME); }\
	void    DUB(X)() { this->do_turn(TurnType::Enum::OUTER##2); this->do_turn(TurnType::Enum::INNERDUB  ); }

/* Syntax:
	 "Add the move X to the dispatch table".
*/
#define MOVE_TABLE(X) \
	{ TurnType::Enum::X,    &Cube::SINGLE(X) },\
	{ TurnType::Enum::X##p, &Cube::PRIME (X) },\
	{ TurnType::Enum::X##2, &Cube::DUB   (X) },

	// RULDFB
	MOVE(R, [U][i][2], [F][i][2], [D][i][2], [B][2-i][0])
	MOVE(L, [F][i][0], [U][i][0], [B][2-i][2], [D][i][0])
	MOVE(U, [F][0][i], [R][0][i], [B][0][i], [L][0][i])
	MOVE(D, [R][2][i], [F][2][i], [L][2][i], [B][2][i])
	MOVE(F, [U][2][i], [L][2-i][2], [D][0][2-i], [R][i][0])
	MOVE(B, [U][0][2-i], [R][2-i][2], [D][2][i], [L][i][0])
	// MSE
	SLICE_MOVE(M, [D][i][1], [F][i][1], [U][i][1], [B][2-i][1])
	SLICE_MOVE(S, [D][1][i], [R][2-i][1], [U][1][2-i], [L][i][1])
	SLICE_MOVE(E, [F][1][i], [L][1][i], [B][1][i], [R][1][i])
	// rludfb
	CMPND_MOVE(r, R, Mp, M, M2)
	CMPND_MOVE(l, L, M, Mp, M2)
	CMPND_MOVE(u, U, Ep, E, E2)
	CMPND_MOVE(d, D, E, Ep, E2)
	CMPND_MOVE(f, F, S, Sp, S2)
	CMPND_MOVE(b, B, Sp, S, S2)
	// xyz
	CMPND_MOVE(x, R, lp, l, l2)
	CMPND_MOVE(y, u, Dp, D, D2)
	CMPND_MOVE(z, f, Bp, B, B2)


	std::map<Cube::TurnType, void(Cube::*)()> turn = {
		// Moves
		MOVE_TABLE(R) MOVE_TABLE(r)
		MOVE_TABLE(L) MOVE_TABLE(l)
		MOVE_TABLE(U) MOVE_TABLE(u)
		MOVE_TABLE(D) MOVE_TABLE(d)
		MOVE_TABLE(F) MOVE_TABLE(f)
		MOVE_TABLE(B) MOVE_TABLE(b)
		MOVE_TABLE(M)
		MOVE_TABLE(S)
		MOVE_TABLE(E)
		// Rotations
		MOVE_TABLE(x)
		MOVE_TABLE(y)
		MOVE_TABLE(z)
	};
};

Cube::Cube()
{
	for (int i = 0; i < 6; ++i)
		for (int j = 0; j < 3; ++j)
			for (int k = 0; k < 3; ++k)
				mat[i][j][k] = static_cast<Facelet>(i);
}

Cube::~Cube()
{
}

std::ostream&
operator<<(std::ostream& out, const Cube& c)
{
	for (int i = 0; i < 3; ++i, out << '\n') {
		out << std::string(6, ' ');
		for (int j = 0; j < 3; ++j)
			out << c.ansicol[c.mat[0][i][j]];
	}
	for (int i = 0; i < 3; ++i, out << '\n')
		for (int k = 1; k < 5; ++k)
			for (int j = 0; j < 3; ++j)
				out << c.ansicol[c.mat[k][i][j]];
	for (int i = 0; i < 3; ++i, out << '\n') {
		out << std::string(6, ' ');
		for (int j = 0; j < 3; ++j)
			out << c.ansicol[c.mat[5][i][j]];
	}
	return out;
}

/* Matrix transformations:
 - Rotate 90deg clockwise
 - Rotate 90deg anticlockwise
 - Rotate 180deg */
inline void
Cube::rot90cw(Cube::Face& A)
{
	for (int j = 0; j < 2; ++j) {
		auto elem = A[0][j];
		A[0][j] = A[2-j][0];
		A[2-j][0] = A[2][2-j];
		A[2][2-j] = A[j][2];
		A[j][2] = elem;
	}
};

inline void
Cube::rot90acw(Cube::Face& A)
{
	for (int j = 0; j < 2; ++j) {
		auto elem = A[0][j];
		A[0][j] = A[j][2];
		A[j][2] = A[2][2-j];
		A[2][2-j] = A[2-j][0];
		A[2-j][0] = elem;
	}
}

inline void
Cube::rot180(Cube::Face& A)
{
	std::swap(A[0][0], A[2][2]);
	std::swap(A[0][2], A[2][0]);
	std::swap(A[0][1], A[2][1]);
	std::swap(A[1][0], A[1][2]);
}

inline void
Cube::do_turn(Cube::TurnType const& t)
{
	(this->*(turn[t]))();
}

/* Evaluate a standard cube notation sequence. */
int
Cube::eval(std::string const& sv)
{	
	std::stringstream ss{sv};
	std::string tmp;
	int turns{};
	while (ss >> tmp) {
		++turns;
		if (tmp.rfind("//") == 0) {
			std::string discard;
			std::getline(ss, discard);
		} else if (turn.find(tmp) != turn.end()) {
			do_turn(tmp);
		} else {
			std::cerr << "Invalid turn " << tmp << "!\n";
			break;
		}
	}
	return turns;
}
