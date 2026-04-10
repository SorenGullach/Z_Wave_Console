#pragma once

#include <cstdint>
#include <functional>
#include <compare>
#include <ostream>
#include "FormatCompat.h"
#include "CommandClass.h"

// ===============================================================
template <typename Tag, typename T = uint8_t>
struct templateid_t
{
private:
	T value = 0;

public:
	constexpr templateid_t() = default;
	constexpr templateid_t(T v) : value(v) {}

	constexpr templateid_t(std::nullptr_t) : value(0) {}

	template <typename E>
	constexpr templateid_t(E e) requires std::is_enum_v<E>
		: value(static_cast<T>(e))
	{
	}

	constexpr T Value() const { return value; }
	constexpr operator T() const { return value; }

	// Same-type equality
	constexpr bool operator==(const templateid_t&) const = default;

	// Three-way comparison
	auto operator<=>(const templateid_t&) const = default;

	// Compare with enums
	template <typename E>
	constexpr bool operator==(E e) const noexcept
		requires std::is_enum_v<E>
	{
		return value == static_cast<T>(e);
	}

	template <std::integral I>
	constexpr bool operator==(I v) const noexcept
	{
		return value == static_cast<T>(v);
	}

	// Compare with raw underlying type
	constexpr bool operator==(T v) const noexcept
	{
		return value == v;
	}
};

template <typename Tag, typename T>
std::ostream& operator<<(std::ostream& os, const templateid_t<Tag, T>& id)
{
	return os << static_cast<unsigned>(id.Value());
}

namespace std
{
	template <typename Tag, typename T>
	struct hash<templateid_t<Tag, T>>
	{
		size_t operator()(const templateid_t<Tag, T>& id) const noexcept
		{
			return std::hash<T>{}(id.Value());
		}
	};
}

// ===============================================================
struct nodeid_tag {};
struct ccid_tag {};

using nodeid_t = templateid_t<nodeid_tag, uint8_t>;
using ccid_t = templateid_t<ccid_tag, uint8_t>;

// ===============================================================
template <typename Tag, typename T = uint8_t>
struct template_vector_t : public std::vector<T>
{
	using base = std::vector<T>;
	using base::base;

	template_vector_t() = default;

	template_vector_t(std::initializer_list<T> il)
		: base(il)
	{
	}
};

template <typename Tag, typename T>
std::ostream& operator<<(std::ostream& os, const template_vector_t<Tag, T>& vec)
{
	for (size_t i = 0; i < vec.size(); i++)
	{
		os << std::format("0x{:02X}", static_cast<unsigned>(vec[i]));
		if (i + 1 < vec.size())
			os << ", ";
	}
	return os;
}


// ===============================================================
struct ccparams_tag {};

// ===============================================================
// SPECIALIZATION for ccparams_t ONLY
template <>
struct template_vector_t<ccparams_tag, uint8_t> : public std::vector<uint8_t>
{
	using base = std::vector<uint8_t>;
	using base::base;

	template_vector_t() = default;

	// Accept initializer_list<eCommandClass>
	template_vector_t(std::initializer_list<eCommandClass> il)
	{
		this->reserve(il.size());
		for (auto e : il)
			this->push_back(static_cast<uint8_t>(e));
	}

	// Accept initializer_list<uint8_t> (normal behavior)
	template_vector_t(std::initializer_list<uint8_t> il)
		: base(il)
	{
	}
};

// ===============================================================
struct payload_tag {};

using payload_t = template_vector_t<payload_tag, uint8_t>;
using ccparams_t = template_vector_t<ccparams_tag, uint8_t>;
