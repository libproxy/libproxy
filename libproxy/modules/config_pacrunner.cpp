/*******************************************************************************
 * libproxy - A library for proxy configuration
 * Copyright (C) 2010 Intel Corporation
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

#include "../extension_config.hpp"
using namespace libproxy;

#include <string.h>
#include <dbus/dbus.h>

class pacrunner_config_extension : public config_extension {
public:
	pacrunner_config_extension() {
		this->conn = NULL;
	}

	~pacrunner_config_extension() {
		if (this->conn) dbus_connection_close(this->conn);
	}

	class scoped_dbus_message {
	public:
		scoped_dbus_message(DBusMessage *msg) {
			this->msg = msg;
		}

		~scoped_dbus_message() {
			if (this->msg)
				dbus_message_unref(msg);
		}

	private:
		DBusMessage *msg;
	};

	vector<url> get_config(const url &dest) {
		// Make sure we have a valid connection with a proper match
		DBusConnection *conn = this->conn;
		vector<url> response;

		if (!conn || !dbus_connection_get_is_connected(conn))
		{
			// If the connection was disconnected,
			// close it an clear the queue
			if (conn)
			{
				dbus_connection_close(conn);
				dbus_connection_read_write(conn, 0);
				for (DBusMessage *msg=NULL ; (msg = dbus_connection_pop_message(conn)) ; dbus_message_unref(msg)) {};
			}

			// Create a new connections
			conn = dbus_bus_get_private(DBUS_BUS_SYSTEM, NULL);
			this->conn = conn;
			if (!conn)
				throw runtime_error("Unable to set up DBus connection");

			// If connection was successful, set it up
			dbus_connection_set_exit_on_disconnect(conn, false);
		}

		DBusMessage *msg, *reply;

		msg = dbus_message_new_method_call("org.pacrunner",
						   "/org/pacrunner/client",
						   "org.pacrunner.Client",
						   "FindProxyForURL");
		if (!msg)
			throw runtime_error("Unable to create PacRunner DBus call");

		string dest_str = dest.to_string();
		string dest_host = dest.get_host();
		const char *dest_cstr = dest_str.c_str();
		const char *dest_host_cstr = dest_host.c_str();

		dbus_message_append_args(msg, DBUS_TYPE_STRING, &dest_cstr,
					 DBUS_TYPE_STRING, &dest_host_cstr,
					 DBUS_TYPE_INVALID);

		reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, NULL);

		dbus_message_unref(msg);

		if (!reply)
			throw runtime_error("Failed to get DBus response from PacRunner");

		scoped_dbus_message smsg(reply);
		char *str = NULL;
		dbus_message_get_args(reply, NULL, DBUS_TYPE_STRING, &str, DBUS_TYPE_INVALID);

		if (!str || !strlen(str) || !strcasecmp(str, "DIRECT"))
			response.push_back(url("direct://"));
		else if (!strncasecmp(str, "PROXY ", 6))
			response.push_back(url("http://" + string(str + 6)));
		else if (!strncasecmp(str, "SOCKS ", 6))
			response.push_back(url("socks://" + string(str + 6)));
		else if (!strncasecmp(str, "SOCKS4 ", 7))
			response.push_back(url("socks4://" + string(str + 7)));
		else if (!strncasecmp(str, "SOCKS5 ", 7))
			response.push_back(url("socks5://" + string(str + 7)));
		else {
			throw runtime_error("Unrecognised proxy response from PacRunner: " + string(str));
		}
		return response;
	}

private:
	DBusConnection *conn;
};

#define TEST_TIMEOUT_MS 100

static bool is_pacrunner_available(void)
{
	DBusMessage *msg, *reply;
	DBusConnection* system;
	dbus_bool_t owned;
	bool found = false;
	const char *name = "org.pacrunner";

	msg = dbus_message_new_method_call("org.freedesktop.DBus",
					   "/org/freedesktop/DBus",
					   "org.freedesktop.DBus",
					   "NameHasOwner");
	if (!msg)
		return false;

	dbus_message_append_args(msg, DBUS_TYPE_STRING, &name,
				 DBUS_TYPE_INVALID);

	system = dbus_bus_get_private(DBUS_BUS_SYSTEM, NULL);
	if (!system)
		goto out_msg;

	reply = dbus_connection_send_with_reply_and_block(system, msg,
							  TEST_TIMEOUT_MS,
							  NULL);
	if (!reply)
		goto out_sys;

	if (dbus_message_get_args(reply, NULL, DBUS_TYPE_BOOLEAN, &owned,
				  DBUS_TYPE_INVALID))
		found = owned;

out_reply:
	dbus_message_unref(reply);
out_sys:
	dbus_connection_close(system);
	dbus_connection_unref(system);
out_msg:
	dbus_message_unref(msg);

	return found;
}

MM_MODULE_INIT_EZ(pacrunner_config_extension, is_pacrunner_available(),
		  NULL, NULL);
