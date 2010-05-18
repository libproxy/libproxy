using Libproxy;

void main () {
	var pf = new ProxyFactory ();
	string[] proxies = pf.get_proxies ("");
	foreach (string proxy in proxies) {
		stdout.printf ("%s\n", proxy);
	}
}
