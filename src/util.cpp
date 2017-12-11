
#include "util.h"

namespace util
{
	std::string strto(std::string &str, const std::string &substr)
	{
		std::string ret{};
		while (str.length() >= substr.length())
		{
			if (str.substr(0, substr.length()) == substr)
			{
				str.erase(0, substr.length());
				return ret;
			}
			ret += str[0];
			str.erase(0, 1);
		}
		return ret;
	}

	std::string strto(const std::string &str, const std::string &substr)
	{
		std::string nonconst = str;
		return strto(nonconst, substr);
	}

	std::string utf8_popchar(std::string &str)
	{
		std::string ret{};
		if (str.length() == 0) return ret;
		char c = str[0];
		unsigned int len = 0;
		if ((c & 0x80) == 0x0) len = 1;
		else if ((c & 0xE0) == 0xC0) len = 2;
		else if ((c & 0xF0) == 0xE0) len = 3;
		else if ((c & 0xF8) == 0xF0) len = 4;
		else if ((c & 0xFC) == 0xF8) len = 5;
		else if ((c & 0xFE) == 0xFC) len = 6;
		else throw std::runtime_error{"Invalid UTF-8 byte sequence"};
		ret += c;
		unsigned int i;
		for (i = 1; i < len && i < str.length(); i++) ret += str[i];
		str = str.substr(i);
		return ret;
	}
	
	int64_t utf8_codepoint(const std::string &str)
	{
		int64_t ret = 0;
		for (unsigned char c : str)
		{
			if ((c & 0x80) == 0x0) ret = (ret << 7) | c;
			else if ((c & 0xC0) == 0x80) ret = (ret << 6) | (c & 0x3F);
			else if ((c & 0xE0) == 0xC0) ret = (ret << 5) | (c & 0x1F);
			else if ((c & 0xF0) == 0xE0) ret = (ret << 4) | (c & 0x0F);
			else if ((c & 0xF8) == 0xF0) ret = (ret << 3) | (c & 0x07);
			else if ((c & 0xFC) == 0xF8) ret = (ret << 2) | (c & 0x03);
			else if ((c & 0xFE) == 0xFC) ret = (ret << 1) | (c & 0x01);
			else throw std::runtime_error{"Invalid UTF-8 byte sequence"};
		}
		return ret;
	}

	std::string basename(const std::string &str, const std::string &substr)
	{
		std::string ret{};
		for (int i = str.length() - 1; i > static_cast<int>(str.length() - substr.length()); i--) ret = str[i] + ret;
		for (int i = str.length() - substr.length(); i >= 0; i--)
		{
			if (str.substr(i, substr.length()) == substr) break;
			ret = str[i] + ret;
		}
		return ret;
	}

	std::string dirname(const std::string &str, const std::string &substr)
	{
		int i;
		for (i = str.length() - substr.length(); i > 0; i--) if (str.substr(i, substr.length()) == substr) break;
		return str.substr(0, i);
	}
	
	bool file_exists(const std::string &path)
	{
		struct stat buf;
		if (stat(path.c_str(), &buf)) return false;
		if (buf.st_mode & S_IFREG) return true;
		return false;
	}
}
