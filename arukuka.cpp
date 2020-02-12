// akemi's PrimeDaihinmin Solver (C) 2019 Fixstars Corp.
// g++ -W -Wall -std=c++17 -O3 -march=native -mavx arukuka.cpp -lgmp -o X -static

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <random>
#include <algorithm>
#include <utility>
#include <cstddef>
#include <cstdint>

#include <boost/hana/string.hpp>

#include <gmpxx.h>
#include "json.hpp"

constexpr const char* version  = "0.06";
constexpr const char* revision = "a";
constexpr const char* ver_date = "20191225";

static double TIME_LIMIT = 10.0;
const int NUM_FIRST_CARDS = 5;

namespace util
{
template < std::size_t N, typename T >
#if defined( __clang__ ) || defined( __GNUC__ )
  __attribute__( ( always_inline ) )
#elif defined( _MSC_VER )
  __forceinline
#endif
[[ nodiscard ]] static constexpr T* assume_aligned( T* ptr )
{
#if defined( __clang__ ) || ( defined( __GNUC__ ) && !defined( __ICC ) )
  return reinterpret_cast< T* >( __builtin_assume_aligned( ptr, N ) );
#elif defined(_MSC_VER)
  if ( ( reinterpret_cast< std::uintptr_t >( ptr ) & ( ( 1 << N ) - 1 ) ) == 0 )
    return ptr;
  else
    __assume( 0 );
#elif defined( __ICC )
  switch ( N )
  {
    case 2: __assume_aligned( ptr, 2 ); break;
    case 4: __assume_aligned( ptr, 4 ); break;
    case 8: __assume_aligned( ptr, 8 ); break;
    case 16: __assume_aligned( ptr, 16 ); break;
    case 32: __assume_aligned( ptr, 32 ); break;
    case 64: __assume_aligned( ptr, 64 ); break;
    case 128: __assume_aligned( ptr, 128 ); break;
  }
  return ptr;
#else
  // Unknown compiler — do nothing
  return ptr;
#endif
}
}

#ifdef SOLVER_DEBUG
#define DBG(x) do { std::cerr << __func__ << "(" << __LINE__ << "): " << #x << ": " << (x) << std::endl; } while(0)
#define dbg_printf fprintf(stderr, ...)
#else
#define DBG(...)
#define dbg_printf(...)
#endif

template <typename F, typename S>
static std::ostream& operator<<(std::ostream& os, const std::pair<F, S>& p)
{
	os << "{";
	os << p.first;
	os << ", ";
	os << p.second;
	os << "}";
	return os;
}

template <typename T>
static std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec)
{
	os << "[";
	for (size_t i = 0; i < vec.size(); ++i)
	{
		if (i)
			os << ", ";
		os << vec[i];
	}
	os << "]";
	return os;
}

template <typename T, size_t N>
static std::ostream& operator<<(std::ostream& os, const std::array<T, N>& vec)
{
	os << "[";
	for (size_t i = 0; i < N; ++i)
	{
		if (i)
			os << ", ";
		os << vec[i];
	}
	os << "]";
	return os;
}

void remove_newline(std::string& s)
{
	std::string target("\n");
	std::string::size_type pos = s.find(target);
	while (pos != std::string::npos) {
		s.replace(pos, target.size(), "");
		pos = s.find(target, pos);
	}
}

// Miller-Rabin primality test
int is_prime(const mpz_t q, int k=25/*50*/)
{
	return mpz_probab_prime_p(q, k);
}

// greedy primality test
int is_prime(const int d)
{
	if (d <= 1)
		return false;
	if (d == 2)
		return true;
	if (d % 2 == 0)
		return false;
	for (int x = 3; x * x <= d; x += 2)
	{
		if (d % x == 0)
			return false;
	}
	return true;
}

void string2digits(const std::string& s, std::vector<int>& v)
{
	for (const char c : s) {
		int n = c - '0';
		if (n < 0 || n > 9) continue;
		v.push_back(n);
	}
}

std::random_device seed_gen;
std::default_random_engine engine(seed_gen());
std::string make_candidate(std::vector<int> hand, int length)
{
	std::stringstream ss("");

	if (length == 1) {
		std::vector<int> h(hand);
		h.erase(std::unique(h.begin(), h.end()), h.end());
		auto p = std::remove(h.begin(),h.end(), 0);
		p = std::remove(h.begin(), p, 1);
		p = std::remove(h.begin(), p, 4);
		p = std::remove(h.begin(), p, 6);
		p = std::remove(h.begin(), p, 8);
		p = std::remove(h.begin(), p, 9);
		h.erase(p, h.end());
		if (h.empty()) {
			return "";
		} else {
			std::uniform_int_distribution<> randint(0, h.size() - 1);
			int n = h[randint(engine)];
			ss << n;
			return ss.str();
		}
	}

	std::vector<int> hh;
	std::vector<int> ht;
	std::copy_if(hand.begin(), hand.end(), std::back_inserter(hh), [](int n){ return n > 0; });
	std::copy_if(hand.begin(), hand.end(), std::back_inserter(ht), [](int n){ return (n & 1) == 1 && n != 5; });
	if (ht.empty()) return "";
	std::uniform_int_distribution<> randht(0, ht.size() - 1);
	int tail = ht[randht(engine)];
	hh.erase(std::find(hh.begin(), hh.end(), tail));
	if (hh.empty()) return "";
	std::uniform_int_distribution<> randhh(0, hh.size() - 1);
	int head = hh[randhh(engine)];
	hand.erase(std::find(hand.begin(), hand.end(), tail));
	hand.erase(std::find(hand.begin(), hand.end(), head));
	std::shuffle(hand.begin(), hand.end(), engine);
	ss << head;
	for (int i = 0; i < length - 2; i++) {
		ss << hand[i];
	}
	ss << tail;
	return ss.str();
}

std::string make_Mersenne(std::vector<int> hand, int length)
{
	std::vector<int> u;

	switch (length) {
	case 1:
		if (std::find(hand.begin(), hand.end(), 3) != hand.end()) return "3";
		if (std::find(hand.begin(), hand.end(), 7) != hand.end()) return "7";
		break;
	case 2:
		u = {1, 3};
		if (std::includes(hand.begin(), hand.end(), u.begin(), u.end())) return "31";
		break;
	case 3:
		u = {1, 2, 7};
		if (std::includes(hand.begin(), hand.end(), u.begin(), u.end())) return "127";
		break;
	case 4:
		u = {1, 8, 9};
		if (std::includes(hand.begin(), hand.end(), u.begin(), u.end())
			&& std::count(hand.begin(), hand.end(), 1) >= 2) return "8191";
		break;
	case 6:
		u = {2, 4, 5, 7, 8};
		if (std::includes(hand.begin(), hand.end(), u.begin(), u.end())
			&& std::count(hand.begin(), hand.end(), 2) >= 2) return "524287";
		u = {0, 1, 3, 7};
		if (std::includes(hand.begin(), hand.end(), u.begin(), u.end())
			&& std::count(hand.begin(), hand.end(), 1) >= 3) return "131071";
		break;
	case 10:
		u = {1, 2, 3, 4, 6, 7, 8};
		if (std::includes(hand.begin(), hand.end(), u.begin(), u.end())
			&& std::count(hand.begin(), hand.end(), 4) >= 3 
			&& std::count(hand.begin(), hand.end(), 7) >= 2) return "2147483647";
		break;
	}

	return "";
}

[[deprecated]]
std::vector<int> solver(std::vector<int> hand, const mpz_class& number, int length)
{
	std::vector<int> cards;
	std::vector<int> ret({});

	if (static_cast<int>(hand.size()) < length) return cards;

	// check Belphegor prime
	int cnt_0 = std::count(hand.begin(), hand.end(), 0);
	int cnt_1 = std::count(hand.begin(), hand.end(), 1);
	int cnt_6 = std::count(hand.begin(), hand.end(), 6);
	if (cnt_0 >=26 && cnt_1 >= 2 && cnt_6 >= 3) {
		ret = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
		return ret;
	}

	// check Mersenne prime
	std::string mersenne = make_Mersenne(hand, length);
	if (!mersenne.empty()) {
		mpz_class m;
		m.set_str(mersenne, 10);
		if (m > number) {
			string2digits(mersenne, ret);
			return ret;
		}
	}

	mpz_class candidate;
	for (int i = 0; i < 10000; i++) {
		std::string s = make_candidate(hand, length);
		if (!s.empty()) {
			candidate.set_str(s, 10);
			if (candidate > number && is_prime(candidate.get_mpz_t())) {
				string2digits(s, ret);
				return ret;
			}
		}
	}

	return ret;
}

constexpr size_t MAX_ALIGN = alignof(std::max_align_t);
#define ALIGNED alignas(MAX_ALIGN)
using card_type = int16_t;
using hand_type = card_type[10];
ALIGNED hand_type g_hand;
int g_num_hand;
constexpr int MAX_DIGITS = 16;
ALIGNED char ans_arr[8192];
static_assert(sizeof(ans_arr) / sizeof(char) > MAX_DIGITS + (MAX_DIGITS - 1) * 2 + 2);
const char *ans_ptr = nullptr;

namespace belphegor
{
constexpr int BELPHEGOR_PRIME_ARR[] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
template <int INDEX>
static constexpr auto _generate_belphegor_str()
{
	return _generate_belphegor_str<INDEX - 1>()
			+ boost::hana::string_c<',', ' '>
			+ boost::hana::string_c<static_cast<char>('0' + BELPHEGOR_PRIME_ARR[INDEX])>;
}
template <>
constexpr auto _generate_belphegor_str<0>()
{
	return boost::hana::string_c<'['>
			+ boost::hana::string_c<static_cast<char>('0' + BELPHEGOR_PRIME_ARR[0])>;
}
static constexpr auto generate_belphegor_str()
{
	return _generate_belphegor_str<sizeof(BELPHEGOR_PRIME_ARR) / sizeof(int) - 1>()
			+ boost::hana::string_c<']'>;
}
constexpr auto BELPHEGOR_PRIME_CSTR = generate_belphegor_str().c_str();
}
static bool belphe_check()
{
	return g_hand[0] >= 26
	    && g_hand[1] >= 2
			&& g_hand[6] >= 3;
}
static void preserve_belphe()
{
	g_hand[0] -= 26;
	g_hand[1] -= 2;
	g_hand[6] -= 3;
	g_num_hand -= 31;
}

std::vector<int64_t> mersenne_check(int length, int64_t prev = -1, const card_type * const __restrict _hand = g_hand)
{
	const card_type * const __restrict hand = util::assume_aligned<MAX_ALIGN>(_hand);

	std::vector<int64_t> mer;

	switch(length) {
		case 1:
			if (hand[3] >= 1) mer.push_back(3);
			if (hand[7] >= 1) mer.push_back(7);
			break;
		case 2:
			if (hand[3] >= 1 && hand[1] >= 1) mer.push_back(31);
			break;
		case 3:
			if (hand[1] >= 1 && hand[2] >= 1 && hand[7] >= 1) mer.push_back(127);
			break;
		case 4:
			if (hand[8] >= 1 && hand[1] >= 2 && hand[9] >= 1) mer.push_back(8191);
			break;
		case 6:
			if (hand[0] >= 1 && hand[1] >= 3
			    && hand[3] >= 1 && hand[7] >= 1)
				mer.push_back(131071);
			if (hand[2] >= 2 && hand[4] >= 1
			    && hand[5] >= 1 && hand[7] >= 1
			    && hand[8] >= 1)
				mer.push_back(524287);
			break;
		case 10:
			if (hand[1] >= 1 && hand[2] >= 1
			    && hand[3] >= 1 && hand[4] >= 3
			    && hand[6] >= 1 && hand[7] >= 2
					&& hand[8] >= 1)
				mer.push_back(2147483647);
			break;
	}

	std::vector<int64_t> ret;
	for (const auto& x : mer)
	{
		if (x > prev)
		{
			ret.push_back(x);
		}
	}

	return ret;
}

static bool is_possible(const card_type * const __restrict _cnt, const card_type * const __restrict _hand = g_hand)
{
	const card_type * const __restrict cnt = util::assume_aligned<MAX_ALIGN>(_cnt);
	const card_type * const __restrict hand = util::assume_aligned<MAX_ALIGN>(_hand);
	for (int i = 0; i < 10; ++i)
	{
		if (cnt[i] > hand[i])
		{
			return false;
		}
	}
	return true;
}

static bool is_possible(int64_t p, const card_type * const __restrict _hand = g_hand)
{
	ALIGNED hand_type cnt = {0};
	while (p > 0)
	{
		int d = p % 10;
		++cnt[d];
		p /= 10;
	}
	return is_possible(cnt, _hand);
}

static bool is_possible(const std::string& str)
{
	ALIGNED hand_type cnt = {0};
	for (const auto& c : str)
	{
		int d = c - '0';
		++cnt[d];
	}
	return is_possible(cnt);
}

static bool is_possible(const mpz_class& p)
{
	const std::string str = p.get_str(10);
	return is_possible(str);
}

static void generate_ans(const std::string str)
{
	const size_t n = str.size();
	ans_arr[0] = '[';
	for (size_t i = 0; i < n; ++i)
	{
		ans_arr[i * 2 + 1] = str[i];
		ans_arr[i * 2 + 2] = ',';
	}
	ans_arr[n * 2] = ']';
	ans_arr[n * 2 + 1] = '\0';
	ans_ptr = ans_arr;
}

static void generate_ans(int64_t p)
{
	if (p <= 0)
	{
		return;
	}
	const auto str = std::to_string(p);
	generate_ans(str);
}

struct penalty_t
{
	int even_overflow;           //!< 偶数が奇数より大きくなったときのペナルティ
	ALIGNED hand_type weakness;  //!< カード事の弱さ
};

penalty_t g_penalty = {1'000'000, {10, 2, 6, 1, 9, 4, 8, 1, 7, 2}};

static int evaluate(int64_t p)
{
	ALIGNED hand_type next;
	std::memcpy(next, g_hand, sizeof(g_hand));
	while (p > 0)
	{
		--next[p % 10];
		p /= 10;
	}
	int odd, even;
	odd = even = 0;
	for (int i = 0; i < 10; ++i)
	{
		if (i == 5 || i % 2 == 0)
		{
			even += next[i];
		}
		else
		{
			odd += next[i];
		}
	}

	int score = 0;

	if (odd * 3 < even * 2)
	{
		score += g_penalty.even_overflow;
	}

	for (int i = 0; i < 10; ++i)
	{
		score += next[i] * g_penalty.weakness[i];
	}

	int cnt = 0;
	for (int i = 0; i < 10; ++i)
	{
		cnt += next[i];
	}
	if (cnt == 0)
	{
		score -= 100'000'000;
	}
	if (cnt == next[3] + next[7])
	{
		score -= 50'000'000;
	}

	return score;
}

int g_action_rewords[NUM_FIRST_CARDS + 1];

static void set_action_rewords(const std::vector<std::pair<std::string, card_type>>& hands, const std::string& name)
{
	memset(g_action_rewords, 0, sizeof(g_action_rewords));
	const size_t n = hands.size();
	size_t pos_me = 0;
	for (size_t i = 0; i < n; ++i)
	{
		if (hands[i].first == name)
		{
			pos_me = i;
			break;
		}
	}
	for (int start = 1; start <= NUM_FIRST_CARDS; ++start)
	{
		auto trial = hands;
		size_t index = pos_me;
		int num_cards = start;
		size_t last = n + 1;
		for (;;)
		{
			if (trial[index].second >= num_cards)
			{
				trial[index].second -= num_cards;
				num_cards += num_cards;
				last = index;
				if (trial[index].second == 0)
				{
					break;
				}
			}
			else
			{
				if (last == index)
				{
					break;
				}
			}
			++index;
			index %= n;
		}
		if (pos_me == last)
		{
			g_action_rewords[start] += 5'000'000;
		}
		auto result = trial;
		std::sort(result.begin(), result.end(), [](auto a, auto b){ return a.second < b.second; });
		std::map<std::string, int> places;
		card_type prev = -1;
		int place = -1;
		for (size_t i = 0; i < n; ++i)
		{
			if (prev != result[i].second)
			{
				prev = result[i].second;
				++place;
			}
			places[result[i].first] = place;
		}
		if (places[name] == 0)
		{
			g_action_rewords[start] += 10'000'000;
		}
	}
}

static int helper0(int p)
{
	int digits = 0;
	while (p > 0)
	{
		++digits;
		p /= 10;
	}
	return g_action_rewords[digits];
}

static void solver0()
{
	const int n_max = std::min(g_num_hand, NUM_FIRST_CARDS);

	int64_t ans = -1;
	int best_score = std::numeric_limits<int>::max();

	for (int n = n_max; n > 0; --n)
	{
		const auto mer = mersenne_check(n);
		for (const int64_t& p : mer)
		{
			const int score = evaluate(p);
			if (best_score > score)
			{
				best_score = score;
				ans = p;
			}
		}
	}

	int d = 1;
	for (int i = 0; i < n_max; ++i)
	{
		d *= 10;
	}
	for (int p = d - 1; p > 0; --p)
	{
		if (!is_possible(p))
		{
			continue;
		}

		const int score = evaluate(p) - helper0(p);
		if (best_score <= score)
		{
			continue;
		}

		if (!is_prime(p))
		{
			continue;
		}

		best_score = score;
		ans = p;
	}
	generate_ans(ans);
}

static void solver1(const int prev, const int length)
{
	if (length != 1) {
		const auto mer = mersenne_check(length, prev);
		if (mer.size())
		{
			int64_t ans = -1;
			int best_score = std::numeric_limits<int>::max();

			for (const int64_t& p : mer)
			{
				const int score = evaluate(p);
				if (best_score > score)
				{
					best_score = score;
					ans = p;
				}
			}
			generate_ans(ans);
			return;
		}
	}

	int64_t ans = -1;
	int best_score = std::numeric_limits<int>::max();

	int d = 1;
	for (; d <= prev; d *= 10);
	--d;
	for (int p = d; p > prev; --p)
	{
		if (!is_possible(p))
		{
			continue;
		}
		const int score = evaluate(p);
		if (best_score <= score)
		{
			continue;
		}
		if (!is_prime(p))
		{
			continue;
		}
		best_score = score;
		ans = p;
	}
	generate_ans(ans);
}

static int score_card(int c, const card_type * const __restrict _cnt)
{
	const card_type * const __restrict cnt = util::assume_aligned<MAX_ALIGN>(_cnt);
	int v = cnt[c];
	if (v > 0 && (c % 2 == 0 || c == 5)) {
		v += 1000000;
	}
	return v;
};

static void solver(const int length)
{
	if (length > std::min(g_num_hand, MAX_DIGITS)) {
		return;
	}
	const auto mer = mersenne_check(length);
	if (length < 8) {
		if (mer.size())
		{
			generate_ans(mer.back());
			return;
		}
	}

	ALIGNED hand_type cnt;
	std::memcpy(cnt, g_hand, sizeof(g_hand));
	std::vector<int> atom;
	for (int i = 0; i < length; ++i)
	{
		int max = -1;
		for (int j = i == 0; j < 10; ++j)
		{
			const int s = score_card(j, cnt);
			max = std::max(max, s);
		}
		std::vector<int> box;
		for (int j = i == 0; j < 10; ++j)
		{
			const int s = score_card(j, cnt);
			if (s != max)
			{
				continue;
			}
			box.push_back(j);
		}
		std::shuffle(box.begin(), box.end(), engine);
		int selected = box[0];
		--cnt[selected];
		atom.push_back(selected);
	}
	std::shuffle(atom.begin(), atom.begin() + length / 2, engine);
	std::sort(atom.begin() + length / 2, atom.end(), [&pena = std::as_const(g_penalty.weakness)](const int x, const int y) {return pena[x] > pena[y];});
	if (atom[0] == 0)
	{
		for (int i = 1; i < length; ++i)
		{
			if (atom[i] > 0)
			{
				std::swap(atom[0], atom[i]);
				break;
			}
		}
	}
	int64_t start = 0;
	int64_t max = 1;
	for (const auto& d : atom)
	{
		start *= 10;
		start += d;
		max *= 10;
	}
	--max;

	int64_t ans = -1;
	int best_score = std::numeric_limits<int>::max();

	mpz_class x;
	for (int64_t p = start; p <= max; ++p)
	{
		int64_t d = p - start;
		if (d * d > 2 * start) {
			break;
		}
		if (!is_possible(p)) {
			continue;
		}
		const int score = evaluate(p);
		if (best_score <= score)
			continue;
		x.set_str(std::to_string(p), 10);
		if (!is_prime(x.get_mpz_t())) {
			continue;
		}
		best_score = score;
		ans = p;
	}

	generate_ans(ans);
}

static void convert(const std::vector<int>& src, mpz_class& dst)
{
	static const std::array<mpz_class, 11> MPZ_NUMS = {
		0_mpz,
		1_mpz,
		2_mpz,
		3_mpz,
		4_mpz,
		5_mpz,
		6_mpz,
		7_mpz,
		8_mpz,
		9_mpz,
		10_mpz
	};
	dst = MPZ_NUMS[0];
	for (const auto& d : src)
	{
		dst *= MPZ_NUMS[10];
		dst += MPZ_NUMS[d];
	}
}

/**
 * でかい数用
 */
static void solver_massive(const int length)
{
	if (length > g_num_hand /*std::min(g_num_hand, 80)*/)
	{
		return;
	}

	ALIGNED hand_type cnt;
	std::memcpy(cnt, g_hand, sizeof(g_hand));
	std::vector<int> atom;
	for (int i = 0; i < length; ++i)
	{
		const int from = i == 0 || i + 1 >= length ? 1 : 0;
		const int step = i + 1 < length ? 1 : 2;
		int max = -1;
		for (int j = from; j < 10; j += step)
		{
			const int s = score_card(j, cnt);
			max = std::max(max, s);
		}
		std::vector<int> box;
		for (int j = from; j < 10; j += step)
		{
			const int s = score_card(j, cnt);
			if (s != max)
			{
				continue;
			}
			box.push_back(j);
		}
		if (box.empty())
		{
			return;
		}
		std::shuffle(box.begin(), box.end(), engine);
		int selected = box[0];
		--cnt[selected];
		atom.push_back(selected);
	}
	std::shuffle(atom.begin(), atom.begin() + length / 2, engine);
	std::sort(atom.begin() + length / 2, atom.end(), [&pena = std::as_const(g_penalty.weakness)](const int x, const int y) {return pena[x] > pena[y];});
	if (atom[0] == 0)
	{
		for (int i = 1; i < length; ++i)
		{
			if (atom[i] > 0)
			{
				std::swap(atom[0], atom[i]);
				break;
			}
		}
	}

	int odd_length = 0;
	for (auto ite = atom.rbegin(); ite != atom.rend(); ++ite)
	{
		if (*ite % 2 == 0 || *ite == 5)
		{
			break;
		}
		++odd_length;
	}

	mpz_class number;
	convert(atom, number);

	if (odd_length > 3)
	{
		// shuffle
		assert (is_possible(number));
		for (int i = 0; i < 1000; ++i)
		{
			if (is_prime(number.get_mpz_t())) {
				break;
			}
			std::shuffle(atom.end() - odd_length, atom.end(), engine);
			convert(atom, number);
		}
		if (is_prime(number.get_mpz_t())) {
			const std::string str = number.get_str(10);
			generate_ans(str);
		}
	}
	else
	{
		// broute force
		for (int i = 1; i < 1000; i += 2)
		{
			int sub = i;
			for (int j = 0; j < 3; ++j)
			{
				atom[length - 1 - j] = sub % 10;
				sub /= 10;
			}
			convert(atom, number);
			if (!is_possible(number))
			{
				continue;
			}
			if (!is_prime(number.get_mpz_t()))
			{
				continue;
			}
			const std::string str = number.get_str(10);
			generate_ans(str);
			// you can return and evaluate
			return;
		}
	}
}

std::deque<std::string> g_win_root;
static void search_win(const bool belphe_possible, const int length, const mpz_class& prev)
{
	// すでに確立していたら出せるか確認する
	if (g_win_root.size())
	{
		const std::string next = g_win_root.front();
		if (is_possible(next))
		{
			return;
		}
		g_win_root.clear();
	}

	struct Node
	{
		ALIGNED std::array<card_type, 10> cur_hand;
		card_type remain;
		bool belphe_possible;
		std::shared_ptr<Node> parent;
		std::string prev_action;
		Node()
				: cur_hand()
				, remain(0)
				, parent()
				, prev_action() {}
		Node(const Node& node)
				: cur_hand(node.cur_hand)
				, remain(node.remain)
				, parent(node.parent)
				, prev_action(node.prev_action) {}

		std::shared_ptr<Node> next(int64_t p, const std::shared_ptr<Node> parent) const
		{
			auto next = std::make_shared<Node>(*this);
			next->prev_action = std::to_string(p);
			while (p)
			{
				const auto d = p % 10;
				next->cur_hand[d]--;
				next->remain--;
				p /= 10;
			}
			next->parent = parent;
			return next;
		}
		std::shared_ptr<Node> use_belphe(const std::shared_ptr<Node> parent) const
		{
			auto next  = std::make_shared<Node>(*this);
			next->belphe_possible = false;
			next->prev_action = "1000000000000066600000000000001";
			next->parent = parent;
			return next;
		}
	};

	struct NodePriority
	{
		bool operator()(const std::shared_ptr<Node> lhs, const std::shared_ptr<Node> rhs) const
		{
			if (lhs->remain != rhs->remain)
			{
				return lhs->remain < rhs->remain;
			}
			return rhs->belphe_possible > lhs->belphe_possible;
		}
	};

	std::set<std::pair<bool, std::array<card_type, 10>>> done;

	std::shared_ptr<Node> initial_state = std::make_shared<Node>();
	std::memcpy(initial_state->cur_hand.data(), g_hand, sizeof(g_hand));
	initial_state->remain = 0;
	for (const auto c : initial_state->cur_hand)
	{
		initial_state->remain += c;
	}
	initial_state->belphe_possible = belphe_possible;

	std::multiset<std::shared_ptr<Node>, NodePriority> tree;
	if (length > 0)
	{
		std::vector<int64_t> mers;
		if (length <= 10)
		{
			mers = mersenne_check(length, prev.get_si());
		}
		if (mers.size())
		{
			for (const auto& p : mers)
			{
				tree.insert(initial_state->next(p, initial_state));
			}
		}
		else if (belphe_possible)
		{
			tree.insert(initial_state->use_belphe(initial_state));
		}
		else
		{
			return;
		}
	}
	else
	{
		tree.insert(initial_state);
	}

	std::shared_ptr<Node> ans;
	// 探索
	for (int iteration = 0; iteration < 1'000; ++iteration)
	{
		if (tree.empty())
		{
			break;
		}
		auto ite = tree.begin();
		const std::shared_ptr<Node> node = *ite;
		tree.erase(ite);

		if (node->remain == 0 && node->belphe_possible == false)
		{
			ans = node;
			break;
		}
		{
			const auto state = std::make_pair(node->belphe_possible, node->cur_hand);
			if (done.count(state))
			{
				continue;
			}
			done.insert(state);
		}

		// max cut
		{
			constexpr int max_cut_numbers[6] = {
				-1,
				7, // NOTICE: it is mersenne
				97,
				997,
				9973,
				99991
			};
			for (int i = 2; i <= 5; ++i)
			{
				const int p = max_cut_numbers[i];
				if (is_possible(p, node->cur_hand.data()))
				{
					tree.insert(node->next(p, node));
				}
			}
		}
		// mersenne cut
		for (int i = 1; i <= 5; ++i)
		{
			for (const auto p : mersenne_check(i, -1, node->cur_hand.data()))
			{
				tree.insert(node->next(p, node));
			}
		}
		// belphe cut
		if (node->belphe_possible)
		{
			tree.insert(node->use_belphe(node));
		}

		// 5!
		if (node->remain <= 5)
		{
			if (node->remain == 1)
			{
				int p = 0;
				for (int i = 0; i < 10; ++i)
				{
					if (node->cur_hand[i])
					{
						p = i;
						break;
					}
				}
				if (is_prime(p))
				{
					tree.insert(node->next(p, node));
				}
			}
			else
			{
				std::vector<int> cards;
				cards.reserve(5);
				for (int i = 0; i < 10; ++i)
				{
					for (int j = 0; j < node->cur_hand[i]; ++j)
					{
						cards.push_back(i);
					}
				}
				std::sort(cards.begin(), cards.end());
				do
				{
					int p = 0;
					for (const int x : cards)
					{
						if (x < 0)
						{
							continue;
						}
						p *= 10;
						p += x;
					}
					if (is_prime(p))
					{
						tree.insert(node->next(p, node));
					}
				} while (std::next_permutation(cards.begin(), cards.end()));
			}
		}
	}

	while (ans && ans->parent)
	{
		g_win_root.push_front(ans->prev_action);
		ans = ans->parent;
	}
}

int main(int argc, char** argv)
{
	std::ios::sync_with_stdio(false);
	std::string s;

	if (argc > 1)
	{
		int x = std::stoi(argv[1]);
		engine.seed(x);
	}

	g_win_root.clear();

	for (;;) {
		getline(std::cin, s);
		nlohmann::json obj = nlohmann::json::parse(s);
		auto action = obj["action"];

		if (action == "play") {
			const auto name(obj["name"].get<std::string>());
			auto hand_(obj["hand"]);
			auto numbers_(obj["numbers"]);
			// auto record(obj["record"]);
			const auto hands_(obj["hands"]);

			memset(g_hand, 0, sizeof(g_hand));
			g_num_hand = hand_.size();
			for (const auto card : hand_) {
				++g_hand[card.get<unsigned int>()];
			}

			std::vector<mpz_class> numbers;
			std::stringstream number_ss;
			mpz_class n;
			for (const auto number : numbers_) {
				number_ss << number.get<std::string>();
				n.set_str(number, 10);
				numbers.push_back(n);
			}

			std::vector<std::pair<std::string, card_type>> hands;
			for (const auto& h : hands_) {
				const auto p = h.get<std::pair<std::string, card_type>>();
				hands.push_back(p);
			}

			set_action_rewords(hands, name);

			int length = number_ss.str().length();

			ans_ptr = nullptr;
			const bool belphe_possible = belphe_check();
			if (belphe_possible) {
				preserve_belphe();
			}

			search_win(belphe_possible, length, numbers.size() ? numbers.back() : 0_mpz);
			if (g_win_root.size()) {
				const std::string act = g_win_root.front();
				g_win_root.pop_front();
				generate_ans(act);
			} else {
				switch (numbers.size()) {
					case 0:
						solver0();
						break;
					case 1:
						solver1(static_cast<int>(numbers.back().get_ui()), length);
						break;
					default:
						if (length > 12)
							solver_massive(length);
						else
							solver(length);
				}
				if (!ans_ptr && belphe_possible) {
					ans_ptr = belphegor::BELPHEGOR_PRIME_CSTR;
				}
			}

			if (ans_ptr) {
				std::cout << "{\"action\": \"number\", \"cards\": " << ans_ptr << "}";
			} else {
				std::cout << "{\"action\": \"pass\"}";
			}
			std::cout << std::endl << std::flush;
		} else if (action == "pass") {
			auto draw(obj["draw"]);
			std::cout << std::endl << std::flush;
		} else if (action == "init") {
			int uid(obj["uid"]);
			auto names(obj["names"]);
			auto name(names[uid]);
			auto hand(obj["hand"]);
			TIME_LIMIT = obj["time"];
			std::cout << std::endl << std::flush;

		} else if (action == "quit") {
			std::cout << std::endl << std::flush;
			break;
		}
	}

	return 0;
}