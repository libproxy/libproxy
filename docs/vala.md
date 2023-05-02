Title: How to use libproxy in Vala
Slug: snippets

# How to use libproxy in Vala


## Makefile

```
all: sample

sample: sample.vala
	valac --pkg libproxy-1.0 sample.vala

clean:
	rm sample
```

## Source

```
using px;

void main() {
  var pf = new px.ProxyFactory();
  string[] proxies = pf.get_proxies("https://github.com/libproxy/libproxy");

  foreach (string proxy in proxies) {
    stdout.printf ("%s\n", proxy);
  }
}
```
