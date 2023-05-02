Title: Configuration Logic
Slug: Design

# Configuration Logic

## Introduction
As the proxy configuration predates libproxy, we need to consider previous
implementation behavior to ensure consistency with user expectations. This page
presents analyses of well known implementation base on the platform they run on.

## Linux
On Linux the pioneer of proxy support is Mozilla browsers (former Netscape).
But other browsers do support proxy and has it's own proxy configuration
interpretation logic. Current GNOME proxy settings is a copy of Firefox
settings.

### Firefox
When using Firefox internal manual settings, the proxy is selected base on the
most specific proxy (e.g. HTTP before SOCKS). Only one proxy is selected, if
connection to that proxy fails, then connection fails.

```
 IF protocol specific proxy is set THEN use it
 ELSE IF SOCKS proxy is set THEN use it
 ELSE use direct connection.
```

### Firefox and Chromium (with GNOME settings)
After some testing we found that Chromium mimics perfectly Firefox behavior
when using system settings. When using manual proxy configuration mode, those
browsers chooses a proxy base on the most generic solution (SOCKS) to the most
specific (per protocol proxies), with an exception when a single proxy is set
for all protocols. Only one protocol is selected and no fallback will occur in
the case of failures. Those browsers support SOCKS, HTTP, HTTPS, FTP and
Firefox also support Gopher. You can also set the configuration to use a
specific PAC file or to automatically discover one (WPAD) but those does not
contain any special logic. Next is the logic represent as pseudo code:

```
 IF not using same proxy for all protocols THEN
   IF SOCKS is set THEN use it
   ELSE IF protocol specific proxy is set THEN use it
   ELSE IF using same proxy for all protocols THEN
    IF SOCKS is set THEN use it
 IF no proxy has been set THEN use direct connection
```

## OS X
OS X uses it's own way for proxy settings. It supports protocols including
SOCKS, HTTP, HTTPS, FTP, Gopher, RTSP, and automatic configuration through PAC
files. For sake of simplicity, we have tested the logic with the default browser
Safari.

### Safari
Safari interpret proxy logic differently from Firefox. If multiple proxy are
configured, it try each of them until a connection is established. From our
testing the order seems to be from most specific to most generic (starting with
manual configuration). Next is the logic represented as pseudo code:

```
 DEFINE proxy_list as list
 IF protocol specific proxy is set THEN add it to proxy_list
 IF SOCKS proxy is set THEN append it to proxy_list
 IF PAC auto-configuration is set THEN append it to proxy_list
 FOREACH proxy in proxy_list
   connect to proxy
   IF connection failed THEN continue
   ELSE stop
```

## Windows
Windows user most often use Internet Explorer, Firefox or Opera for browsing.
Analyses as shown that Firefox acts exactly the same as on Linux, except that
same logic is applied for internal settings and system setting (IE settings).
Internet explorer also act the same way, and Opera only support protocol
specific proxies (no SOCKS). So essentially, if chooses choose the most
specific proxy, and if that one fails the own connection fails.

## Conclusion
Base on current result, we see that the most common logic is to select a proxy
starting from the most specific (HTTP, FTP, etc.) to the least specific
(SOCKS, PAC then WPAD). OS X pushes a bit further by trying all the configure
proxy that match the protocol. This technique is interesting for libproxy
since it warranty that connection will be possible for all cases covered by
the others. The only difference with the GNOME environment is that OS X may
connect through an HTTP server while Firefox and Chromium (on Gnome) would
connect to a SOCKS server if both are available.
