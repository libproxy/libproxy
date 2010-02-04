/*******************************************************************************
 * libproxy - A library for proxy configuration
 * Copyright (C) 2009 Nathaniel McCallum <nathaniel@natemccallum.com>
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

#include <sys/stat.h> // For stat()
#ifndef WIN32
#include <dirent.h>   // For opendir(), readdir(), closedir()
#endif

#include "module_manager.hpp"
using namespace std;
using namespace com::googlecode::libproxy;

#define _str(s) #s
#define __str(s) _str(s)

static vector<string> strsplit(const char* cstr, string delimiter) {
	vector<string> v;
	string str = cstr ? cstr : "";

	for (size_t i=str.find(delimiter) ; i != string::npos ; i=str.find(delimiter)) {
		v.push_back(str.substr(0, i));
		str = str.substr(i+delimiter.size());
	}
	if (str != "")
		v.push_back(str);

	return v;
}

static bool globmatch(string glob, string str)
{
	vector<string> segments = strsplit(glob.c_str(), "*");
	for (vector<string>::iterator i=segments.begin() ; i != segments.end() ; i++)
	{
		// Handle when the glob does not end with '*'
		// (insist the segment match the end of the string)
		if (i == segments.end() &&  *i != "" && str != "")
			return false;

		// Search for this segment in this string
		size_t offset = str.find(*i);

		// If the segment isn't found at all, its not a match
		if (offset == string::npos)
			return false;

		// Handle when the glob does not start with '*'
		// (insist the segment match the start of the string)
		if (i == segments.begin() && *i != "" && offset != 0)
			return false;

		// Increment further into the string
		str = str.substr(offset + i->size());
	}
	return true;
}

module_manager::~module_manager() {
	// Free all modules
	for (map<string, vector<module*> >::iterator i=this->modules.begin() ; i != this->modules.end() ; i++) {
		for (vector<module*>::iterator j=i->second.begin() ; j != i->second.end() ; j++)
			delete *j;
		i->second.clear();
	}
	this->modules.clear();

	// Free all dl_modules
	for (set<dl_module*>::iterator i=this->dl_modules.begin() ; i != this->dl_modules.end() ; i++)
		delete *i;
	this->dl_modules.clear();
}

bool module_manager::load_file(const string filename, const string condsym) {
	dl_module* dlobj = NULL;
	string modname   = module::make_name(filename);

	// If the conditional symbol was specified
	// ensure that the symbol exists within our
	// current process before loading.
	if (condsym != "") {
		dlobj = new dl_module();
		if (!dlobj->get_symbol(condsym)) {
			delete dlobj;
			return false;
		}
		delete dlobj;
	}

	// Stat the file to make sure it is a file
	struct stat st;
	if (stat(filename.c_str(), &st) != 0) return false;
	if ((st.st_mode & S_IFMT) != S_IFREG) return false;

	// Prepare for blacklist check
	vector<string> blacklist = strsplit(getenv("PX_MODULE_BLACKLIST"), ",");
	vector<string> whitelist = strsplit(getenv("PX_MODULE_WHITELIST"), ",");
	bool           doload    = true;

	// Check our whitelist/blacklist to see if we should load this module
	for (vector<string>::iterator i=blacklist.begin() ; i != blacklist.end() ; i++)
		if (globmatch(*i, modname))
			doload = false;
	for (vector<string>::iterator i=whitelist.begin() ; i != whitelist.end() ; i++)
		if (globmatch(*i, modname))
			doload = true;

	if (!doload)
		return false;

	// Load the module
	dlobj = new dl_module(filename);

	// Insert the module
	if (this->dl_modules.insert(dlobj).second == false) {
		delete dlobj;
		return false;
	}

	// Call the INIT function
	module_manager::LOAD_TYPE load = (module_manager::LOAD_TYPE)
					dlobj->get_symbol(__str(PX_MODULE_LOAD_NAME));
	if (!load || !load(*this)) {
		this->dl_modules.erase(dlobj);
		delete dlobj;
		return false;
	}

	return true;
}

bool module_manager::load_dir(const string dirname) {
	bool loaded = false;
#ifdef WIN32
	WIN32_FIND_DATA fd;
	HANDLE search;

	// Create the file search string and do the search
	string srch = dirname + PATHSEP + "*";
	search = FindFirstFile(srch.c_str(), &fd);

	// If we got results, try to load each file
	if (search != INVALID_HANDLE_VALUE) {
		do {
			string tmp = dirname + PATHSEP + fd.cFileName;
			loaded = this->load_file(tmp) || loaded;
		} while (FindNextFile(search, &fd));
		FindClose(search);
	}
#else
	struct dirent *ent;

	// Open the module dir
	DIR *moduledir = opendir(dirname.c_str());
	if (moduledir) {
		// For each module...
		while((ent = readdir(moduledir))) {
			// Load the module
			string tmp = dirname + PATHSEP + ent->d_name;
			loaded = this->load_file(tmp) || loaded;
		}
		closedir(moduledir);
	}
#endif

	return loaded;
}
