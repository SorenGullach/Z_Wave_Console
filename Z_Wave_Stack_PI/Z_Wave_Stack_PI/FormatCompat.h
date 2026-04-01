#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace FormatCompat
{
	// Forward declarations to reduce compilation dependencies
	std::string HexValue(std::uint64_t value, int width = 2);
	std::string ValueToString(const std::string& value);
	std::string ValueToString(const char* value);
	std::string ValueToString(char* value);

	template <typename T>
	std::string ValueToString(const T& value);

	template <typename T>
	std::uint64_t ToUnsignedValue(const T& value);

	template <typename T>
	std::string FormatValue(std::string_view specifier, const T& value);

	template <typename T>
	void ReplaceFirst(std::string& text, const T& value);

	template <typename... Args>
	std::string Format(std::string_view text, Args&&... args);
}

// Implementation must be in header for templates
#include "FormatCompat.inl"
