#pragma once

#include "StringTypeTraits.hpp"
#include "StringConverter.hpp"

#include "Stringify/StringifyInteger.hpp"
#include "Stringify/StringifyPointer.hpp"
#include "Stringify/StringifyFloatingPoint.hpp"

#include <any>
#include <array>
#include <cstdio>
#include <deque>
#include <string>
#include <sstream>
#include <variant>
#include <xtr1common>

#if defined (_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4996)

namespace klib {
	namespace kFormat
	{
		// C++ STL string/string_view
		template<typename CharType, typename T, typename C = CharType, typename U = T>
		constexpr
			std::enable_if_t <type_trait::Is_CharType_V<C>
			&& type_trait::Is_StringType_V<U>,
			const typename T::value_type*>
			GetValue(const T& str)
		{
			return str.data();
		}

		// Non C string, but primative type ptrs
		template<typename CharType, typename T, typename C = CharType, typename U = T>
		constexpr
			std::enable_if_t<type_trait::Is_CharType_V<C>
			&& std::is_pointer_v<U>
			&& !type_trait::Is_CharType_V<ONLY_TYPE(U)>
			&& !type_trait::Is_StringType_V<ONLY_TYPE(U)>
			, const void*>
			GetValue(const T obj)
		{
			return (const void*)obj;
		}

		// C string ptrs
		template<typename CharType, typename T, typename C = CharType, typename U = T>
		constexpr
			std::enable_if_t<type_trait::Is_CharType_V<C>
			&& std::is_pointer_v<U>
			&& type_trait::Is_CharType_V<ONLY_TYPE(U)>
			&& !type_trait::Is_StringType_V<ONLY_TYPE(U)>
			&& std::is_same_v<C, ONLY_TYPE(U)>
			, const T>
			GetValue(const T obj)
		{
			return obj;
		}

		// Primitive types (int, double, unsigned long long,...) but not bool
		template<typename CharType, typename T, typename C = CharType, typename U = T>
		constexpr
			std::enable_if_t<type_trait::Is_CharType_V<C>
			&& std::is_arithmetic_v<U>
			, const U>
			GetValue(const T obj)
		{
			return obj;
		}

		// Non-primative types
		template<typename CharType, typename T, typename C = CharType, typename U = T>
		constexpr
			std::enable_if_t<(type_trait::Is_CharType_V<C>
				&& !std::is_arithmetic_v<std::decay_t<U>>
				&& !type_trait::Is_StringType_V<U>
				&& !std::is_pointer_v<std::decay_t<U>>
				), const CharType*>
			GetValue(const T& obj)
		{
			return obj.ToString().data();
		}

		template<typename CharType, typename T, typename C = CharType, typename U = T>
		constexpr
			std::enable_if_t <type_trait::Is_CharType_V<C>
			&& type_trait::Is_StringType_V<U>,
			const T*>
			GetValuePtr(const T& str)
		{
			return &str;
		}

		// Non C string, but primitive ptrs
		template<typename CharType, typename T, typename C = CharType, typename U = T>
		constexpr
			std::enable_if_t <type_trait::Is_CharType_V<C>
			&& std::is_pointer_v<U>
			&& !type_trait::Is_CharType_V<ONLY_TYPE(U)>
			&& !type_trait::Is_StringType_V<ONLY_TYPE(U)>
			, const void*>
			GetValuePtr(const T obj)
		{
			return (const void*)obj;
		}

		// C string ptrs
		template<typename CharType, typename T, typename C = CharType, typename U = T>
		constexpr
			std::enable_if_t <type_trait::Is_CharType_V<C>
			&& std::is_pointer_v<U>
			&& type_trait::Is_CharType_V<ONLY_TYPE(U)>
			&& !type_trait::Is_StringType_V<ONLY_TYPE(U)>
			&& std::is_same_v<C, ONLY_TYPE(U)>
			, const T>
			GetValuePtr(const T obj)
		{
			return obj;
		}

		// primitive types (int, double, unsigned long long,...)
		template<typename CharType, typename T, typename C = CharType, typename U = T>
		constexpr
			std::enable_if_t <type_trait::Is_CharType_V<C>
			&& std::is_arithmetic_v<U>
			, const U*>
			GetValuePtr(const T& obj)
		{
			return &obj;
		}

		// Non-primitive custom types - Must have a function ToString that returns a C++ STL string type
		template<typename CharType, typename T, typename C = CharType, typename U = T>
		constexpr
			std::enable_if_t <(type_trait::Is_CharType_V<C>
				&& !std::is_arithmetic_v<std::decay_t<U>>
				&& !type_trait::Is_StringType_V<U>
				&& !std::is_pointer_v<std::decay_t<U>>
				), const std::basic_string<C>*>
			GetValuePtr(T obj)
		{
			static std::basic_string<CharType> str;
			str = obj.ToString();
			return &str;
		}

		template<typename CharType, typename T, typename ...Ts>
		constexpr std::basic_string<CharType> MakeStringFromData(const std::basic_string<CharType>& format, T arg1, Ts ...argPack)
		{
			constexpr auto npos = std::basic_string<CharType>::npos;

			CharType* buffer = nullptr;
			size_t length = npos;

			if _CONSTEXPR_IF(std::is_same_v<CharType, char>)
			{
				length = static_cast<size_t>(_snprintf(nullptr, 0, format.data(), arg1, argPack...) + 1);
				if (length == 0) throw std::runtime_error("Error during char type \"ToString(...)\" formatting: string returned length == 0");
				buffer = new CharType[length]();
				sprintf_s(buffer, length, format.data(), arg1, argPack...);
			}
			else if _CONSTEXPR_IF(std::is_same_v<CharType, wchar_t>)
			{
				length = static_cast<size_t>(_snwprintf(nullptr, 0, format.data(), arg1, argPack...) + 1);
				if (length == 0) throw std::runtime_error("Error during wchar_t type \"ToString(...)\" formatting: string returned length == 0");
				buffer = new CharType[length]();
				swprintf_s(buffer, length, format.data(), arg1, argPack...);
			}
			else
			{
				const auto fmt = kString::Convert<wchar_t>(format);
				const auto str = MakeStringFromData<wchar_t>(fmt, arg1, argPack...);
				const auto text = kString::Convert<CharType>(str);

				return text;
			}

			const auto formattedText = std::basic_string<CharType>(buffer, buffer + (CAST(ptrdiff_t, length) - 1));
			delete[] buffer;
			return formattedText;
		}

		template<typename CharType, size_t Size>
		std::deque<std::pair<unsigned char, std::string>> CreateIdentifiers(std::basic_string<CharType>& fmt,  std::array<std::any, Size>& elems)
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
		USE_RESULT constexpr std::basic_string<CharType> ToString(const std::basic_string<CharType>& format, const T& arg, const Ts& ...argPack)
		{
			using DataTypes = std::variant<std::monostate, T, Ts...>;

			static constexpr auto printfSymbol = CharType('%');
			static constexpr auto openerSymbol = CharType('{');
			static constexpr auto closerSymbol = CharType('}');
			static constexpr auto precisionSymbol = CharType(':');
			static constexpr auto nullTerminator = type_trait::s_NullTerminator<CharType>;
			static constexpr auto npos = std::basic_string<CharType>::npos;


			if (auto pfSymPos = format.find(printfSymbol); pfSymPos != npos)
			{
				return MakeStringFromData<CharType>(format, GetValue<CharType, T>(arg), GetValue<CharType, Ts>(argPack)...);
			}

			std::basic_string<CharType> fmt(format);

			std::array<std::any, std::variant_size_v<DataTypes> -1> elems = { GetValuePtr<CharType, T>(arg), GetValuePtr<CharType, Ts>(argPack)... };

			std::deque<std::pair<unsigned char, std::string>> identifiers = CreateIdentifiers(fmt, elems);

			std::basic_string<CharType> finalString;
			for (const auto& id : identifiers)
			{
				const auto& val = elems[id.first];
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

				if (id.second.find("void") != npos)
				{
					auto data = std::any_cast<const void*>(val);
					currentSection.append(stringify::StringifyPointer<CharType>(data, padding));
					finalString.append(currentSection);
				}
				else if (id.second.find("basic_string_view") != npos)
				{
					const auto data = std::any_cast<const std::basic_string_view<CharType>*>(val);
					currentSection.erase(replacePos);
					currentSection.insert(replacePos, data->data());
					finalString.append(currentSection);
				}
				else if (id.second.find("basic_string") != npos)
				{
					const auto data = std::any_cast<const std::basic_string<CharType>*>(val);
					currentSection.erase(replacePos);
					currentSection.insert(replacePos, data->data());
					finalString.append(currentSection);
				}
				else if (id.second.find("unsigned") != npos)
				{
					if (id.second.find("char") != npos)
					{
						auto data = std::any_cast<const unsigned char*>(val);
						currentSection.append(stringify::StringUnsignedIntegral<CharType>(*data, padding));
						finalString.append(currentSection);
					}
					else if (id.second.find("short") != npos)
					{
						auto data = std::any_cast<const unsigned short*>(val);
						currentSection.append(stringify::StringUnsignedIntegral<CharType>(*data, padding));
						finalString.append(currentSection);
					}
					else if (id.second.find("int") != npos)
					{
						if (id.second.find("__int64") != npos)
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
					else if (auto longPos = id.second.find("long"); longPos != npos)
					{
						auto long2Pos = id.second.find_first_of("long long");
						if (long2Pos != npos)
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
				else if (const auto longPos = id.second.find("long"); longPos != npos)
				{
					if (id.second.find_first_of("double", longPos + 4) != npos)
					{
						auto data = std::any_cast<const long double*>(val);
						currentSection += stringify::StringFloatingPoint<CharType>(*data, padding);
						finalString.append(currentSection);
					}
					else if (id.second.find_first_of("int", longPos + 4) != npos)
					{
						auto data = std::any_cast<const long int*>(val);
						currentSection += stringify::StringSignedIntegral<CharType>(*data, padding);
						finalString.append(currentSection);
					}
					else if (const auto long2Pos = id.second.find_first_of("long long"); long2Pos != npos)
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
				else if (id.second.find("char") != npos)
				{
					const auto data = std::any_cast<const CharType*>(val);
					currentSection.erase(replacePos);
					currentSection.insert(replacePos, data);
					finalString.append(currentSection);
				}
				else if (id.second.find("short") != npos)
				{
					auto data = std::any_cast<const short*>(val);
					currentSection += stringify::StringSignedIntegral<CharType>(*data, padding);
					finalString.append(currentSection);
				}
				else if (id.second.find("int") != npos)
				{
					if (id.second.find("__int64") != npos)
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
				else if (kString::HasString (id.second, "double"))
				{
					const auto data = std::any_cast<const double*>(val);
					currentSection += stringify::StringFloatingPoint<CharType>(*data, padding);
					finalString.append(currentSection);
				}
				else if (id.second.find("float") != npos)
				{
					const auto data = std::any_cast<const float*>(val);
					currentSection.append(stringify::StringFloatingPoint<CharType>(*data, padding));
					finalString.append(currentSection);
				}
				else if (id.second.find("bool") != npos)
				{
					const auto res = std::any_cast<const bool*>(val);
					std::basic_string_view<CharType> data;
					if _CONSTEXPR_IF(std::is_same_v<CharType, char>)
						data = *res ? "true" : "false";
					else if _CONSTEXPR_IF(std::is_same_v<CharType, wchar_t>)
						data = *res ? L"true" : L"false";
					else if _CONSTEXPR_IF(std::is_same_v<CharType, char16_t>)
						data = *res ? u"true" : u"false";
					else if _CONSTEXPR_IF(std::is_same_v<CharType, char32_t>)
						data = *res ? U"true" : U"false";
#ifdef __cpp_char8_t
					else if _CONSTEXPR_IF(std::is_same_v<CharType, char8_t>)
						data = *res ? u8"true" : u8"false";
#endif
					currentSection.erase(replacePos);
					currentSection.insert(replacePos, data);

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
			const std::basic_string<CharType> fmt(format);
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

#	pragma warning(pop)
#endif