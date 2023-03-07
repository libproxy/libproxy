Title: How to use libproxy in Ruby
Slug: snippets

# How to use libproxy in Ruby

```
#!/usr/bin/ruby

require 'gir_ffi'
GirFFI.setup :Libproxy

pf = Libproxy::ProxyFactory.new()

proxies = pf.get_proxies("https://github.com/libproxy/libproxy")
proxies.each do |proxy|
  puts proxy
end

pf.free()
```
