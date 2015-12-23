/* 
 * File:   util.h
 * Author: matt
 *
 * Created on July 28, 2013, 4:48 PM
 */

#ifndef UTIL_H
#define	UTIL_H

#include <string>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <vector>
#include <sys/stat.h>

namespace util
{
	template <typename T> std::string t2s(const T t) // Convert type to string
	{
		std::stringstream ret{};
		ret << t;
		return ret.str();
	}

	template <typename T> T s2t(const std::string &s) // Convert string to type
	{
		//for (char c : s) if (c != ' ' && c != '\t' && c != '-' && (c < 48 || c > 57)) throw std::runtime_error("String is not an int");
		std::stringstream ss{};
		ss << s;
		T ret;
		ss >> ret;
		return ret;
	}
	
	std::string strto(std::string &str, const std::string &substr); // Remove and return the portion of the string up to the supplied substring.  Like getline(), only without sstreams
	std::string strto(const std::string &str, const std::string &substr); // Same as above, without eating up the string
	std::string utf8_popchar(std::string &str); // Remove and return a single UTF-8 character from the front of a string
	int64_t utf8_codepoint(const std::string &str); // Return the codepoint of the UTF-8 character contained in the string
	std::string basename(const std::string &str, const std::string &substr = "/"); // Return the part of the string after the last occurrence of the supplied substring
	std::string dirname(const std::string &str, const std::string &substr = "/"); // Return the part of the string up to the last occurrence of the supplied substring
	bool file_exists(const std::string &path); // Return true if the file exists and false otherwise
	
	struct enum_hash
	{
		template <typename T> inline typename std::enable_if<std::is_enum<T>::value, std::size_t>::type operator ()(T const value) const
		{
			return static_cast<std::size_t>(value);
		}
	};
}

#endif	/* UTIL_H */

