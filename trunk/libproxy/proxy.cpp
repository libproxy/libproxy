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

#include <vector>
#include <cstring>  // For strdup()
#include <iostream> // For cout

#include <libmodman/module_manager.hpp>

#include "extension_config.hpp"
#include "extension_ignore.hpp"
#include "extension_network.hpp"
#include "extension_pacrunner.hpp"
#include "extension_wpad.hpp"

#ifdef WIN32
#define setenv(name, value, overwrite) SetEnvironmentVariable(name, value)
#define strdup _strdup
#endif

namespace libproxy {
using namespace std;

class proxy_factory {
public:
	proxy_factory();
	~proxy_factory();
	vector<string> get_proxies(string url);

private:
#ifdef WIN32
	HANDLE mutex;
#else
	pthread_mutex_t mutex;
#endif
	module_manager  mm;
	char*  pac;
	url*   pacurl;
	bool   wpad;
};

bool istringcmp(string a, string b) {
    transform( a.begin(), a.end(), a.begin(), ::tolower );
    transform( b.begin(), b.end(), b.begin(), ::tolower );
    return ( a == b );
}

// Convert the PAC formatted response into our proxy URL array response
static vector<string>
_format_pac_response(string response)
{
	vector<string> retval;

	// Skip ahead one character if we start with ';'
	if (response[0] == ';')
		return _format_pac_response(response.substr(1));

	// If the string contains a delimiter (';')
	if (response.find(';') != string::npos) {
		retval   = _format_pac_response(response.substr(response.find(';')+1));
		response = response.substr(0, response.find(';'));
	}

	// Strip whitespace
	response = response.substr(response.find_first_not_of(" \t"), response.find_last_not_of(" \t")+1);

	// Get the method and the server
	string method = "";
	string server = "";
	if (response.find_first_of(" \t") == string::npos)
		method = response;
	else {
		method = response.substr(0, response.find_first_of(" \t"));
		server = response.substr(response.find_first_of(" \t") + 1);
	}

	// Insert our url value
	if (istringcmp(method, "proxy") && url::is_valid("http://" + server))
		retval.insert(retval.begin(), string("http://") + server);
	else if (istringcmp(method, "socks") && url::is_valid("http://" + server))
		retval.insert(retval.begin(), string("socks://") + server);
	else if (istringcmp(method, "direct"))
		retval.insert(retval.begin(), string("direct://"));

	return retval;
}

proxy_factory::proxy_factory() {
#ifdef WIN32
	this->mutex = CreateMutex(NULL, false, NULL);
	WaitForSingleObject(this->mutex, INFINITE);
#else
	pthread_mutex_init(&this->mutex, NULL);
	pthread_mutex_lock(&this->mutex);
#endif
	this->pac    = NULL;
	this->pacurl = NULL;
	this->wpad   = false;

	// Register our types
	this->mm.register_type<config_extension>();
	this->mm.register_type<ignore_extension>();
	this->mm.register_type<network_extension>();
	this->mm.register_type<pacrunner_extension>();
	this->mm.register_type<wpad_extension>();

	// Load all modules
	this->mm.load_dir(MODULEDIR);
	this->mm.load_dir(MODULEDIR, false);

#ifdef WIN32
	ReleaseMutex(this->mutex);
#else
	pthread_mutex_unlock(&this->mutex);
#endif
}

proxy_factory::~proxy_factory() {
#ifdef WIN32
	WaitForSingleObject(this->mutex, INFINITE);
#else
	pthread_mutex_lock(&this->mutex);
#endif
	if (this->pac) delete this->pac;
	if (this->pacurl) delete this->pacurl;
#ifdef WIN32
	ReleaseMutex(this->mutex);
#else
	pthread_mutex_unlock(&this->mutex);
	pthread_mutex_destroy(&this->mutex);
#endif
}


vector<string> proxy_factory::get_proxies(string __url) {
	url*                       realurl = NULL;
	url                        confurl("direct://");
	bool                       ignored = false, invign = false;
	string                     confign;
	config_extension*          config;
	vector<network_extension*> networks;
	vector<config_extension*>  configs;
	vector<ignore_extension*>  ignores;
	vector<string>             response;
	const char*                debug = getenv("_PX_DEBUG");

	// Check to make sure our url is valid
	if (!url::is_valid(__url))
		goto do_return;
	realurl = new url(__url);

#ifdef WIN32
	WaitForSingleObject(this->mutex, INFINITE);
#else
	pthread_mutex_lock(&this->mutex);
#endif

	// Check to see if our network topology has changed...
	networks = this->mm.get_extensions<network_extension>();
	for (vector<network_extension*>::iterator i=networks.begin() ; i != networks.end() ; i++) {
		// If it has, reset our wpad/pac setup and we'll retry our config
		if ((*i)->changed()) {
			if (debug) cout << "Network changed" << endl;
			vector<wpad_extension*> wpads = this->mm.get_extensions<wpad_extension>();
			for (vector<wpad_extension*>::iterator j=wpads.begin() ; j != wpads.end() ; j++)
				(*j)->rewind();
			if (this->pac) delete this->pac;
			this->pac = NULL;
			break;
		}
	}

	configs = this->mm.get_extensions<config_extension>();
	for (vector<config_extension*>::iterator i=configs.begin() ; i != configs.end() ; i++) {
		config = *i;

		// Try to get the confurl
		try { confurl = (config)->get_config(*realurl); }
		catch (runtime_error e) { goto invalid_config; }

		// Validate the input
		if (!(confurl.get_scheme() == "http" ||
			  confurl.get_scheme() == "socks" ||
			  confurl.get_scheme().substr(0, 4) == "pac+" ||
			  confurl.get_scheme() == "wpad" ||
			  confurl.get_scheme() == "direct"))
			goto invalid_config;

		config->set_valid(true);
		confign = config->get_ignore(*realurl);
		break;

		invalid_config:
			confurl = "direct://";
			confign = "";
			(config)->set_valid(false);
			config = NULL;
	}
	if (debug) cout << "Using config: " << typeid(*config).name() << endl;
	if (debug) cout << "Using ignore: " << confign << endl;

	/* Check our ignore patterns */
	ignores = this->mm.get_extensions<ignore_extension>();
	invign  = confign[0] == '-';
	if (invign) confign = confign.substr(1);
	for (size_t i=0 ; i < confign.size() && i != string::npos && !ignored; i=confign.substr(i).find(',')) {
		while (i < confign.size() && confign[i] == ',') i++;

		for (vector<ignore_extension*>::iterator it=ignores.begin() ; it != ignores.end() && !ignored ; it++)
			if ((*it)->ignore(*realurl, confign.substr(i, confign.find(','))))
				ignored = true;
	}
	if (!ignored && invign) goto do_return;
	if (ignored && !invign) goto do_return;

	/* If we have a wpad config */
	if (debug) cout << "Config is: " << confurl.to_string() << endl;
	if (confurl.get_scheme() == "wpad") {
		/* If the config has just changed from PAC to WPAD, clear the PAC */
		if (!this->wpad) {
			if (this->pac)    delete this->pac;
			if (this->pacurl) delete this->pacurl;
			this->pac    = NULL;
			this->pacurl = NULL;
			this->wpad = true;
		}

		/* If we have no PAC, get one */
		if (!this->pac) {
			if (debug) cout << "Trying to find the PAC using WPAD..." << endl;
			vector<wpad_extension*> wpads = this->mm.get_extensions<wpad_extension>();
			for (vector<wpad_extension*>::iterator i=wpads.begin() ; i != wpads.end() ; i++) {
				if (debug) cout << "WPAD search via: " << typeid(**i).name() << endl;
				if ((this->pacurl = (*i)->next(&this->pac))) {
					if (debug) cout << "PAC found!" << endl;
					break;
				}
			}

			/* If getting the PAC fails, but the WPAD cycle worked, restart the cycle */
			if (!this->pac) {
				if (debug) cout << "No PAC found..." << endl;
				for (vector<wpad_extension*>::iterator i=wpads.begin() ; i != wpads.end() ; i++) {
					if ((*i)->found()) {
						if (debug) cout << "Resetting WPAD search..." << endl;

						/* Since a PAC was found at some point,
						 * rewind all the WPADS to start from the beginning */
						/* Yes, the reuse of 'i' here is intentional...
						 * This is because if *any* of the wpads have found a pac,
						 * we need to reset them all to keep consistent state. */
						for (i=wpads.begin() ; i != wpads.end() ; i++)
							(*i)->rewind();

						// Attempt to find a PAC
						for (i=wpads.begin() ; i != wpads.end() ; i++) {
							if (debug) cout << "WPAD search via: " << typeid(**i).name() << endl;
							if ((this->pacurl = (*i)->next(&this->pac))) {
								if (debug) cout << "PAC found!" << endl;
								break;
							}
						}
						break;
					}
				}
			}
		}
	}

	// If we have a PAC config
	else if (confurl.get_scheme().substr(0, 4) == "pac+") {
		/* Save the PAC config */
		if (this->wpad)
			this->wpad = false;

		/* If a PAC already exists, but came from a different URL than the one specified, remove it */
		if (this->pac) {
			if (this->pacurl->to_string() != confurl.to_string()) {
				delete this->pacurl;
				delete this->pac;
				this->pacurl = NULL;
				this->pac    = NULL;
			}
		}

		/* Try to load the PAC if it is not already loaded */
		if (!this->pac) {
			this->pacurl = new url(confurl);
			this->pac    = confurl.get_pac();
			if (!this->pac)
				goto do_return;
		}
	}

	/* In case of either PAC or WPAD, we'll run the PAC */
	if (this->pac && (confurl.get_scheme() == "wpad" || confurl.get_scheme().substr(0, 4) == "pac+") ) {
		vector<pacrunner_extension*> pacrunners = this->mm.get_extensions<pacrunner_extension>();

		/* No PAC runner found, fall back to direct */
		if (pacrunners.size() == 0)
			goto do_return;

		/* Run the PAC, but only try one PACRunner */
		if (debug) cout << "Using pacrunner: " << typeid(*pacrunners[0]).name() << endl;
		response = _format_pac_response(pacrunners[0]->get(this->pac, this->pacurl->to_string())->run(*realurl));
	}

	/* If we have a manual config (http://..., socks://...) */
	else if (confurl.get_scheme() == "http" || confurl.get_scheme() == "socks")
	{
		this->wpad = false;
		if (this->pac)    { delete this->pac;    this->pac = NULL; }
		if (this->pacurl) { delete this->pacurl; this->pacurl = NULL; }
		response.clear();
		response.push_back(confurl.to_string());
	}

	/* Actually return, freeing misc stuff */
	do_return:
#ifdef WIN32
		ReleaseMutex(this->mutex);
#else
		pthread_mutex_unlock(&this->mutex);
#endif
		if (realurl)
			delete realurl;
		if (response.size() == 0)
			response.push_back("direct://");
		return response;
}

}

struct _pxProxyFactory {
	libproxy::proxy_factory pf;
};

extern "C" DLL_PUBLIC struct _pxProxyFactory *px_proxy_factory_new(void) {
	return new struct _pxProxyFactory;
}

extern "C" DLL_PUBLIC char** px_proxy_factory_get_proxies(struct _pxProxyFactory *self, const char *url) {
	std::vector<std::string> proxies;
	char** retval;

	// Call the main method
	//try {
		proxies = self->pf.get_proxies(url);
		retval  = new char*[proxies.size()+1];
	//} catch (exception&) {
		// Return NULL on any exception
	//	return NULL;
	//}

	// Copy the results into an array
	// Return NULL on memory allocation failure
	retval[proxies.size()] = NULL;
	for (size_t i=0 ; i < proxies.size() ; i++) {
		retval[i] = strdup(proxies[i].c_str());
		if (retval[i] == NULL) {
			for (int j=i-1 ; j >= 0 ; j--)
				free(retval[j]);
			delete retval;
			return NULL;
		}
	}
	return retval;
}

extern "C" DLL_PUBLIC void px_proxy_factory_free(struct _pxProxyFactory *self) {
	delete self;
}
