#pragma once

#include <cctype>
#include <iomanip>
#include <sstream>
#include <concepts>

namespace FormatCompat
{
	// Concept to check if a type can be streamed to std::ostream
	template <typename T>
	concept Streamable = requires(std::ostringstream& stream, const T& value) {
		{ stream << value } -> std::convertible_to<std::ostream&>;
	};

	inline std::string HexValue(std::uint64_t value, int width)
	{
		std::ostringstream stream;
		stream << std::uppercase << std::hex << std::setfill('0');
		if (width > 0)
			stream << std::setw(width);
		stream << value;
		return stream.str();
	}

	inline std::string ValueToString(const std::string& value)
	{
		return value;
	}

	inline std::string ValueToString(const char* value)
	{
		return value ? std::string(value) : std::string();
	}

	inline std::string ValueToString(char* value)
	{
		return value ? std::string(value) : std::string();
	}

   template <typename T>
	std::string ValueToString(const T& value)
	{
		std::ostringstream stream;
		using value_type = std::remove_cv_t<std::remove_reference_t<T>>;

		if constexpr (std::is_same_v<value_type, std::uint8_t> ||
			std::is_same_v<value_type, std::int8_t> ||
			std::is_same_v<value_type, unsigned char> ||
			std::is_same_v<value_type, signed char>)
		{
			stream << static_cast<int>(value);
		}
        else if constexpr (std::is_enum_v<value_type>)
		{
			stream << static_cast<std::underlying_type_t<value_type>>(value);
		}
		else if constexpr (Streamable<T>)
		{
			stream << value;
		}
		else
		{
            static_assert(Streamable<T>, "ValueToString requires a streamable or enum type.");
		}

		return stream.str();
	}

	template <typename T>
	std::uint64_t ToUnsignedValue(const T& value)
	{
		using value_type = std::remove_cv_t<std::remove_reference_t<T>>;

		if constexpr (std::is_enum_v<value_type>)
		{
			using underlying_type = std::underlying_type_t<value_type>;
			return static_cast<std::uint64_t>(static_cast<underlying_type>(value));
		}
		else
		{
			return static_cast<std::uint64_t>(value);
		}
	}

	template <typename T>
	std::string FormatValue(std::string_view specifier, const T& value)
	{
		using value_type = std::remove_cv_t<std::remove_reference_t<T>>;

		if (specifier.empty())
			return ValueToString(value);

		if constexpr (std::is_arithmetic_v<value_type> || std::is_enum_v<value_type>)
		{
			if (specifier.front() == ':' && specifier.back() == 'X')
			{
				int width = 0;
				for (size_t i = 1; i + 1 < specifier.size(); ++i)
				{
					if (std::isdigit(static_cast<unsigned char>(specifier[i])) != 0)
						width = (width * 10) + (specifier[i] - '0');
				}

				return HexValue(ToUnsignedValue(value), width > 0 ? width : 2);
			}
		}

		return ValueToString(value);
	}

	template <typename T>
	void ReplaceFirst(std::string& text, const T& value)
	{
		auto openPos = text.find('{');
		if (openPos == std::string::npos)
			return;

		auto closePos = text.find('}', openPos);
		if (closePos == std::string::npos)
			return;

		const auto specifier = std::string_view(text).substr(openPos + 1, closePos - openPos - 1);
		text.replace(openPos, closePos - openPos + 1, FormatValue(specifier, value));
	}

	template <typename... Args>
	std::string format(std::string_view text, Args&&... args)
	{
		std::string result(text);
		(ReplaceFirst(result, std::forward<Args>(args)), ...);
		return result;
	}
}
