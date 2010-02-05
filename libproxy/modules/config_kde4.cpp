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

#include "kconfig.h"
#include "kconfiggroup.h"
#include "xhasclient.cpp" // For xhasclient(...)

#include "../extension_config.hpp"
using namespace libproxy;

class kde_config_extension : public config_extension {
public:
	url get_config(url dst) throw (runtime_error) {
		/* The constructor of KConfig uses qAppName() which asumes a QApplication object to exist.
		If not, an error message is written. This error message and all others seems to be disabled for
		libraries, but to be sure, we can reemplace temporaly Qt's internal message handler by a
		dummy implementation. */
		// QtMsgHandler oldHandler = qInstallMsgHandler(dummyMessageHandler);  // supress Qt messages

		string cfg;
		int ptype;

		// Open the config file
		KConfig kde_cf("kioslaverc", KConfig::NoGlobals);  // like in kprotocolmanager.cpp
		KConfigGroup kde_settings(&kde_cf, "Proxy Settings");  // accessing respective group

		// Read the config file to find out what type of proxy to use
		ptype = kde_settings.readEntry("ProxyType", 0);

		QByteArray ba;
		QString manual_proxy;
		switch (ptype) {
			case 1: // Use a manual proxy
				manual_proxy = kde_settings.readEntry(QString(dst.get_scheme().c_str()) + "Proxy", "");
				if (manual_proxy.isEmpty()) {
					manual_proxy = kde_settings.readEntry("httpProxy", "");
					if (manual_proxy.isEmpty()) {
						manual_proxy = kde_settings.readEntry("socksProxy", "");
						if (manual_proxy.isEmpty()) {
							manual_proxy = "direct://";
						};
					};
				};
				// The result of toLatin1() is undefined for non-Latin1 strings.
				// However, KDE saves this entry using IDN and percent-encoding, so no problem...
				ba = manual_proxy.toLatin1();
				cfg = string(ba.data());
				break;
			case 2: // Use a manual PAC
				// The result of toLatin1() is undefined for non-Latin1 strings.
				// However, KDE saves this entry using IDN and percent-encoding, so no problem...
				ba = kde_settings.readEntry("Proxy Config Script", "").toLatin1();
				cfg = string(ba.data());
				if (url::is_valid("pac+" + cfg))
					cfg = "pac+" + cfg;
				else
					cfg = "wpad://";
				break;
			case 3: // Use WPAD
				cfg = "wpad://";
				break;
			case 4: // Use envvar
				throw runtime_error("User config_envvar"); // We'll bypass this config plugin and let the envvar plugin work
				break;
			default:
				cfg = "direct://";
				break;
		};

		// qInstallMsgHandler(oldHandler);  // restore old behaviour
		return libproxy::url(cfg);
	}

	string get_ignore(url /*dst*/) {
		// TODO: support ReversedException

		/* The constructor of KConfig uses qAppName() which asumes a QApplication object to exist.
		If not, an error message is written. This error message and all others seems to be disabled for
		libraries, but to be sure, we can reemplace temporaly Qt's internal message handler by a
		dummy implementation. */
		// QtMsgHandler oldHandler = qInstallMsgHandler(dummyMessageHandler);  // supress Qt messages

		string returnValue;
		// Open the config file
		KConfig kde_cf("kioslaverc", KConfig::NoGlobals);  // like in kprotocolmanager.cpp
		KConfigGroup kde_settings(&kde_cf, "Proxy Settings");  // accessing respective group

		if (kde_settings.readEntry("ProxyType", 0) == 1)  { // apply ignore list only for manual proxy configuration
			QStringList list = kde_settings.readEntry("NoProxyFor", QStringList());
			for (int i = 0; i < list.size(); ++i) {
				list[i] = QUrl(list.at(i)).toEncoded();
			};
			returnValue = string(list.join(",").toLatin1().data());
		} else {
			returnValue = "";
		};

		// qInstallMsgHandler(oldHandler);  // restore old behaviour
		return returnValue;
	}
/*
private:
	void dummyMessageHandler(QtMsgType, const char *) {
	}
*/
};

MM_MODULE_EZ(kde_config_extension, xhasclient("kicker", NULL), NULL);
