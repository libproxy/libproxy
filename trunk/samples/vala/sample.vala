using Libproxy;

void main () {
	var pf = new ProxyFactory ();
	string[] proxies = pf.get_proxies ("http://www.googe.com");
	foreach (string proxy in proxies) {
		stdout.printf ("%s\n", proxy);
	}
}
