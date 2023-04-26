#include <numeric>
#include <ranges>

#include "misc/dijkstra.h"
#include "util.h"

namespace test
{

	struct Fraction
	{
		struct no_reduce_t
		{
		};
		static constexpr no_reduce_t no_reduce {};

		int64_t numer;
		int64_t denom;

		Fraction() : numer(0), denom(1) { }
		Fraction(int64_t numer, int64_t denom) : numer(numer), denom(denom) { reduce(); }

		static Fraction one() { return { 1, 1 }; }
		static Fraction zero() { return { 0, 1 }; }

		Fraction reciprocal() const { return { denom, numer, no_reduce }; }
		Fraction operator*(Fraction other) const { return { numer * other.numer, denom * other.denom }; }
		Fraction operator/(Fraction other) const { return { numer * other.denom, denom * other.numer }; }
		Fraction operator+(Fraction other) const
		{
			return { numer * other.denom + other.numer * denom, denom * other.denom };
		}
		Fraction operator-() const { return { -numer, denom, no_reduce }; };
		Fraction operator-(Fraction other) const { return *this + (-other); }

		bool operator==(Fraction other) const { return (numer == other.numer) & (denom == other.denom); }
		// clang-format off
		std::strong_ordering operator<=>(Fraction other) const { return numer * other.denom <=> other.numer * denom; }
		// clang-format on

		template <typename Cb>
		void zpr_print(Cb&& cb, zpr::format_args args) const
		{
			zpr::detail::print_one(cb, args, numer);
			cb(" / ", 3);
			zpr::detail::print_one(cb, args, denom);
		}

	private:
		Fraction(int64_t numer, int64_t denom, no_reduce_t) : numer(numer), denom(denom) { }

		void reduce()
		{
			if(denom < 0)
			{
				numer = -numer;
				denom = -denom;
			}
			auto gcd = std::gcd(numer, denom);
			numer /= gcd;
			denom /= gcd;
		}
	};

	struct Node
	{
		using Distance = int64_t;
		using DistanceNodePair = std::pair<Node, Distance>;

		Fraction total;
		Fraction num {};

		std::vector<DistanceNodePair> neighbours()
		{
			std::vector<DistanceNodePair> ret;
			ret.reserve(100);
			for(int64_t i = 1; i <= 100; ++i)
			{
				ret.push_back(DistanceNodePair { Node { total + Fraction { 1, i }, Fraction { 1, i } }, i });
			}
			return ret;
		}

		size_t hash() const { return (size_t) (num.numer ^ (num.denom << 32)); }

		template <typename Cb>
		void zpr_print(Cb&& cb, zpr::format_args args) const
		{
			zpr::detail::print_one(cb, args, num);
		}

		std::strong_ordering operator<=>(Node other) const
		{
			// clang-format off
			return total <=> other.total;
			// clang-format on
		}
		bool operator==(Node other) const { return total == other.total; }
	};

}

int main()
{
	static_assert(zpr::detail::is_iterable_by_member<util::subrange<std::vector<test::Node>::iterator>>::value);
	static_assert(zpr::detail::is_iterable<util::subrange<std::vector<test::Node>::iterator>>::value);
	static_assert(zpr::detail::has_formatter<util::subrange<std::vector<test::Node>::iterator>>::value);
	auto wow = util::dijkstra_shortest_path(test::Node { test::Fraction::zero() },
	    test::Node { test::Fraction { 7, 12 } });
	zpr::println("{}", util::subrange(wow.begin() + 1, wow.end()));
}

bool sap::isDraftMode()
{
	return false;
}

bool sap::compile(zst::str_view input_file, zst::str_view output_file)
{
	return false;
}
