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
#ifdef WIN32
#include <io.h>
#define open _open
#define O_RDONLY _O_RDONLY
#define close _close
#endif
#include <fcntl.h> // For ::open()
#include <cstring> // For memcpy()
#include <sstream> // For int/string conversion (using stringstream)
#include <cstdio>  // For sscanf()
#include <cstdlib>    // For atoi()
#include <sys/stat.h> // For stat()

#ifdef WIN32
#include <io.h>
#define pfsize(st) (st.st_size)
#define close _close
#define read _read
#else
#define pfsize(st) (st.st_blksize * st.st_blocks)
#endif

#include "url.hpp"
using namespace libproxy;
using namespace std;

// This mime type should be reported by the web server
#define PAC_MIME_TYPE "application/x-ns-proxy-autoconfig"
// Fall back to checking for this mime type, which servers often report wrong
#define PAC_MIME_TYPE_FB "text/plain"

// This is the maximum pac size (to avoid memory attacks)
#define PAC_MAX_SIZE 102400

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
	try                  { tmp = new url(__url); }
	catch (parse_error&) { return false; }
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
		this->port = _get_default_port(this->scheme);
		if (this->scheme.find('+') != this->scheme.npos)
			this->port = _get_default_port(this->scheme.substr(this->scheme.find('+')+1));

		int hostlen = strlen(host);
		for (int i=hostlen-1 ; i >= 0 ; i--) {
			if (host[i] >= '0' && host[i] <= '9') continue;  // Still might be a port
			if (host[i] != ':' || hostlen - 1 == i) break;   // Definitely not a port
			if (sscanf(host + i + 1, "%hu", &this->port) != 1) break; // Parse fail, should never happen
			port_specified = true;
			host[i] = '\0'; // Terminate at the port ':'
			break;
		}

		this->host = host;
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
		for (int i=0 ; this->ips[i] ; i++)
			delete this->ips[i];
		delete this->ips;
	}
}

bool url::operator==(const url& url) const {
	return this->orig == url.to_string();
}

url& url::operator=(const url& url) {
	int i=0;

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
		for (i=0 ; this->ips[i] ; i++)
			delete this->ips[i];
		delete this->ips;
		this->ips = NULL;
	}

	if (url.ips) {
		// Copy the new ip cache
		for (i=0 ; url.ips[i] ; i++);
		this->ips = new sockaddr*[i];
		for (i=0 ; url.ips[i] ; i++)
			this->ips[i] = _copyaddr(*url.ips[i]);
	}
	return *this;
}

url& url::operator=(string strurl) throw (parse_error) {
	url* tmp = new url(strurl);
	*this = *tmp;
	delete tmp;
	return *this;
}

string url::get_host() const {
	return this->host;
}

sockaddr const* const* url::get_ips(bool usedns) {
	// Check the cache
	if (this->ips) return this->ips;

	// Check without DNS first
	if (usedns && this->get_ips(false)) return this->ips;

	// Check DNS for IPs
	struct addrinfo* info;
	struct addrinfo flags;
	flags.ai_family   = AF_UNSPEC;
	flags.ai_socktype = 0;
	flags.ai_protocol = 0;
	flags.ai_flags    = AI_NUMERICHOST;
	if (!getaddrinfo(this->host.c_str(), NULL, usedns ? NULL : &flags, &info)) {
		struct addrinfo* first = info;
		unsigned int i = 0;

		// Get the size of our array
		for (info = first ; info ; info = info->ai_next)
			i++;

		// Return NULL if no IPs found
		if (i == 0)
			return this->ips = NULL;

		// Create our array since we actually have a result
		this->ips = new sockaddr*[++i];
		memset(this->ips, NULL, sizeof(sockaddr*)*i);

		// Copy the sockaddr's into this->ips
		for (i = 0, info = first ; info ; info = info->ai_next) {
			if (info->ai_addr->sa_family == AF_INET || info->ai_addr->sa_family == AF_INET6) {
				this->ips[i] = _copyaddr(*(info->ai_addr));
				if (!this->ips[i]) break;
				((sockaddr_in **) this->ips)[i++]->sin_port = htons(this->port);
			}
		}

		freeaddrinfo(first);
		return this->ips;
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

uint16_t url::get_port() const {
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

static inline string _readline(int fd) {
	// Read a character.
	// If we don't get a character, return empty string.
	// If we are at the end of the line, return empty string.
	char c = '\0';
	if (read(fd, &c, 1) != 1 || c == '\n') return "";
	return string(1, c) + _readline(fd);
}

char* url::get_pac() {
	int sock = -1;
	bool correct_mime_type = false, chunked = false;
	unsigned long int content_length = 0, status = 0;
	char* buffer = NULL;
	string request;

	// In case of a file:// url we open the file and read it
	if (this->scheme == "file") {
		struct stat st;
		if ((sock = ::open(this->path.c_str(), O_RDONLY)) < 0)
			return NULL;
		if (!fstat(sock, &st) && pfsize(st) < PAC_MAX_SIZE) {
			buffer = new char[pfsize(st)+1];
			if (read(sock, buffer, pfsize(st)) == 0) {
				delete buffer;
				buffer = NULL;
			}
		}
		return buffer;
	}

	// DNS lookup of host
	if (!this->get_ips(true))
		return NULL;

	// Iterate through each IP trying to make a connection
	// Stop at the first one
	for (int i=0 ; this->ips[i] ; i++) {
		sock = socket(this->ips[i]->sa_family, SOCK_STREAM, 0);
		if (sock < 0) continue;

		if (this->ips[i]->sa_family == AF_INET &&
			!connect(sock, this->ips[i], sizeof(struct sockaddr_in)))
			break;
		else if (this->ips[i]->sa_family == AF_INET6 &&
			!connect(sock, this->ips[i], sizeof(struct sockaddr_in6)))
			break;

		close(sock);
		sock = -1;
	}

	// Test our socket
	if (sock < 0) return NULL;

	// Build the request string
	request  = "GET " + this->path + " HTTP/1.1\r\n";
	request += "Host: " + this->host + "\r\n";
	request += "Accept: " + string(PAC_MIME_TYPE) + "\r\n";
	request += "Connection: close\r\n";
	request += "\r\n";

	// Send HTTP request
	if ((size_t) send(sock, request.c_str(), request.size(), 0) != request.size()) {
		close(sock);
		return NULL;
	}

	/* Verify status line */
	string line = _readline(sock);
	if (sscanf(line.c_str(), "HTTP/1.%*d %lu", &status) == 1 && status == 200) {
		/* Check for correct mime type and content length */
		for (line = _readline(sock) ; line != "\r" && line != "" ; line = _readline(sock)) {
			// Check for content type
			if (line.find("Content-Type: ") == 0 &&
				(line.find(PAC_MIME_TYPE) != string::npos ||
				 line.find(PAC_MIME_TYPE_FB) != string::npos))
				correct_mime_type = true;

			// Check for chunked encoding
			else if (line.find("Content-Transfer-Encoding: chunked") == 0)
				chunked = true;

			// Check for content length
			else if (content_length == 0)
				sscanf(line.c_str(), "Content-Length: %lu", &content_length);
		}

		// Get content
		unsigned int recvd = 0;
		buffer = new char[PAC_MAX_SIZE];
		*buffer = '\0';
		do {
			unsigned int chunk_length;

			if (chunked) {
				// Discard the empty line if we received a previous chunk
				if (recvd > 0) _readline(sock);

				// Get the chunk-length line as an integer
				if (sscanf(_readline(sock).c_str(), "%x", &chunk_length) != 1 || chunk_length == 0) break;

				// Add this chunk to our content length,
				// ensuring that we aren't over our max size
				content_length += chunk_length;
				if (content_length >= PAC_MAX_SIZE) break;
			}

			while (recvd != content_length) {
				int r = recv(sock, buffer + recvd, content_length - recvd, 0);
				if (r < 0) break;
				recvd += r;
			}
			buffer[content_length] = '\0';
		} while (chunked);

		if (string(buffer).size() != content_length) {
			delete buffer;
			buffer = NULL;
		}
	}

	// Clean up
	close(sock);
	return buffer;
}
