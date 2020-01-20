// akemi's PrimeDaihinmin Solver (C) 2019 Fixstars Corp.
// g++ akemi.cpp -std=c++17 -o akemi -O3 -Wall -Wno-unused-but-set-variable -lgmp

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
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

#define ALIGNED alignas(64)
using hand_type = int[10];
ALIGNED hand_type g_hand;
int g_num_hand;
ALIGNED char ans_arr[64];
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

std::vector<int64_t> mersenne_check(int length, int64_t prev = -1)
{
	std::vector<int64_t> mer;

	switch(length) {
		case 1:
			if (g_hand[3] >= 1) mer.push_back(3);
			if (g_hand[7] >= 1) mer.push_back(7);
			break;
		case 2:
			if (g_hand[3] >= 1 && g_hand[1] >= 1) mer.push_back(31);
			break;
		case 3:
			if (g_hand[1] >= 1 && g_hand[2] >= 1 && g_hand[7] >= 1) mer.push_back(127);
			break;
		case 4:
			if (g_hand[8] >= 1 && g_hand[1] >= 2 && g_hand[9] >= 1) mer.push_back(8191);
			break;
		case 6:
			if (g_hand[0] >= 1 && g_hand[1] >= 3
			    && g_hand[3] >= 1 && g_hand[7] >= 1)
				mer.push_back(131071);
			if (g_hand[2] >= 2 && g_hand[4] >= 1
			    && g_hand[5] >= 1 && g_hand[7] >= 1
			    && g_hand[8] >= 1)
				mer.push_back(524287);
			break;
		case 10:
			if (g_hand[1] >= 1 && g_hand[2] >= 1
			    && g_hand[3] >= 1 && g_hand[4] >= 3
			    && g_hand[6] >= 1 && g_hand[7] >= 2
					&& g_hand[8] >= 1)
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

static bool is_possible(const int * const __restrict _cnt)
{
	const int * const __restrict cnt = util::assume_aligned<64>(_cnt);
	for (int i = 0; i < 10; ++i)
	{
		if (cnt[i] > g_hand[i])
		{
			return false;
		}
	}
	return true;
}

static bool is_possible(int64_t p)
{
	ALIGNED hand_type cnt = {0};
	while (p > 0)
	{
		int d = p % 10;
		++cnt[d];
		p /= 10;
	}
	return is_possible(cnt);
}

static void generate_ans(int64_t p)
{
	std::stringstream ss;
	ss << "[";
	std::string str = std::to_string(p);
	for (size_t i = 0; i < str.size(); ++i)
	{
		if (i)
		{
			ss << ", ";
		}
		ss << str[i];
	}
	ss << "]";
	std::string ans = ss.str();
	std::memcpy(ans_arr, ans.c_str(), ans.size());
	ans_arr[ans.size()] = '\0';
	ans_ptr = ans_arr;
}

static void solver0()
{
	const int num_first_cards = 5;
	const int n_max = std::min(g_num_hand, num_first_cards);

	for (int n = n_max; n > 0; --n)
	{
		const auto mer = mersenne_check(n);
		if (mer.size())
		{
			generate_ans(mer.back());
			return;
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
		if (!is_prime(p))
		{
			continue;
		}
		generate_ans(p);
		return;
	}
}

static void solver1(const int prev, const int length)
{
	{
		const auto mer = mersenne_check(length, prev);
		if (mer.size())
		{
			generate_ans(mer.back());
			return;
		}
	}

	int d = 1;
	for (; d <= prev; d *= 10);
	--d;
	for (int p = d; p > prev; --p)
	{
		if (!is_possible(p))
		{
			continue;
		}
		if (!is_prime(p))
		{
			continue;
		}
		generate_ans(p);
		return;
	}
}

static void solver(const int length)
{
	if (length > std::min(g_num_hand, 10)) {
		return;
	}
	{
		const auto mer = mersenne_check(length);
		if (mer.size())
		{
			generate_ans(mer.back());
			return;
		}
	}

	int cnt[10];
	std::memcpy(cnt, g_hand, sizeof(g_hand));
	std::vector<int> atom;
	for (int i = 0; i < length; ++i)
	{
		int max = -1;
		for (int j = i == 0; j < 10; ++j)
		{
			max = std::max(max, cnt[j]);
		}
		std::vector<int> box;
		for (int j = i == 0; j < 10; ++j)
		{
			if (cnt[j] != max)
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
	mpz_class x;
	for (int64_t p = start; p <= max; ++p)
	{
		int64_t d = p - start;
		if (d * d > start) {
			break;
		}
		if (!is_possible(p)) {
			continue;
		}
		x.set_str(std::to_string(p), 10);
		if (!is_prime(x.get_mpz_t())) {
			continue;
		}
		generate_ans(p);
		break;
	}
}

int main(void)
{
	std::ios::sync_with_stdio(false);
	std::string s;

	for (;;) {
		getline(std::cin, s);
		nlohmann::json obj = nlohmann::json::parse(s);
		auto action = obj["action"];

		if (action == "play") {
			auto name(obj["name"]);
			auto hand_(obj["hand"]);
			auto numbers_(obj["numbers"]);
			// auto record(obj["record"]);
			// auto hands(obj["hands"]);

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

			int length = number_ss.str().length();

			ans_ptr = nullptr;
			if (belphe_check()) {
				ans_ptr = belphegor::BELPHEGOR_PRIME_CSTR;
			} else {
				switch (numbers.size()) {
					case 0:
						solver0();
						break;
					case 1:
						solver1(static_cast<int>(numbers.back().get_ui()), length);
						break;
					default:
						solver(length);
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