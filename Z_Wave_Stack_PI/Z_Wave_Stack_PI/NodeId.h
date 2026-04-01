#pragma once

#include <cstdint>
#include <functional>
#include <compare>
#include <ostream>

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

inline std::ostream& operator<<(std::ostream& stream, const node_t& node)
{
	stream << static_cast<unsigned int>(node.value);
	return stream;
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
