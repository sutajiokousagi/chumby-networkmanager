#!/usr/bin/perl
# This generates random network configs useful
# in fuzzing a start_network replacement.
use warnings;
use strict;

# Generate a random number of configs.
my $configs = rand(32)%32;

if(!$configs) {
	print "<configurations/>\n";
	exit(0);
}

print "<configurations>\n";
for(0..$configs) {
	generate_config();
}
print "</configurations>\n";



sub generate_config {
	my $config = {};
	generate_network_type($config);
	generate_allocation($config);

	if($$config{'type'} eq 'wlan') {
		generate_hwaddr($config);
		generate_ssid($config);
		generate_secure($config);
	}
	elsif($$config{'type'} eq 'lan') {
		;
	}
	else {
		die("Unknown network type: $$config{'type'}\n");
	}

	print "    <configuration";
	while( my ($key, $value) = each %$config) {
		print " $key=\"$value\"";
	}
	print "/>\n";
}


sub generate_network_type {
	my ($config) = @_;
	$$config{'type'} = 'wlan';

	# 10% chance of being lan
	if(!(rand(10)%10)) {
		$$config{'type'} = 'lan';
	}
}

sub generate_ssid {
	my ($config) = @_;
	$$config{'ssid'} = "";
	my $len = rand(31)%31;
	for(0..$len) {
		$$config{'ssid'} .= generate_char();
	}
}

sub generate_char {
	my @letters = (' ', '-', '&quot;', "'", '!', '&amp;', '&lt;', '&gt;');
	push(@letters, chr) for(ord('a')..ord('z'));
	push(@letters, chr) for(ord('A')..ord('Z'));
	push(@letters, chr) for(ord('0')..ord('9'));
	# Invalid HTML entities are uncommon
	if(!(rand(32)%32)) {
		return '&#' . (32+(rand(1024)%1024)) . ';';
	}
	else {
		return $letters[rand(@letters)];
	}
}

sub generate_hex {
	my @hexvalues = qw(0 1 2 3 4 5 6 7 8 9 a b c d e f);
	return $hexvalues[rand(16)%16];
}

sub generate_secure {
	my ($config) = @_;
	my @auths = qw(OPEN WEPAUTO WPAPSK WPA2PSK);
	$$config{'auth'} = $auths[rand($#auths)%$#auths];
	
	if($$config{'auth'} eq "OPEN") {
		$$config{'encryption'} = "NONE";
	}
	elsif($$config{'auth'} eq "WEPAUTO") {
		$$config{'encryption'} = "WEP";
		$$config{'key'} = "";
		if(!(rand(2)%2)) {
			$$config{'encoding'} = 'hex';
			for(0..9) {
				$$config{'key'} .= generate_hex();
			}
			# Generate twice as many characters for 128-bit WEP
			if(!(rand(2)%2)) {
				for(0..15) {
					$$config{'key'} .= generate_hex();
				}
			}
		}
		else {
			my $characters = (rand(25)%25);
			for(0..$characters) {
				$$config{'key'} .= generate_char();
			}
		}
	}

	elsif($$config{'auth'} eq "WPAPSK" || $$config{'auth'} eq "WPA2PSK") {
		$$config{'encryption'} = "AES";
		if(!(rand(2)%2)) {
			$$config{'encryption'} = "TKIP";
		}
		$$config{'key'} = "";

		# People using PSK is VERY unlikely
		if(!(rand(50)%50)) {
			for(0..63) {
				$$config{'key'} .= generate_hex();
			}
		}
		else {
			my $characters = rand(63)%63;
			for(8..$characters) {
				$$config{'key'} .= generate_char();
			}
		}
	}
}

# 20% chance of being static
sub generate_allocation {
	my ($config) = @_;
	$$config{'allocation'} = 'dhcp';
	if(!(rand(5)%5)) {
		$$config{'allocation'} = 'static';
		$$config{'ip'} = (rand(256)%256) . "." . (rand(256)%256) . "." . (rand(256)%256) . "." . (rand(256)%256);
		$$config{'netmask'} = (rand(256)%256) . "." . (rand(256)%256) . "." . (rand(256)%256) . "." . (rand(256)%256);
		$$config{'gateway'} = (rand(256)%256) . "." . (rand(256)%256) . "." . (rand(256)%256) . "." . (rand(256)%256);
		$$config{'nameserver1'} = (rand(256)%256) . "." . (rand(256)%256) . "." . (rand(256)%256) . "." . (rand(256)%256);
		$$config{'nameserver2'} = (rand(256)%256) . "." . (rand(256)%256) . "." . (rand(256)%256) . "." . (rand(256)%256);
	}
}

sub generate_hwaddr {
	my ($config) = @_;
	my $hwaddr = "";
	$hwaddr .= generate_hex();
	$hwaddr .= generate_hex();
	$hwaddr .= ":";
	$hwaddr .= generate_hex();
	$hwaddr .= generate_hex();
	$hwaddr .= ":";
	$hwaddr .= generate_hex();
	$hwaddr .= generate_hex();
	$hwaddr .= ":";
	$hwaddr .= generate_hex();
	$hwaddr .= generate_hex();
	$hwaddr .= ":";
	$hwaddr .= generate_hex();
	$hwaddr .= generate_hex();
	$hwaddr .= ":";
	$hwaddr .= generate_hex();
	$hwaddr .= generate_hex();
	$$config{'hwaddr'} = $hwaddr;
}
