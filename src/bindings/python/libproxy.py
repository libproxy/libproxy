###############################################################################
# libproxy - A library for proxy configuration
# Copyright (C) 2006 Nathaniel McCallum <nathaniel@natemccallum.com>
# 
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
###############################################################################

"A library for proxy configuration and autodetection."

import ctypes
import ctypes.util

import sys

# Load libpython (for a cross platform 'free()')
_names = ("python%d.%d" % sys.version_info[:2], "python%d%d" % sys.version_info[:2])
_libpython = None
for _name in _names:
    if ctypes.util.find_library(_name):
        _libpython = ctypes.cdll.LoadLibrary(ctypes.util.find_library(_name))
if not _libpython:
    raise ImportError, "Unable to import libpython!?!?"

# Load libproxy
if not ctypes.util.find_library("proxy"):
    raise ImportError, "Unable to import libproxy!?!?"
_libproxy = ctypes.cdll.LoadLibrary(ctypes.util.find_library("proxy"))
_libproxy.px_proxy_factory_get_proxy.restype = ctypes.POINTER(ctypes.c_void_p)

class ProxyFactory(object):
    """A ProxyFactory object is used to provide potential proxies to use
    in order to reach a given URL (via 'getProxy(url)').
    
    ProxyFactory is NOT thread safe!
    
    Usage is pretty simple:
        pf = libproxy.ProxyFactory()
        for url in urls:
            proxies = pf.getProxy(url)
            for proxy in proxies:
                if proxy == "direct://":
                    # Fetch URL without using a proxy
                elif proxy.startswith("http://"):
                    # Fetch URL using an HTTP proxy
                elif proxy.startswith("socks://"):
                    # Fetch URL using a SOCKS proxy
                
                if fetchSucceeded:
                    break    
    """

    def __init__(self):
        self._pf = _libproxy.px_proxy_factory_new()
        
    def getProxy(self, url):
        """Given a URL, returns a list of proxies in priority order to be used
        to reach that URL."""
        if type(url) != str:
            raise TypeError, "url must be a string!"
        
        proxies = []
        array = _libproxy.px_proxy_factory_get_proxy(self._pf, url)
        i=0
        while array[i]:
            proxies.append(str(ctypes.cast(array[i], ctypes.c_char_p).value))
            _libpython.PyMem_Free(array[i])
            i += 1
        _libpython.PyMem_Free(array)
        
        return proxies
        
    def __del__(self):
        _libproxy.px_proxy_factory_free(self._pf)
    
