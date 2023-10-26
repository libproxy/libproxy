function FindProxyForURL(url, host)
{
  var resolved_ip = dnsResolve(host);
  var myIP = myIpAddress();
  var privateIP = /^(0|10|127|192\.168|172\.1[6789]|172\.2[0-9]|172\.3[01]|169\.254|192\.88\.99)\.[0-9.]+$/;

  if (shExpMatch(host, "192.168.10.4"))
    return "SOCKS 127.0.0.1:1983"

  if (dnsDomainIs(host, "192.168.10.5"))
    return "SOCKS4 127.0.0.1:1983"

  if (dnsDomainIs(host, "192.168.10.6"))
    return "SOCKS4A 127.0.0.1:1983"

  if (dnsDomainIs(host, "192.168.10.7"))
    return "SOCKS5 127.0.0.1:1983"

  /* Invalid return */
  if (dnsDomainIs(host, "192.168.10.8"))
    return "PROXY %%"

  /* Invalid return */
  if (dnsDomainIs(host, "192.168.10.9"))
    return "INVALID"

  alert("DEFAULT")

  /* Don't send non-FQDN or private IP auths to us */
  if (isPlainHostName(host) || isInNet(resolved_ip, "192.0.2.0","255.255.255.0") || privateIP.test(resolved_ip))
    return "DIRECT";

  if (dnsDomainIs(host, "www.example.com"))
    return "PROXY 127.0.0.1:1984"

  /* Return nothing to check wrong output */
}
