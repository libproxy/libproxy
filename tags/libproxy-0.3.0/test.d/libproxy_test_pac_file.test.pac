function FindProxyForURL(url, host)
{
  var myIP = myIpAddress();
  if (host == "code.google.com")
    return "PROXY 1.2.3.4:1234";
  else
    return "DIRECT";
}
