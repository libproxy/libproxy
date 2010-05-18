using Libproxy;

void main () {
	var proxy_factory = new ProxyFactory ();
	string[] proxies = proxy_factory.get_proxies ("");
	foreach (string proxy in proxies) {
		stdout.printf ("%s\n", proxy);
	}
}
