function FindProxyForURL(url, host)
{
  /* Here is an intentional error to test the pac runners.
     The correct call would be myIpAddress... DO NOT FIX */
  myIP=myNetworkAddress();
  return "PROXY " + myIP + ":80";
  
}
