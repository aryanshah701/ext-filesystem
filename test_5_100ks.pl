#!/usr/bin/perl
use 5.16.0;
use warnings FATAL => 'all';

use Test::Simple tests => 28;
use IO::Handle;

sub mount {
    system("(make mount 2>&1) >> test.log &");
    sleep 1;
}

sub unmount {
    system("(make unmount 2>&1) >> test.log");
}

sub write_text {
    my ($name, $data) = @_;
    open my $fh, ">", "mnt/$name" or return;
    $fh->say($data);
    close $fh;
}

sub read_text {
    my ($name) = @_;
    open my $fh, "<", "mnt/$name" or return "";
    local $/ = undef;
    my $data = <$fh> || "";
    close $fh;
    $data =~ s/\s*$//;
    return $data;
}

sub read_text_slice {
    my ($name, $count, $offset) = @_;
    open my $fh, "<", "mnt/$name" or return "";
    my $data;
    seek $fh, $offset, 0;
    read $fh, $data, $count;
    close $fh;
    return $data;
}

system("rm -f data.nufs test.log");

say "#           == 100 1k files ==";


mount();

my $right = "ng is four";


# first file
my $huge0 = "=This string is fourty characters long.=" x 2500;
write_text("100k.txt", $huge0);
my $huge1 = read_text("100k.txt");
ok($huge0 eq $huge1, "Read back 100k correctly.");

my $huge2 = read_text_slice("100k.txt", 10, 8050);
$right = "ng is four";
ok($huge2 eq $right, "Read with offset & length");

# second file
my $huge3 = "=This string is fourty characters long.=" x 2500;
write_text("100k2.txt", $huge3);
my $huge4 = read_text("100k2.txt");
ok($huge3 eq $huge4, "Read back 100k2 correctly.");

my $huge5 = read_text_slice("100k2.txt", 10, 8050);
$right = "ng is four";
ok($huge5 eq $right, "Read with offset & length");

# third file
my $huge6 = "=This string is fourty characters long.=" x 2500;
write_text("100k3.txt", $huge6);
my $huge7 = read_text("100k3.txt");
ok($huge6 eq $huge7, "Read back 100k3 correctly.");

my $huge8 = read_text_slice("100k3.txt", 10, 8050);
$right = "ng is four";
ok($huge8 eq $right, "Read with offset & length");


# fourth file
my $huge9 = "=This string is fourty characters long.=" x 2500;
write_text("100k4.txt", $huge9);
my $huge10 = read_text("100k4.txt");
ok($huge9 eq $huge10, "Read back 100k4 correctly.");

my $huge11 = read_text_slice("100k4.txt", 10, 8050);
$right = "ng is four";
ok($huge11 eq $right, "Read with offset & length");

# fifth file
my $huge12 = "=This string is fourty characters long.=" x 2500;
write_text("100k4.txt", $huge12);
my $huge13 = read_text("100k4.txt");
ok($huge12 eq $huge13, "Read back 100k4 correctly.");

my $huge14 = read_text_slice("100k4.txt", 10, 8050);
$right = "ng is four";
ok($huge14 eq $right, "Read with offset & length");


unmount();

ok(!-d "mnt/numbers", "numbers dir doesn't exist after umount");

