#!/usr/bin/perl

# this is an example quack-receiver that 
# - reads from serial port (or logfile)
# - should not require installing any perl modules
# - parses the lora messages
# - sends them on 
# - - to a terminal and/or
# - - to a logfile and/or
# - - to a grafana and/or
# - - to whatever you add modules for
#
# only the --infile is really required:
# 	perl duckster.pl -i /dev/ttyUSB0
#
# more complete:
# 	perl duckster.pl --infile /dev/ttyUSB0 --logfile duck.log --DEBUG 
#

use warnings;
use strict;

# optional for fractional seconds
eval { require Time::HiRes; };

my $infile;
my $baud = 115200;

my $sqlite;

my $logfile;
my $DEBUG = 0;

# process options 
use Getopt::Long;
Getopt::Long::Configure('bundling');
unless (GetOptions(
        'infile|i=s'    => \$infile,
        'logfile|l=s'   => \$logfile,
        'baud|b=s'      => \$baud,
        'sqlite|s=s'    => \$sqlite,
        'debug|D+'      => \$DEBUG,
)) {
        terminate('UNKNOWN', 'FATAL: error parsing options');
}

die "no infile" unless defined $infile && length $infile;
die "infile '$infile' doesnt exist" unless -e $infile;

# open optional database
my $dbh;
if (defined $sqlite && length $sqlite) {
	require DBI;
	$dbh = DBI->connect("dbi:SQLite:dbname=$sqlite","","");
	die "no dbh" unless defined $dbh;
	$dbh->do("CREATE TABLE IF NOT EXISTS clusterData (timestamp datetime, duck_id TEXT, message_id TEXT, payload TEXT, path TEXT)") or die $dbh->errstr;
	print "SQLITE: $sqlite up\n" if $DEBUG;
}

my $lf;
if (defined $logfile && length $logfile) {
	use IO::Handle;
	open $lf, ">>", $logfile or die "open($logfile): $!";
}

REOPEN:
if ( -c $infile && $baud ) {
	my $cmd = "stty -F $infile $baud raw -echo";
	print "PORTSETUP: '$cmd'\n" if $DEBUG;
	my $out = `$cmd`;
	print "OUTPUT: '$out'\n" if $DEBUG;
}
open IF, "<", $infile or die "open($infile): $!";
my $C=0;
RETAIL:
while (<IF>) {
	$C=0;

	# parse line
	my $h = &line_to_hash($_);
	next unless defined $h && ref $h eq "HASH";

	# this is an example payload that just dumps the hash
	use Data::Dumper;
	$Data::Dumper::Sortkeys = 1;
	print Dumper($h);

	# ADD YOUR PAYLOAD HERE

	# this payload sends the data to grafana via tcp
	#&hash_to_carb($h);

	# this payload writes data to a database
	&hash_to_db($h);
}
print STDERR "SLEEP $C\n";
sleep 10;
goto RETAIL if $C++ < 30;
&decarb();

exit 0;

####

sub line_to_hash ($) {
	my ($l,) = @_;
	$l =~ s/[\r\n]+$//;
	my $o = $l;
	my $ts = eval { Time::HiRes::time(); } || time;
	if ($l =~ s/^(\d+(\.\d+)?) //) {
		# reuse existing timestamp?
		$ts = $1;
	}
	if (defined $lf) {
		print $lf "$ts $l\n";
		$lf->flush() or die "cant flush $logfile";
	}
	unless ($l =~ s/^LORA (RCV|SND) //) {
		print STDERR "IGNORED: '$o'\n" if $DEBUG;
		return;
	}
	print STDERR "PARSING: '$o'\n" if $DEBUG;
	my $verb = $1;

	my %d = ( 
		ts   => $ts, 
		verb => $verb,
		);

	# soak up generic key:val pairs
	while ($l =~ s/^(\w+):(-?\d+(\.\d+)?) //) {
		my ($k,$v,) = ($1,$2,);
		if (exists $d{$k}) {
			die "dupe key $k in $o";
		}
		$d{$k} = $v;
	}

	# data: is last attr
	unless ($l =~ s/^data://) {
		die "no data in $_";
	}
	unless (defined $d{size}) {
		die "no size in $o";
	}
	$l =~ s/ //g;
	unless ((length $l) == ($d{size}*2)) {
		die "bad size in $o";
	}
	my $d = pack "H*",$l;
        unless (length $d == $d{size}) {
		die "bad packed length";
	}	

	unless ($d =~ /^\xf5/) {
		warn "NOCDP: $o";
		return;
	}

	while (length $d > 1) {
		my $O = substr $d, 0, 8;
		my $t = substr $d, 0, 1, "";
		my $l = ord substr $d, 0, 1, "";
		my $v = substr $d, 0, $l, "";
		my $T = {
			chr(0xf8) => 'iamhere',
			chr(0xf7) => 'payload',
			chr(0xf6) => 'msgid',
			chr(0xf5) => 'sndid',
			chr(0xf4) => 'ping',
			chr(0xf3) => 'path',
			}->{$t} || "";
		#die "unmatched tag ".ord($t)." in ".unpack("H*",$O)." in $o" unless length $T;
		unless (length $T) {
			warn "unmatched tag ".ord($t)." in ".unpack("H*",$O)." in $o";
			last;
		}
		$v =~ s/\x00$//;
		my $k = "cdp_$T";
		if (exists $d{$k}) {
			die "dupe key $k in $o";
		}
		$d{$k} = $v;

		if ($T eq 'payload' && $v =~ /^myid:/) {
			while ($v =~ s/^(\w+):([-\w]+)( |$)//) {
				my ($K,$V)=(lc $1, $2);
				my $k = "cdp_pl_$K";
				if (exists $d{$k}) {
					die "dupe key $k in $o";
				}
				$d{$k} = $V;
			}
		}
	}
	return \%d;
}


my %SEEN;
sub line_to_carb {
	my ($h,) = @_;
	my %d = %$h;
	my $ts = $d{ts};
	die "no ts" unless defined $ts && $ts;
	my $s = "";
	my $pl = 0;
	if (defined($d{cdp_path}) && $d{cdp_path} =~ /(?:,|^)(\w+)$/)  { 
		$s = $1;
		my $p = $d{cdp_path};
		$p =~ s/[^,]+//g;
		$pl = 1 + length $p;
	}
	for (qw/size rssi snr fe/) {
		next unless defined $d{$_};
		&carb("lora.all.$_", $d{$_}, $ts);
		if ($pl) {
			&carb("lora.$s.$_", $d{$_}, $ts);
		}
	}
	if ($pl)  { 
		&carb("lora.$s.pathlen", $pl, $ts);
	}

	if (defined $d{cdp_sndid} && (!($SEEN{$d{cdp_payload}}++))) {
		my $S = $d{cdp_sndid};
		unless ($S =~ /^\w+$/) {
			warn "BADSND: '$S'";
			next;
		}
		my $QS = quotemeta $S;
		if ($d{cdp_path} !~ /^$QS(,|$)/) {
			warn "BADPATH: '$S' vs '".$d{cdp_path}."'";
		}
		&carb("cdp.$S.plsize", length($d{cdp_payload}), $ts);
		if ($pl)  { 
			&carb("cdp.$S.pathlen", $pl, $ts);
		}
		for my $t (qw/bat temp gfh gmah gmfh/) {
			my $v = $d{"cdp_pl_$t"};
			next unless defined $v;
			&carb("cdp.$S.$t", $v, $ts);
		}
	}
}

my %CC;
my $CT;
sub carb {
	my ($k, $v, $t,) = @_;
	return unless defined $v;
	die "bad path $k" unless $k =~ /^[.\w]+$/;
	$t ||= time;
	my $cstep = 60;
	my $T = int($cstep * int($t/$cstep));
	if (defined $CT && $CT != $T) {
		&decarb();
	}
	$CT = $T;

	if (!exists $CC{$k}) {
		$CC{$k} = {
			cnt => 1,
			sum => $v,
			min => $v,
			max => $v,
			};
	} else {
		$CC{$k}{cnt}++;
		$CC{$k}{sum}+=$v;
		$CC{$k}{min} = ($CC{$k}{min} > $v) ? $v : $CC{$k}{min};
		$CC{$k}{max} = ($CC{$k}{max} < $v) ? $v : $CC{$k}{max};
	}
}

sub decarb {
	return unless defined $CT;
	use IO::Socket::INET;
	for my $k (sort keys %CC) {
	my $sock = IO::Socket::INET->new(PeerAddr => 'localhost:2003');
	die "no sock" unless defined $sock && $sock;
		my $K = $CC{$k};
		for my $t (sort keys %$K) {
			my $v = $K->{$t};
			print "CARB: $k.$t $v $CT\n";
			#print CF "$k.$t $v $CT\n";
			print $sock "$k.$t $v $CT\n";
		}
	close $sock;
	}
	%CC = ();
	$CT = undef;
}
## END of carbon / grafana payload

## BEGIN of database payload
sub hash_to_db {
	my ($h,) = @_;
	return unless defined $dbh;

	require POSIX;

	my $sth = $dbh->prepare_cached("INSERT INTO clusterData VALUES (?,?,?,?,?)") or die $dbh->errstr;
	my $rc = $sth->execute(
			POSIX::strftime("%Y-%m-%d %H:%M:%S", gmtime($h->{ts})),
			$h->{cdp_sndid},
			$h->{cdp_msgid},
			$h->{cdp_payload},
			$h->{cdp_path},
		) or die $sth->errstr;

}



