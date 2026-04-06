#pragma once

#include <cstdint>
#include <functional>
#include <compare>
#include <ostream>

struct nodeid_t
{
private:
	uint8_t value = 0;

public:
	constexpr nodeid_t() = default;
	constexpr explicit nodeid_t(uint8_t v) : value(v) {}

	constexpr explicit operator uint8_t() const { return value; }
	constexpr uint8_t Value() const { return value; }

	constexpr nodeid_t operator+(uint8_t rhs) const
	{
		return nodeid_t(static_cast<uint8_t>(value + rhs));
	}
	constexpr nodeid_t& operator+=(uint8_t rhs)
	{
		value = static_cast<uint8_t>(value + rhs);
		return *this;
	}
	constexpr nodeid_t operator-(uint8_t rhs) const
	{
		return nodeid_t(static_cast<uint8_t>(value - rhs));
	}
	constexpr nodeid_t& operator-=(uint8_t rhs)
	{
		value = static_cast<uint8_t>(value - rhs);
		return *this;
	}

	auto operator<=>(const nodeid_t&) const = default;
};

inline std::ostream& operator<<(std::ostream& stream, const nodeid_t& node)
{
	stream << static_cast<unsigned int>(node.Value());
	return stream;
}

namespace std
{
	template<>
	struct hash<nodeid_t>
	{
		size_t operator()(const nodeid_t& n) const noexcept
		{
			return std::hash<uint8_t>{}(n.Value());
		}
	};
}
