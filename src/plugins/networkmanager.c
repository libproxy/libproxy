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

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <misc.h>
#include <proxy_factory.h>

#include <dbus/dbus.h>
#include <NetworkManager/NetworkManager.h>

void
nm_on_get_proxy(pxProxyFactory *self)
{
	// Make sure we have a valid connection with a proper match
	DBusConnection *conn = px_proxy_factory_misc_get(self, "networkmanager");
	if (!conn || !dbus_connection_get_is_connected(conn))
	{
		// If the connection was disconnected, 
		// close it an clear the queue
		if (conn)
		{
			dbus_connection_close(conn);
			dbus_connection_read_write(conn, 0);
			for (DBusMessage *msg=NULL ; (msg = dbus_connection_pop_message(conn)) ; dbus_message_unref(msg));
		}
		
		// Create a new connections
		conn = dbus_bus_get_private(DBUS_BUS_SYSTEM, NULL);
		if (!conn) return;
		
		// If connection was successful, set it up
		dbus_connection_set_exit_on_disconnect(conn, false);
		dbus_bus_add_match(conn, "type='signal',interface='" NM_DBUS_INTERFACE "',member='StateChange'", NULL);
		dbus_connection_flush(conn);
		px_proxy_factory_misc_set(self, "networkmanager", conn);
	}
	
	// We are guaranteed a connection,
	// so check for incoming messages
	bool changed = false;
	while (true)
	{
		DBusMessage *msg = NULL;
		uint32_t state;
		
		// Pull messages off the queue
		dbus_connection_read_write(conn, 0);
		if (!(msg = dbus_connection_pop_message(conn)))
			break;
		
		// If a message is the right type and value,
		// we'll reset the network
		if (dbus_message_get_args(msg, NULL, DBUS_TYPE_UINT32, &state, DBUS_TYPE_INVALID))
			if (state == NM_STATE_CONNECTED)
				changed = true;
		
		dbus_message_unref(msg);
	}
	
	// Reset the network if ready
	if (changed) 
		px_proxy_factory_network_changed(self);
}

bool
on_proxy_factory_instantiate(pxProxyFactory *self)
{
	return px_proxy_factory_on_get_proxies_add(self, nm_on_get_proxy);
}

void
on_proxy_factory_destantiate(pxProxyFactory *self)
{
	px_proxy_factory_on_get_proxies_del(self, nm_on_get_proxy);
	DBusConnection *conn = px_proxy_factory_misc_get(self, "networkmanager");
	if (conn)
	{
		dbus_connection_close(conn);
		px_proxy_factory_misc_set(self, "networkmanager", NULL);
	}
}
