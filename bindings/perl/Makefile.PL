# Makefile.PL for Perl module WWW::Curl
# Check out the README file for more information.

use inc::Module::Install;

name			      'Net-Libproxy';
abstract		    'Perl binding for Libproxy';
author			    'Gonéri Le Bouder <goneri@rulezlan.org>';
license			    'Perl';
perl_version		'5.008001';
no_index		    directory => 'template';

if ($^O ne 'MSWin32') {
	$lflags = `pkg-config libproxy-1.0 --libs`;
}

# Let Module::Install generate META.yml and other necessary files.
WriteMakefile(
    'NAME'         => 'Net::Libproxy',
    'VERSION_FROM' => 'lib/Net/Libproxy.pm',      # finds $VERSION
    'LIBS'         => "$ldflags $lflags",        # e.g., '-lm'
    'INC'          => "."
);

