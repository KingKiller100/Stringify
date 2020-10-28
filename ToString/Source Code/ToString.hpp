#pragma once

#include "StringTypeTraits.hpp"
#include "StringConverter.hpp"

#include "Stringify/StringIdentity.hpp"
#include "Stringify/kSprintf.hpp"
#include "Stringify/StringifyBool.hpp"
#include "Stringify/StringifyInteger.hpp"
#include "Stringify/StringifyPointer.hpp"
#include "Stringify/StringifyFloatingPoint.hpp"

#include <any>
#include <array>
#include <deque>
#include <string>
#include <sstream>
#include <variant>
#include <xtr1common>

namespace klib {
	namespace kFormat
	{
		template<typename CharType, std::size_t Size>
		std::deque<std::pair<unsigned char, std::string>> CreateIdentifiers(std::basic_string<CharType>& fmt, std::array<std::any, Size>& elems)
		{
			static constexpr auto openerSymbol = CharType('{');
			static constexpr auto closerSymbol = CharType('}');
			static constexpr auto precisionSymbol = CharType(':');
			static constexpr auto nullTerminator = type_trait::s_NullTerminator<CharType>;
			static constexpr auto npos = std::basic_string<CharType>::npos;

			std::deque<std::pair<unsigned char, std::string>> identifiers;
			for (auto openerPos = fmt.find_first_of(openerSymbol);
				openerPos != npos;
				openerPos = fmt.find_first_of(openerSymbol, openerPos + 1))
			{
				if (fmt[openerPos + 1] == openerSymbol ||
					fmt[openerPos + 1] == CharType(' ') ||
					fmt[openerPos + 1] == CharType('\t') ||
					fmt[openerPos + 1] == nullTerminator)
				{
					openerPos += 2;
					continue;
				}

				const auto closePos = fmt.find_first_of(closerSymbol, openerPos);
				std::string bracket = kString::Convert<char>(fmt.substr(openerPos + 1, closePos - 1));

				const auto relativeColonPos = bracket.find_first_of(precisionSymbol);
				const auto optionIndex = bracket.substr(0, relativeColonPos);
				const auto idx = static_cast<unsigned char>(std::stoi(optionIndex));
				const auto type = elems[idx].type().name();

				identifiers.push_back(std::make_pair(idx, type));
			}
			identifiers.shrink_to_fit();

			return identifiers;
		}

		// Outputs a interpolated string with data given for all string types. NOTE: Best performance with char and wchar_t type strings
		template<class CharType, typename T, typename ...Ts>
		USE_RESULT constexpr std::basic_string<CharType> ToString(const std::basic_string_view<CharType>& format, const T& arg, const Ts& ...argPack)
		{
			using namespace kString;
			using DataTypes = std::variant<std::monostate, T, Ts...>;

			static constexpr auto printfSymbol = CharType('%');
			static constexpr auto openerSymbol = CharType('{');
			static constexpr auto closerSymbol = CharType('}');
			static constexpr auto precisionSymbol = CharType(':');
			static constexpr auto nullTerminator = type_trait::s_NullTerminator<CharType>;
			static constexpr auto npos = std::basic_string<CharType>::npos;

			if (auto pfSymPos = format.find(printfSymbol); pfSymPos != npos)
			{
				return stringify::Sprintf<CharType>(format, stringify::Identity<CharType>(arg), stringify::Identity<CharType>(argPack)...);
			}

			std::array<std::any, std::variant_size_v<DataTypes> -1> elems = { stringify::IdentityPtr<CharType, T>(arg)
				, stringify::IdentityPtr<CharType, Ts>(argPack)... };

			std::basic_string<CharType> fmt(format);
			std::deque<std::pair<unsigned char, std::string>> identifiers = CreateIdentifiers(fmt, elems);

			std::basic_string<CharType> finalString;
			for (const auto& id : identifiers)
			{
				const auto& val = elems[id.first];
				const auto& type = id.second;
				const auto inputPos = fmt.find_first_of(closerSymbol) + 1;
				auto currentSection = fmt.substr(0, inputPos);
				auto replacePos = currentSection.find_first_of(openerSymbol);
				auto colonPos = currentSection.find(precisionSymbol, replacePos);
				size_t padding = stringify::nPrecision;

				if (colonPos != npos)
				{
					padding = std::stoll(kString::Convert<char>(currentSection.substr(colonPos + 1, inputPos - 1)));
				}

				currentSection.erase(replacePos);

				if (HasString(type, "void"))
				{
					auto data = std::any_cast<const void*>(val);
					currentSection.append(stringify::StringifyPointer<CharType>(data, padding));
					finalString.append(currentSection);
				}
				else if (HasString(type, "basic_string_view"))
				{
					const auto data = std::any_cast<const std::basic_string_view<CharType>*>(val);
					currentSection.erase(replacePos);
					currentSection.insert(replacePos, data->data());
					finalString.append(currentSection);
				}
				else if (HasString(type, "basic_string"))
				{
					const auto data = std::any_cast<const std::basic_string<CharType>*>(val);
					currentSection.erase(replacePos);
					currentSection.insert(replacePos, data->data());
					finalString.append(currentSection);
				}
				else if (HasString(type, "unsigned"))
				{
					if (HasString(type, "char"))
					{
						auto data = std::any_cast<const unsigned char*>(val);
						currentSection.append(stringify::StringUnsignedIntegral<CharType>(*data, padding));
						finalString.append(currentSection);
					}
					else if (HasString(type, "short"))
					{
						auto data = std::any_cast<const unsigned short*>(val);
						currentSection.append(stringify::StringUnsignedIntegral<CharType>(*data, padding));
						finalString.append(currentSection);
					}
					else if (HasString(type, "int"))
					{
						if (HasString(type, "__int64"))
						{
							auto data = std::any_cast<const unsigned __int64*>(val);
							currentSection.append(stringify::StringUnsignedIntegral<CharType>(*data, padding));
							finalString.append(currentSection);
						}
						else
						{
							auto data = std::any_cast<const unsigned int*>(val);
							currentSection.append(stringify::StringUnsignedIntegral<CharType>(*data, padding));
							finalString.append(currentSection);
						}
					}
					else if (HasString(type, "long"))
					{
						if (HasString(type, "long long"))
						{
							auto data = std::any_cast<const unsigned long long*>(val);
							currentSection.append(stringify::StringUnsignedIntegral<CharType>(*data, padding));
							finalString.append(currentSection);
						}
						else
						{
							auto data = std::any_cast<const unsigned long*>(val);
							currentSection.append(stringify::StringUnsignedIntegral<CharType>(*data, padding));
							finalString.append(currentSection);
						}
					}
				}
				else if (HasString(type, "long"))
				{
					if (HasString(type, "double"))
					{
						auto data = std::any_cast<const long double*>(val);
						currentSection += stringify::StringFloatingPoint<CharType>(*data, padding);
						finalString.append(currentSection);
					}
					else if (HasString(type, "int"))
					{
						auto data = std::any_cast<const long int*>(val);
						currentSection += stringify::StringSignedIntegral<CharType>(*data, padding);
						finalString.append(currentSection);
					}
					else if (HasString(type, "long long"))
					{
						const auto data = std::any_cast<const long long*>(val);
						currentSection += stringify::StringSignedIntegral<CharType>(*data, padding);
						finalString.append(currentSection);
					}
					else
					{
						auto data = std::any_cast<const long*>(val);
						currentSection += stringify::StringSignedIntegral<CharType>(*data, padding);
						finalString.append(currentSection);
					}
				}
				else if (HasString(type, "char"))
				{
					const auto data = std::any_cast<const CharType*>(val);
					currentSection.erase(replacePos);
					currentSection.insert(replacePos, data);
					finalString.append(currentSection);
				}
				else if (HasString(type, "short"))
				{
					auto data = std::any_cast<const short*>(val);
					currentSection += stringify::StringSignedIntegral<CharType>(*data, padding);
					finalString.append(currentSection);
				}
				else if (HasString(type, "int"))
				{
					if (HasString(type, "__int64"))
					{
						auto data = std::any_cast<const __int64*>(val);
						currentSection += stringify::StringSignedIntegral<CharType>(*data, padding);
						finalString.append(currentSection);
					}
					else
					{
						auto data = std::any_cast<const int*>(val);
						currentSection += stringify::StringSignedIntegral<CharType>(*data, padding);
						finalString.append(currentSection);
					}
				}
				else if (HasString(type, "double"))
				{
					const auto data = std::any_cast<const double*>(val);
					currentSection += stringify::StringFloatingPoint<CharType>(*data, padding);
					finalString.append(currentSection);
				}
				else if (HasString(type, "float"))
				{
					const auto data = std::any_cast<const float*>(val);
					currentSection.append(stringify::StringFloatingPoint<CharType>(*data, padding));
					finalString.append(currentSection);
				}
				else if (HasString(type, "bool"))
				{
					const auto res = std::any_cast<const bool*>(val);
					currentSection.append(stringify::StringBool<CharType>(*res));
					finalString.append(currentSection);
				}
				else
				{
					throw std::runtime_error("Type entered not recognised/supported");
				}

				fmt.erase(0, inputPos);
				identifiers.pop_front();
			}

			if (!fmt.empty())
				finalString.append(fmt);

			return finalString;
		}

		template<class CharType, typename T, typename ...Ts>
		USE_RESULT constexpr std::basic_string<CharType> ToString(const CharType* format, const T& arg, const Ts& ...argPack)
		{
			const std::basic_string_view<CharType> fmt(format);
			const std::basic_string<CharType> text = ToString(fmt, arg, argPack...);
			return text;
		}

		template<class CharType, typename T>
		constexpr std::basic_string<CharType> ToString(T&& object)
		{
			std::basic_stringstream<CharType> ss;
			ss << std::forward<T>(object);
			return ss.str();
		}
	}

#ifdef KLIB_SHORT_NAMESPACE
	using namespace kFormat;
#endif
}
