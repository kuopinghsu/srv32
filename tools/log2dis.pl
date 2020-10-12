#!/usr/bin/perl -w
use strict;

my $objdump = "riscv64-unknown-elf-objdump";
my %DIS;

if ($#ARGV != 1) {
    print "Usage: log2dis.pl trace.log file.elf\n";
    exit -1;
}

open(FH, "$objdump -d $ARGV[1]|") || die "can not open file $ARGV[1]";

while(<FH>) {
    if (/^\s+([0-9a-fA-F]+):\s+([0-9a-fA-F]+)\s+(.+)$/) {
        my $addr = sprintf "%08x", hex($1);
        $DIS{$addr} = $3;
    }
}

close (FH);

open(FH, "< $ARGV[0]") || die "can not open file $ARGV[0]";
open(FO, "> $ARGV[0].dis") || die "can not open file $ARGV[0].dis";

while(<FH>) {
    chomp;
    if (/^\s+(\d+)\s+([0-9a-fA-F]+)/) {
        my $addr = sprintf "%08x", hex($2);
        my $len = length($_);
        my $space = " ";
        if ($len < 60) {
            $space = " "x(60-$len);
        }
        if (exists($DIS{$addr})) {
            printf FO "$_%s; %s\n", $space, $DIS{$addr};
        } else {
            printf FO "$_\n";
        }
    }
}

close(FH);
close(FO);

exit(0);


