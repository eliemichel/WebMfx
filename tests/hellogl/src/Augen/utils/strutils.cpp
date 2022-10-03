/*
 * Copyright (c) 2017 - 2020 -- Élie Michel <elie.michel@exppad.com>
 * This is private research work, DO NOT SHARE without the explicit
 * consent of the authors.
 */

#include "utils/strutils.h"

#include <string>
#include <algorithm>
#include <functional>
#include <cctype>

// trim from start (in place)
void ltrim(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(),
		[](int c) {return !std::isspace(c); }
	));
}

// trim from end (in place)
void rtrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(),
		[](int c) {return !std::isspace(c); }
	).base(), s.end());
}

void trim(std::string& s) {
	ltrim(s);
	rtrim(s);
}

std::string toLower(const std::string & s) {
	std::string r = s;
	std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c) {
		return static_cast<unsigned char>(std::tolower(static_cast<int>(c)));
	});
	return r;
}

bool startsWith(const std::string & s, const std::string & prefix) {
	if (prefix.size() > s.size()) return false;
	return equal(prefix.begin(), prefix.end(), s.begin());
}

bool endsWith(const std::string & s, const std::string & postfix) {
	if (postfix.size() > s.size()) return false;
	return equal(postfix.rbegin(), postfix.rend(), s.rbegin());
}

std::string replaceAll(std::string str, const std::string& search, const std::string& replace) {
	size_t pos = 0;
	while ((pos = str.find(search, pos)) != std::string::npos) {
		str.replace(pos, search.length(), replace);
		pos += replace.length();
	}
	return str;
}

std::string bitname(int flags, int flagCount)
{
	std::string text(flagCount, '0');
	for (int f = 0; f < flagCount; ++f) {
		if ((flags & (1 << f)) != 0) {
			text[f] = '1';
		}
	}
	return text;
}

std::vector<std::string> split(const std::string& str, char sep)
{
	std::vector<std::string> vec;
	size_t offset = 0;

	for (size_t cursor = 0; cursor < str.length(); ++cursor) {
		if (str[cursor] == sep) {
			vec.push_back(str.substr(offset, cursor - offset));
			offset = cursor + 1;
		}
	}

	return vec;
}
