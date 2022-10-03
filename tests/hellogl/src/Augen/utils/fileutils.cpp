/*
 * Copyright (c) 2017 - 2020 -- Élie Michel <elie.michel@exppad.com>
 * This is private research work, DO NOT SHARE without the explicit
 * consent of the authors.
 */

#include <regex>
#include <algorithm>
#include <string>

#include "utils/fileutils.h"
#include "Logger.h"

using namespace std;

string baseDir(const string & path) {
	std::string fixedPath = fixPath(path);
	size_t pos = fixedPath.find_last_of(PATH_DELIM);
	return pos != string::npos ? fixedPath.substr(0, pos) : "";
}

string shortFileName(const string& path) {
	size_t pos = path.find_last_of(PATH_DELIM);
	return pos != string::npos ? path.substr(pos + 1) : path;
}

string fixPath(const string & path) {
	string p = path;
	p = replaceAll(p, "/", PATH_DELIM);
	p = replaceAll(p, "\\", PATH_DELIM);
	return p;
}

bool isAbsolutePath(const string & path) {
#ifdef _WIN32
	return path.length() >= 2 && path[1] == ':';
#else
	return path.length() >= 1 && path[0] == '/';
#endif
}

string canonicalPath(const string & path) {
	std::string out = path;
	regex r(string() + PATH_DELIM_ESCAPED + "[^" + PATH_DELIM_ESCAPED + "]+" + PATH_DELIM_ESCAPED + "\\.\\.");
	while (regex_search(out, r)) {
		out = regex_replace(out, r, "", regex_constants::format_first_only);
	}
	return out;
}

string resolvePath(const string & path, const string & basePath) {
	if (isAbsolutePath(path)) {
		return path;
	} else {
		return canonicalPath(joinPath(basePath, path));
	}
}
