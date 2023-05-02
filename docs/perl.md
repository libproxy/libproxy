Title: How to use libproxy in Perl
Slug: snippets

# How to use libproxy in Perl

```
#!/usr/bin/perl
use warnings;
use Glib::Object::Introspection;
Glib::Object::Introspection->setup(
  basename => 'Px',
  version => '1.0',
  package => 'Px');

my $pf = new Px::ProxyFactory;

$proxies = $pf->get_proxies("https://github.com/libproxy/libproxy");
foreach my $proxy (@$proxies) {
  print $proxy."\n";
}
```
