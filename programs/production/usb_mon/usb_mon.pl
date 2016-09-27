#!/usr/bin/perl
#
# Monitor the USB system and report anything added or removed
#
use strict;
use warnings;

my $out_file = "/var/log/usb_mon";
#
# get_usb -- Return a list of the usb devices
#
sub get_usb()
{
    my %result = ();	# Result we are returning

    my @list = `lsusb`;	# List the usb devices
	foreach my $i (@list) {
	    chomp($i);
	    $result{$i} = 1;
	}
    return (%result);
}


my %old_usb = ();       # Old USB status

#%old_usb = get_usb();
{
    my $now = localtime();
    open OUT_FILE, ">>$out_file";
    print OUT_FILE "$now START\n";
    close (OUT_FILE);
}

while (1) {
    my %new_usb = get_usb();	# Usb after transfer

    foreach my $old (keys %old_usb) {
	if (defined($new_usb{$old})) {
	    $new_usb{$old} = 'B';
	} else {
	    $new_usb{$old} = 'X';
	}
    }
    # Get time for printing
    my $now = localtime();
    open OUT_FILE, ">>$out_file";
    foreach my $new (keys %new_usb) {
	if ($new_usb{$new} eq '1') {
	    print OUT_FILE "$now ADD $new\n";
	} elsif ($new_usb{$new} eq 'X') {
	    print OUT_FILE "$now REM $new\n";
	}
    }
    close (OUT_FILE);
    %old_usb = get_usb();
    sleep(10);
}


