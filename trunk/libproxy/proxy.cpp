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
#include <algorithm>
#include <iostream>
#include <cstring>

#include "config_file.hpp"
#include "module_config.hpp"
#include "module_ignore.hpp"
#include "module_network.hpp"
#include "module_pacrunner.hpp"
#include "module_wpad.hpp"

#ifdef WIN32
#define setenv(name, value, overwrite) SetEnvironmentVariable(name, value)
#define strdup _strdup
#endif

namespace com {
namespace googlecode {
namespace libproxy {
using namespace std;

static const char* _builtin_modules[] = {
                BUILTIN_MODULES
                NULL
};

class proxy_factory {
public:
	proxy_factory();
	~proxy_factory();
	vector<string> get_proxies(string url);

private:
#ifndef WIN32
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
#ifndef WIN32
	pthread_mutex_init(&this->mutex, NULL);
	pthread_mutex_lock(&this->mutex);
#endif
	this->pac    = NULL;
	this->pacurl = NULL;

	// Register our singletons
	this->mm.set_singleton<pacrunner_module>(true);

	// If we have a config file, load the config order from it
	setenv("_PX_CONFIG_ORDER", "", 1);
   	config_file cf;
	if (cf.load(SYSCONFDIR "proxy.conf")) {
		string tmp;
		if (cf.get_value("config_order", tmp))
			setenv("_PX_CONFIG_ORDER", tmp.c_str(), 1);
	}

	// Load all builtin modules
	for (int i=0 ; _builtin_modules[i] ; i++)
		this->mm.load_builtin(_builtin_modules[i]);

	// Load all modules
	this->mm.load_dir(MODULEDIR);

#ifndef WIN32
	pthread_mutex_unlock(&this->mutex);
#endif
}

proxy_factory::~proxy_factory() {
#ifndef WIN32
	pthread_mutex_lock(&this->mutex);
#endif
	if (this->pac) delete this->pac;
	if (this->pacurl) delete this->pacurl;
#ifndef WIN32
	pthread_mutex_unlock(&this->mutex);
	pthread_mutex_destroy(&this->mutex);
#endif
}


vector<string> proxy_factory::get_proxies(string __url) {
	url*                    realurl = NULL;
	url                     confurl("direct://");
	string                  confign;
	config_module*          config;
	vector<network_module*> networks;
	vector<config_module*>  configs;
	vector<ignore_module*>  ignores;
	vector<string>          response;

	// Check to make sure our url is valid
	if (!url::is_valid(__url))
		goto do_return;
	realurl = new url(__url);

#ifndef WIN32
	// Lock the mutex
	pthread_mutex_lock(&this->mutex);
#endif

	// Check to see if our network topology has changed...
	networks = this->mm.get_modules<network_module>();
	for (vector<network_module*>::iterator i=networks.begin() ; i != networks.end() ; i++) {
		// If it has, reset our wpad/pac setup and we'll retry our config
		if ((*i)->changed()) {
			vector<wpad_module*> wpads = this->mm.get_modules<wpad_module>();
			for (vector<wpad_module*>::iterator j=wpads.begin() ; j != wpads.end() ; j++)
				(*j)->rewind();
			if (this->pac) delete this->pac;
			this->pac = NULL;
			break;
		}
	}

	configs = this->mm.get_modules<config_module>();
	for (vector<config_module*>::iterator i=configs.begin() ; i != configs.end() ; i++) {
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
		break;

		invalid_config:
			confurl = "direct://";
			confign = "";
			(config)->set_valid(false);
			config = NULL;
	}

	/* Check our ignore patterns */
	ignores = this->mm.get_modules<ignore_module>();
	for (size_t i=-1 ; i < confign.size() ; i=confign.find(',')) {
		for (vector<ignore_module*>::iterator it=ignores.begin() ; it != ignores.end() ; it++)
			if ((*it)->ignore(*realurl, confign.substr(i+1, confign.find(','))))
				goto do_return;
	}

	/* If we have a wpad config */
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
			vector<wpad_module*> wpads = this->mm.get_modules<wpad_module>();
			for (vector<wpad_module*>::iterator i=wpads.begin() ; i != wpads.end() ; i++)
				if ((this->pacurl = (*i)->next(&this->pac)))
					break;

			/* If getting the PAC fails, but the WPAD cycle worked, restart the cycle */
			if (!this->pac) {
				for (vector<wpad_module*>::iterator i=wpads.begin() ; i != wpads.end() ; i++) {
					if ((*i)->found()) {
						/* Since a PAC was found at some point,
						 * rewind all the WPADS to start from the beginning */
						/* Yes, the reuse of 'i' here is intentional...
						 * This is because if *any* of the wpads have found a pac,
						 * we need to reset them all to keep consistent state. */
						for (i=wpads.begin() ; i != wpads.end() ; i++)
							(*i)->rewind();

						// Attempt to find a PAC
						for (i=wpads.begin() ; i != wpads.end() ; i++)
							if ((this->pacurl = (*i)->next(&this->pac)))
								break;
						break;
					}
				}
			}
		}
	}
	// If we have a PAC config
	else if (confurl.get_scheme().substr(0, 4) == "pac+")
	{
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
		vector<pacrunner_module*> pacrunners = this->mm.get_modules<pacrunner_module>();

		/* No PAC runner found, fall back to direct */
		if (pacrunners.size() == 0)
			goto do_return;

		/* Run the PAC, but only try one PACRunner */
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
#ifndef WIN32
		pthread_mutex_unlock(&this->mutex);
#endif
		if (realurl)
			delete realurl;
		if (response.size() == 0)
			response.push_back("direct://");
		return response;
}

}
}
}

struct _pxProxyFactory {
	com::googlecode::libproxy::proxy_factory pf;
};

extern "C" DLL_PUBLIC struct _pxProxyFactory *px_proxy_factory_new(void) {
	return new struct _pxProxyFactory;
}

extern "C" DLL_PUBLIC char** px_proxy_factory_get_proxies(struct _pxProxyFactory *self, const char *url) {
	vector<string> proxies;
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
