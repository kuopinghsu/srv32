#!/usr/bin/perl -w
use strict;

my $VERBOSE = 1;
my $CROSS_COMPILER = defined($ENV{'CROSS_COMPILER'}) ? $ENV{'CROSS_COMPILER'} : "riscv64-unknown-elf-";
my $objdump = "${CROSS_COMPILER}objdump";
my %DIS;
$| = 1;

if ($#ARGV == 2 && $ARGV[0] eq "-q") {
    $VERBOSE = 0;
    shift;
}

if ($#ARGV != 1) {
    print "Usage: log2dis.pl [-q] trace.log file.elf\n";
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

my $line = 0;
while(<FH>) {
    chomp;
    if (/^\s+(\d+)\s+([0-9a-fA-F]+)/) {
        my $addr = sprintf "%08x", hex($2);
        my $len = length($_);
        my $space = " ";
        if ($len < 74) {
            $space = " "x(74-$len);
        }
        if (exists($DIS{$addr})) {
            printf FO "$_%s; %s\n", $space, $DIS{$addr};
        } else {
            printf FO "$_\n";
        }
        $line++;
        printf "." if ($VERBOSE == 1 && ($line % 100000) == 0);
    }
}

printf "Done.\n" if ($VERBOSE == 1);

close(FH);
close(FO);

exit(0);


