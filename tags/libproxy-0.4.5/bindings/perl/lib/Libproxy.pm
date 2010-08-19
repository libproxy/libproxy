package Net::Libproxy;
use 5.008000;
use warnings;
our $VERSION = '0.04';

require Exporter;
require DynaLoader;
@ISA = qw(Exporter DynaLoader);
@EXPORT = qw(proxy_factory_new proxy_factory_get_proxies);

bootstrap Net::Libproxy;

sub new {
  my $self;

  $self->{pf} = Net::Libproxy::proxy_factory_new();

  bless $self;
}

sub getProxies {
  my ($self, $url) = @_;

  return Net::Libproxy::proxy_factory_get_proxies($self->{pf}, $url);
}

1;


=head1 NAME

Net::Libproxy - Perl binding for libproxy ( http://code.google.com/p/libproxy/  )

=head1 SYNOPSIS

  use Net::Libproxy;

  $p = new Net::Libproxy;
  $proxies = $p->getProxies('http://code.google.com/p/libproxy/');

  foreach my $proxy (@$proxies) {
    print $proxy."\n";
  }

=head1 DESCRIPTION

libproxy is a lightweight library which makes it easy to develop
applications proxy-aware with a simple and stable API.

=head2 EXPORT

These two functions are also exported.
proxy_factory_new()
proxy_factory_get_proxies()

=head1 SEE ALSO

Libproxy homepage: http://code.google.com/p/libproxy/
Net::Libproxy on Gitorious: http://gitorious.org/net-libproxy
You can also read proxy.h and Net/Libproxy.pm

=head1 AUTHOR

Goneri Le Bouder, E<lt>goneri@rulezlan.orgE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2009 by Goneri Le Bouder

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.10.0 or,
at your option, any later version of Perl 5 you may have available.


=cut
