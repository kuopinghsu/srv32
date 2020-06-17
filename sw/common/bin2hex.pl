#!/usr/bin/perl -w
use strict;

my $MEMINIT = 1;
my $RAMSIZE = 128*1024;

hex2bin("imem.bin", "imem.hex");
hex2bin("dmem.bin", "dmem.hex");

sub hex2bin {
  use Fcntl;
  my ($in_file, $out_file) = @_;
  my $data;

  if (! -f $in_file) {
    print "File $in_file not found\n";
    return;
  }

  open(FILE, "< $in_file") || die "Can not open $in_file";
  open(OUT, "> $out_file") || die "Can not create $out_file";
  binmode(FILE);

  my $i = 0;
  while(read(FILE, $data, 4)) {
    my $v = unpack("I", $data);
    printf OUT "%08x // 32'h%08x\n", $v, $i;
    $i+=4;
  }
  while($i < $RAMSIZE) {
    if ($MEMINIT) {
        printf OUT "00000000 // 32'h%08x\n", $i;
    } else {
        printf OUT "xxxxxxxx // 32'h%08x\n", $i;
    }
    $i+=4;
  }

  close(FILE);
  close(OUT);
}

