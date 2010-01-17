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

#include <cstdlib>    // For atoi(...)
#include <sys/stat.h> // For stat(...)
#include <cstdio>     // For sscanf(...)

#ifdef _WIN32
#include <winsock2.h>
#define pfsize(st) (st.st_size)
#else
#include <sys/socket.h>
#define pfsize(st) (st.st_blksize * st.st_blocks)
#endif

#include "pac.hpp"

// This mime type should be reported by the web server
#define PAC_MIME_TYPE "application/x-ns-proxy-autoconfig"
// Fall back to checking for this mime type, which servers often report wrong
#define PAC_MIME_TYPE_FB "text/plain"

// This is the maximum pac size (to avoid memory attacks)
#define PAC_MAX_SIZE 102400

namespace com {
namespace googlecode {
namespace libproxy {
using namespace std;

static inline string _readline(int fd)
{
	// Read a character.
	// If we don't get a character, return empty string.
	// If we are at the end of the line, return empty string.
	char c = '\0';
	if (read(fd, &c, 1) != 1 || c == '\n') return "";
	return string(1, c) + _readline(fd);
}

pac::~pac() {
	delete this->pacurl;
}

pac::pac(const pac& pac) {
	this->pacurl = new libproxy::url(*pac.pacurl);
	this->cache  = pac.cache;
}

pac::pac(const url& url) throw (io_error) {
	int sock;
	bool correct_mime_type;
	unsigned long int content_length = 0, status = 0;

	this->pacurl = new libproxy::url(url.to_string().find("pac+") == 0 ? url.to_string().substr(4) : url);

	/* Get the pxPAC */
	map<string, string> headers;
	headers["Accept"]     = PAC_MIME_TYPE;
	headers["Connection"] = "close";
	sock = this->pacurl->open(headers);
	if (sock < 0) throw io_error("Unable to connect: " + url.to_string());

	if (url.get_scheme() != "file")
	{
		/* Verify status line */
		string line = _readline(sock);
		if (sscanf(line.c_str(), "HTTP/1.%*d %lu", &status) != 1 || status != 200) goto error;

		/* Check for correct mime type and content length */
		for (line = _readline(sock) ; line != "\r" && line != "" ; line = _readline(sock)) {
			// Check for content type
			if (line.find("Content-Type: ") == 0 &&
				(line.find(PAC_MIME_TYPE) != string::npos ||
				 line.find(PAC_MIME_TYPE_FB) != string::npos))
				correct_mime_type = true;

			// Check for content length
			else if (content_length == 0)
				sscanf(line.c_str(), "Content-Length: %lu", &content_length);
		}

		// Get content
		if (!content_length || content_length > PAC_MAX_SIZE || !correct_mime_type) goto error;
		char *buffer = new char[content_length];
		for (size_t recvd=0 ; recvd < content_length ; )
			recvd += recv(sock, buffer + recvd, content_length - recvd, 0);
		this->cache = buffer;
		delete buffer;
	}
	else
	{ /* file:// url */
		struct stat st;

		if (fstat(sock, &st) || pfsize(st) > PAC_MAX_SIZE) goto error;
		char *buffer = new char[pfsize(st)+1];
		if (read(sock, buffer, pfsize(st)) == 0) {
			delete buffer;
			goto error;
		}
		this->cache = buffer;
		delete buffer;
	}

	// Clean up
	close(sock);
	return;

error:
	if (sock >= 0) close(sock);
	throw io_error("Unable to read PAC from " + url.to_string());
}

url pac::get_url() const {
	return *(this->pacurl);
}

string pac::to_string() const {
	return this->cache;
}

}
}
}
