#pragma once

#include <cstdint>
#include <format>
#include <functional>
#include <compare>

struct node_t
{
	uint8_t value = 0;

	constexpr node_t() = default;
	constexpr explicit node_t(uint8_t v) : value(v) {}

	constexpr explicit operator uint8_t() const { return value; }

	constexpr node_t operator+(uint8_t rhs) const
	{
		return node_t(static_cast<uint8_t>(value + rhs));
	}
	constexpr node_t& operator+=(uint8_t rhs)
	{
		value = static_cast<uint8_t>(value + rhs);
		return *this;
	}
	constexpr node_t operator-(uint8_t rhs) const
	{
		return node_t(static_cast<uint8_t>(value - rhs));
	}
	constexpr node_t& operator-=(uint8_t rhs)
	{
		value = static_cast<uint8_t>(value - rhs);
		return *this;
	}

	auto operator<=>(const node_t&) const = default;
};

namespace std
{
	template <class CharT>
	struct formatter<node_t, CharT> : formatter<unsigned int, CharT>
	{
		template <class FormatContext>
		auto format(const node_t& n, FormatContext& ctx) const
		{
			return formatter<unsigned int, CharT>::format(static_cast<unsigned int>(n.value), ctx);
		}
	};
}

namespace std
{
	template<>
	struct hash<node_t>
	{
		size_t operator()(const node_t& n) const noexcept
		{
			return std::hash<uint8_t>{}(n.value);
		}
	};
}
