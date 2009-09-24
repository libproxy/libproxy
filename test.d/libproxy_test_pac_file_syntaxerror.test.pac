function FindProxyForURL(url, host)
{
  myIP=myNetworkAddress();
  return "PROXY " + myIP + ":80";
  
}
