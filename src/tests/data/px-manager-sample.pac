function FindProxyForURL(url, host)
{
  var myIP = myIpAddress();

  return "PROXY 127.0.0.1:1983"
}
