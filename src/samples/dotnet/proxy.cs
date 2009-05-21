using System;
using libproxy;

public class proxy {
    public static int Main (string[] args)
    {
        if (args.Length > 0) {
            ProxyFactory pf = new ProxyFactory();
            string[] Proxies = pf.GetProxies(args[0]);
            Console.WriteLine(Proxies[0]);
            pf = null;
            return 0;
        } else {
            Console.WriteLine("Please start the program with one parameter");
            Console.WriteLine("Example: proxy.exe http://libproxy.googlecode.com/");
            return 1;
        }
    }
}
