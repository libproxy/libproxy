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

#include <fcntl.h> // For ::open()
#include <cstring> // For memcpy()
#include <sstream> // For int/string conversion (using stringstream)
#include <cstdio>  // For sscanf()

#include "url.hpp"

namespace com {
namespace googlecode {
namespace libproxy {
using namespace std;

static inline int _get_default_port(string scheme) {
        struct servent *serv;
        if ((serv = getservbyname(scheme.c_str(), NULL))) return ntohs(serv->s_port);
        return 0;
}

template <class T>
static inline string _to_string (const T& t) {
	stringstream ss;
	ss << t;
	return ss.str();
}

#define _copyaddr_t(type, addr) (sockaddr*) memcpy(new type, &(addr), sizeof(type))
static inline sockaddr* _copyaddr(const struct sockaddr& addr) {
	switch (addr.sa_family) {
	case (AF_INET):
		return _copyaddr_t(sockaddr_in, addr);
	case (AF_INET6):
		return _copyaddr_t(sockaddr_in6, addr);
	default:
		return NULL;
	}
}

bool url::is_valid(const string __url) {
	url* tmp;
	try                     { tmp = new url(__url); }
	catch (parse_error& pe) { return false; }
    delete tmp;
    return true;
}

url::url(const string url) throw(parse_error, logic_error) {
	char *schm = new char[url.size()];
	char *auth = new char[url.size()];
	char *host = new char[url.size()];
	char *path = new char[url.size()];
	bool port_specified = false;
	this->ips = NULL;

	// Break apart our url into 4 sections: scheme, auth (user/pass), host and path
	// We'll do further parsing of auth and host a bit later
	// NOTE: reset the unused variable after each scan or we get bleed-through
	if (sscanf(url.c_str(),   "%[^:]://%[^@]@%[^/]/%s", schm, auth, host, path) != 4                && !((*path = NULL)) &&  // URL with auth, host and path
		sscanf(url.c_str(),   "%[^:]://%[^@]@%[^/]",    schm, auth, host) != 3                      && !((*auth = NULL)) &&  // URL with auth, host
		sscanf(url.c_str(),   "%[^:]://%[^/]/%s",       schm, host, path) != 3                      && !((*path = NULL)) &&  // URL with host, path
		sscanf(url.c_str(),   "%[^:]://%[^/]",          schm, host) != 2                            && !((*host = NULL)) &&  // URL with host
		!(sscanf(url.c_str(), "%[^:]://%s",             schm, path) == 2 && string("file") == schm) && !((*path = NULL)) &&  // URL with path (ex: file:///foo)
		!(sscanf(url.c_str(), "%[^:]://",               schm) == 1 && (string("direct") == schm || string("wpad") == schm))) // URL with scheme-only (ex: wpad://, direct://)
	{
		delete schm;
		delete auth;
		delete host;
		delete path;
		throw parse_error("Invalid URL: " + url);
	}

	// Set scheme and path
	this->scheme   = schm;
	this->path     = *path ? string("/") + path : "";
	*schm = NULL;
	*path = NULL;

	// Parse auth further
	if (*auth) {
		this->user = auth;
		if (string(auth).find(":") != string:: npos) {
			this->pass = this->user.substr(this->user.find(":")+1);
			this->user = this->user.substr(0, this->user.find(":"));
		}
		*auth = NULL;
	}

	// Parse host further. Basically, we're looking for a port.
	if (*host) {
		this->host = host;
		port_specified = sscanf(host, "%*[^:]:%d", &this->port) == 1;
		if (port_specified)
			this->host = this->host.substr(0, this->host.rfind(':'));
		else
			this->port = _get_default_port(this->scheme);
		*host = NULL;
	}

	// Cleanup
	delete schm;
	delete auth;
	delete host;
	delete path;

	// Verify by re-assembly
	if (this->user != "" && this->pass != "")
		this->orig = this->scheme + "://" + this->user + ":" + this->pass + "@" + this->host;
	else
		this->orig = this->scheme + "://" + this->host;
	if (port_specified)
		this->orig = this->orig + ":" + _to_string<int>(this->port) + this->path;
	else
		this->orig = this->orig + this->path;
	if (this->orig != url)
		throw logic_error("Re-assembly failed!");
}

url::url(const url &url) {
	this->ips = NULL;
	*this = url;
}

url::~url() {
	if (this->ips) {
		for (vector<const sockaddr*>::iterator i = this->ips->begin() ; i != this->ips->end() ; i++)
			delete *i;
		delete this->ips;
	}
}

bool url::operator==(const url& url) const {
	return this->orig == url.to_string();
}

url& url::operator=(const url& url) {
	// Ensure these aren't the same objects
	if (&url == this)
		return *this;

	this->host   = url.host;
	this->orig   = url.orig;
	this->pass   = url.pass;
	this->path   = url.path;
	this->port   = url.port;
	this->scheme = url.scheme;
	this->user   = url.user;

	if (this->ips) {
		// Free any existing ip cache
		for (vector<const sockaddr*>::iterator i = this->ips->begin() ; i != this->ips->end() ; i++)
			delete *i;
		delete this->ips;
		this->ips = NULL;
	}

	if (url.ips) {
		// Copy the new ip cache
		this->ips = new vector<const sockaddr*>();
		for (vector<const sockaddr*>::iterator i = url.ips->begin() ; i != url.ips->end() ; i++)
			this->ips->push_back(_copyaddr(**i));
	}
	return *this;
}

url& url::operator=(string strurl) throw (parse_error) {
	url* tmp = new url(strurl);
	*this = *tmp;
	delete tmp;
	return *this;
}

int url::open() {
	map<string, string> headers;
	return this->open(headers);
}

int url::open(map<string, string> headers) {
	char *joined_headers = NULL;
	string request;
	int sock = -1;

	// In case of a file:// url we open the file and return a handle to it
	if (this->scheme == "file")
		return ::open(this->path.c_str(), O_RDONLY);

	// DNS lookup of host
	if (!this->get_ips(true)) goto error;

	// Iterate through each IP trying to make a connection
	for (vector<const sockaddr*>::iterator i = this->ips->begin() ; i != this->ips->end() ; i++) {
		sock = socket((*i)->sa_family, SOCK_STREAM, 0);
		if (sock < 0) continue;

		if ((*i)->sa_family == AF_INET &&
			!connect(sock, *i, sizeof(struct sockaddr_in)))
			break;
		else if ((*i)->sa_family == AF_INET6 &&
			!connect(sock, *i, sizeof(struct sockaddr_in6)))
			break;

		close(sock);
		sock = -1;
	}
	if (sock < 0) goto error;

	// Set any required headers
	headers["Host"] = this->host;

	// Build the request string
	request = "GET " + this->path + " HTTP/1.1\r\n";
	for (map<string, string>::iterator it = headers.begin() ; it != headers.end() ; it++)
		request += it->first + ": " + it->second + "\r\n";
	request += "\r\n";

	// Send HTTP request
	if (send(sock, request.c_str(), request.size(), 0) != request.size()) goto error;

	// Return the socket, which is ready for reading the response
	return sock;

	error:
		if (sock >= 0) close(sock);
		return -1;
}

string url::get_host() const {
	return this->host;
}

const vector<const sockaddr*>* url::get_ips(bool usedns) {
	// Check the cache
	if (this->ips) return (const vector<const sockaddr*>*) this->ips;

	// Check without DNS first
	if (usedns && this->get_ips(false)) return (const vector<const sockaddr*>*) this->ips;

	// Check DNS for IPs
	struct addrinfo* info;
	struct addrinfo flags;
	flags.ai_family   = AF_UNSPEC;
	flags.ai_socktype = 0;
	flags.ai_protocol = 0;
	flags.ai_flags    = AI_NUMERICHOST;
	if (!getaddrinfo(this->host.c_str(), NULL, usedns ? NULL : &flags, &info)) {
		struct addrinfo* first = info;

		// Create our vector since we actually have a result
		this->ips = new vector<const sockaddr*>();

		// Copy the sockaddr's into this->ips
		for ( ; info ; info = info->ai_next) {
			if (info->ai_addr->sa_family == AF_INET || info->ai_addr->sa_family == AF_INET6) {
				this->ips->push_back(_copyaddr(*(info->ai_addr)));
				((sockaddr_in*)(*(this->ips))[this->ips->size()-1])->sin_port = htons(this->port);
			}
		}

		freeaddrinfo(first);
		return (const vector<const sockaddr*>*) this->ips;
	}

	// No addresses found
	return NULL;
}

string url::get_password() const {
	return this->pass;
}

string url::get_path() const {
	return this->path;
}

int url::get_port() const {
	return this->port;
}

string url::get_scheme() const {
	return this->scheme;
}

string url::get_username() const {
	return this->user;
}

string url::to_string() const {
	return this->orig;
}

}
}
}
