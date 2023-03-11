function FindProxyForURL(url, host)
{
  var myIP = myIpAddress();

  if(shExpMatch(host,"192.168.10.4"))
    return "SOCKS 127.0.0.1:1983"

  if(shExpMatch(host,"192.168.10.5"))
    return "SOCKS4 127.0.0.1:1983"

  if(shExpMatch(host,"192.168.10.6"))
    return "SOCKS4A 127.0.0.1:1983"

  if(shExpMatch(host,"192.168.10.7"))
    return "SOCKS5 127.0.0.1:1983"

  return "PROXY 127.0.0.1:1983"
}
