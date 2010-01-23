/*******************************************************************************
 * libproxy - A library for proxy configuration
 * Copyright (C) 2006 Nathaniel McCallum <nathaniel@natemccallum.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 ******************************************************************************/

#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>

#include "config_file.hpp"

namespace com {
namespace googlecode {
namespace libproxy {

#define PX_CONFIG_FILE_DEFAULT_SECTION "__DEFAULT__"

static string trim(string str, const char *set) {
	const string::size_type first = str.find_first_not_of(set);
	if (first != string::npos)
		str = str.substr(first, str.find_last_not_of(set)-first+1);
	return str;
}

bool config_file::get_value(string key, string& value) {
	return this->get_value(PX_CONFIG_FILE_DEFAULT_SECTION, key, value);
}

bool config_file::get_value(string section, string key, string& value) {
	if (this->sections.find(section) == this->sections.end())
		return false;
	if (this->sections[section].find(key) == this->sections[section].end())
		return false;
	value = this->sections[section][key];
	return true;
}

bool config_file::is_stale() {
	struct stat st;
	return (!stat(this->filename.c_str(), &st) && st.st_mtime > this->mtime);
}

bool config_file::load(string filename) {
	// Stat the file and get its mtime
	struct stat st;
	if (stat(filename.c_str(), &st))
		return false;
	this->filename = filename;
	this->mtime    = st.st_mtime;

	// Open the file
	ifstream file(filename.c_str());
	if (!file.is_open())
		return false;

	string current = PX_CONFIG_FILE_DEFAULT_SECTION;

	for (string line="" ; !file.eof() ; getline(file, line)) {
		if (file.fail()) {
			this->sections.clear();
			file.close();
			return false;
		}

		// Strip the line
		line = trim(line, " \t");

		// Check for comment and/or empty line
		if (line[0] == '#' || line[0] == ';' || line == "") continue;

		// If we have a new section, get the current section name
		if (line[0] == '[' || line[line.size()-1] == ']')
			current = trim(line, "[]");

		// Otherwise set our value
		else if (line.find('=') != string::npos)
			this->sections[current][line.substr(0, line.find('='))] = line.substr(line.find('=')+1);
	}

	file.close();
	return true;
}

}
}
}
