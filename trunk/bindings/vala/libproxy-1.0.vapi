[CCode (cprefix = "px", cheader_filename = "proxy.h")]
namespace Px {
	[Compact]
	[CCode (free_function = "px_proxy_factory_free")]
	public class ProxyFactory {
		public ProxyFactory ();
		[CCode (array_length = false, array_null_terminated = true)]
		public string[] get_proxies (string url);
	}
}
