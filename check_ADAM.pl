#!/usr/bin/perl
# check_ADAM
#
# Purpose: Nagios-compatible plugin for NCNR Facility Monitoring
#          1. Connect to Advantech ADAM6000 industrial I/O modules using Modbus
#             protocol over TCP
#          2. read raw data from the specified channels
#          3. scale the raw data to engineering units
#          4. print scaled data to STDOUT
#          5. compare the scaled data against specified thresholds/relationships
#             (gt or lt) and set exit codes to:
#                 0 = NORMAL     threshholds not exceeded
#                 1 = WARNING    (not implemented)
#                 2 = CRITICAL   threshholds exceeded
#                 3 = UNKNOWN    communications or other error encountered
# 
# Author: Dr. N. C. Maliszewskyj, NIST Center for Neutron Research April 2008
package ADAMTCP;
require 5.004;

use POSIX;
use IO::Socket;
use IO::Select;
use Socket;
use Fcntl;

$ADAMTCP::revision = '$Id: check_ADAM.pl,v 1.0 2008/04/28 13:21:56 nickm Exp nickm $ ';
$ADAMTCP::VERSION=1.0;

sub new {
    my $class = shift;
    my %hash = @_;
    my $self = {};

    bless $self;
    $self->{DEBUG} = 0;

    unless (exists $hash{host}) { 
	print STDERR "No host specified";
	exit;
    }
    unless (exists $hash{port}) {
	print STDERR "No port specified";
	exit;
    }
    if (exists $hash{debug}) {
	$self->{DEBUG} = ($hash{debug}) ? 1 : 0;
    }
    $self->tcp_connect($hash{host},$hash{port});
    return $self;
}

sub tcp_connect {
    my $self = shift;
    my $host = shift;
    my $port = shift;

    my $client = IO::Socket::INET->new(Proto    => 'tcp',
				       PeerAddr => $host,
				       PeerPort => $port)
	or die "Can't connect to port $self->{PORT} on host $self->{HOST}: $!";

    my $select = IO::Select->new($client);
    $self->{HOST}   = $host;
    $self->{PORT}   = $port;
    $self->{CLIENT} = $client;
    $self->{SELECT} = $select;
    $self->{TIMEOUT}= 2.0;
    print STDERR "[Connecting to $host:$tcpport]\n" if ($self->{DEBUG});

    return $self;
}

sub tcp_disconnect {
    my $self = shift;

    close $self->{CLIENT};
    undef $self->{CLIENT};
}

sub HexToRTU {
    my $self = shift;
    my $hmessage = shift;
    my $hlen = length($hmessage);
    my $hexval;

    my $rlen = $hlen/2; # Assume length is right
    for ($i=0;$i<$rlen;$i++) {
	$hexval = hex(substr($hmessage,2*$i,2));
	$rlist[$i] = chr($hexval);
    }
    my $rtu = join('',@rlist);
    return $rtu;
}

sub RTUToHex {
    my $self = shift;
    my $rmessage = shift;
    my $rlen = length($rmessage);
    
    my @rlist = unpack('C*',$rmessage);
    undef(@hlist);
    foreach $rval (@rlist) {
	$hval = sprintf("%02x",$rval);
	push(@hlist,$hval);
    }
    my $hex = join('',@hlist);
    return $hex;
}


#   General purpose routine for sending/receiving modbus
#   data. Arguments:
#   Addr - ADAM module address (usually 0x01)
#   Fun  - Modbus function number
#          0x01 - Read Coil Status
#          0x02 - Read Input Status
#          0x03 - Read Holding Registers (16 bits each)
#          0x04 - Read Input   Registers (16 bits each)
#          0x05 - Force Single Coil - Write data to force coil ON/OFF
#          0x06 - Preset Single Register - Write data in 16 bit integers
#          0x08 - Loopback diagnosis
#          0x15 - Force Multiple Coils
#          0x16 - Preset Multiple Registers
#    StartIndex - Module internal address number
#    TotalPoint - The number of points to read/write

sub CommonSendRecv {
    my $self       = shift;
    my $Addr       = shift;
    my $Fun        = shift;
    my $StartIndex = shift;
    my $TotalPoint = shift;

    my $ModCmd = sprintf("%02X%02X%04X%04X",
			 $Addr,$Fun,($StartIndex-1),$TotalPoint);
    my $lModCmd = length($ModCmd);
    my $HexSend = sprintf("0000000000%02X",($lModCmd/2)).$ModCmd;
    my $RTUSend = $self->HexToRTU($HexSend);
    my $lRTUSend = length($RTUSend);
    if ($self->{DEBUG}) {
	printf "Sending \"$HexSend\"\n";
	printf "lRTUSend = $lRTUSend\n";
    }
    $bytes = $self->{CLIENT}->send($RTUSend,0);
    # Check to make sure the number of bytes sent matches the
    # length of the transfer string
    if ($bytes != $lRTUSend) {
	warn "Couldn't send full message: $!\n";
	return undef;
    }
    
    unless ($self->{SELECT}->can_read($self->{TIMEOUT})) {
	warn "Read on socket timed out: $!\n";
	return undef; # Error return
    }
    $rv = $self->{CLIENT}->recv($RTURecv, POSIX::BUFSIZ, 0);    
    unless(defined($rv)) {
	warn "Read on socket failed: $!\n";
	return undef;
    }
    $iRTURecv = length($RTURecv);
    # This is cheap; we should really interpret the modbus packet properly
    return undef if ($iRTURecv < 9);
    $HexRecv = $self->RTUToHex($RTURecv);
    if ($self->{DEBUG}) {
	printf "Received \"$HexRecv\"\n";
	printf "iRTURecv = $iRTURecv\n";
    }
    # Consistency check
    if ((substr($HexRecv,14,2)) eq (substr($HexSend,14,2))) {
	$DataLength = unpack('B',substr($RTURecv,8,1));
	my $Data = substr($RTURecv,9);
	return $Data
    }
    return undef
}

# Function 0x01
sub ReadCoilStatus {
    my $self       = shift;
    my $Addr       = shift;
    my $StartIndex = shift;
    my $TotalPoint = shift;

    my $Data = $self->CommonSendRecv($Addr, 1, $StartIndex, $TotalPoint);
    return $Data;
}
# Function 0x02
sub ReadInputStatus {
    my $self       = shift;
    my $Addr       = shift;
    my $StartIndex = shift;
    my $TotalPoint = shift;

    my $Data = $self->CommonSendRecv($Addr, 2, $StartIndex, $TotalPoint);
    return $Data;
}
# Function 0x03
sub ReadHoldingRegs {
    my $self       = shift;
    my $Addr       = shift;
    my $StartIndex = shift;
    my $TotalPoint = shift;

    my $Data = $self->CommonSendRecv($Addr, 3, $StartIndex, $TotalPoint);
    return undef unless (defined($Data));
    $lData = length($Data);
    $nvals = $lData/2;
    undef(@regs);
    for($i=0;$i<$nvals;$i++) {
	$offset = $i*2;
	$sval = unpack('n',substr($Data,$offset,2));
	push(@regs,$sval);
    }

    return @regs;
}
# Function 0x04
sub ReadInputRegs {
    my $self       = shift;
    my $Addr       = shift;
    my $StartIndex = shift;
    my $TotalPoint = shift;

    my $Data = $self->CommonSendRecv($Addr, 4, $StartIndex, $TotalPoint);
    return undef unless (defined($Data));
    $lData = length($Data);
    $nvals = $lData/2;
    undef(@regs);
    for($i=0;$i<$nvals;$i++) {
	$offset = $i*2;
	$sval = unpack('n',substr($Data,$offset,2));
	push(@regs,$sval);
    }

    return @regs; 
}
# Function 0x05
sub ForceSingleCoil {
    my $self       = shift;
    my $Addr       = shift;
    my $StartIndex = shift;
    my $TotalPoint = shift;
    my $Data = $self->CommonSendRecv($Addr, 5, $StartIndex, $TotalPoint);
    return $Data;
}
# Function 0x06
sub PresetSingleReg {
    my $self       = shift;
    my $Addr       = shift;
    my $Index      = shift;
    my $iData      = shift;

    # Should coerce $iData to 16 bits
    my $ModCmd = sprintf("%02X%02X%04X%04X",
			 $Addr,6,($Index-1),$TotalPoint,$iData);
    my $lModCmd = length($ModCmd);
    my $HexSend = sprintf("0000000000%02X",($lModCmd/2)).$ModCmd;
    my $RTUSend = $self->HexToRTU($HexSend);
    my $lRTUSend = length($RTUSend);
    if ($self->{DEBUG}) {
	printf "Sending \"$HexSend\"\n";
	printf "lRTUSend = $lRTUSend\n";
    }
    $bytes = $self->{CLIENT}->send($RTUSend,0);
    # Check to make sure the number of bytes sent matches the
    # length of the transfer string
    if ($bytes != $lRTUSend) {
	warn "Couldn't send full message: $!\n";
	return undef;
    }
    
    unless ($self->{SELECT}->can_read($self->{TIMEOUT})) {
	warn "Read on socket timed out: $!\n";
	return undef; # Error return
    }
    $rv = $self->{CLIENT}->recv($RTURecv, POSIX::BUFSIZ, 0);    
    unless(defined($rv)) {
	warn "Read on socket failed: $!\n";
	return undef;
    }
    $iRTURecv = length($RTURecv);
    # This is cheap; we should really interpret the modbus packet properly
    return undef if ($iRTURecv < 12);
    $HexRecv = $self->RTUToHex($RTURecv);
    if ($self->{DEBUG}) {
	printf "Received \"$HexRecv\"\n";
	printf "iRTURecv = $iRTURecv\n";
    }
    # Consistency check
    if ((substr($HexRecv,14,2)) eq (substr($HexSend,14,2))) {
	$DataLength = unpack('B',substr($RTURecv,8,1));
	my $Data = substr($RTURecv,9);
	return $Data
    }
    return undef    
}
# Function 0x0F
sub ForceMultiCoils {
    my $self       = shift;
    my $Addr       = shift;
    my $iCoilIndex = shift;
    my $iTotalPoint= shift;
    my $iTotalByte = shift;
    my @iData      = @_;
    my $i=0;

    # Should coerce $iData to 16 bits
    my @Mod;
    $Mod[0] = sprintf("%02X%02X%04X%04X%02X",
			 $Addr,0x0F,($iCoilIndex-1),$iTotalPoint,$iTotalByte);
    for ($i=0;$i<$iTotalPoint;$i++) {
	push(@Mod,sprintf("%02X",$iData[$i]));
    }
    my $ModCmd = join('',@Mod);
    my $lModCmd = length($ModCmd);
    my $HexSend = sprintf("0000000000%02X",($lModCmd/2)).$ModCmd;
    my $RTUSend = $self->HexToRTU($HexSend);
    my $lRTUSend = length($RTUSend);
    if ($self->{DEBUG}) {
	printf "Sending \"$HexSend\"\n";
	printf "lRTUSend = $lRTUSend\n";
    }
    $bytes = $self->{CLIENT}->send($RTUSend,0);
    # Check to make sure the number of bytes sent matches the
    # length of the transfer string
    if ($bytes != $lRTUSend) {
	warn "Couldn't send full message: $!\n";
	return undef;
    }
    
    unless ($self->{SELECT}->can_read($self->{TIMEOUT})) {
	warn "Read on socket timed out: $!\n";
	return undef; # Error return
    }
    $rv = $self->{CLIENT}->recv($RTURecv, POSIX::BUFSIZ, 0);    
    unless(defined($rv)) {
	warn "Read on socket failed: $!\n";
	return undef;
    }
    $iRTURecv = length($RTURecv);
    # This is cheap; we should really interpret the modbus packet properly
    return undef if ($iRTURecv < 12);
    $HexRecv = $self->RTUToHex($RTURecv);
    if ($self->{DEBUG}) {
	printf "Received \"$HexRecv\"\n";
	printf "iRTURecv = $iRTURecv\n";
    }
    # Consistency check
    if ((substr($HexRecv,14,2)) eq (substr($HexSend,14,2))) {
	$DataLength = unpack('B',substr($RTURecv,8,1));
	my $Data = substr($RTURecv,9);
	return $Data
    }
    return undef    
}
# Function 0x10
sub PresetMultiRegs {
    my $self       = shift;
    my $Addr       = shift;
    my $iStartReg   = shift;
    my $iTotalReg  = shift;
    my $iTotalByte = shift;
    my $Data      = @_;
    my $i,@Mod;

    # Should coerce $iData to 16 bits
    $Mod[0] = sprintf("%02X%02X%04X%04X%02X",
			 $Addr,0x10,($iStartReg-1),$iTotalPoint,$iTotalByte);
    @iData = split('C*',$Data);
    for ($i=0;$i<$iTotalPoint;$i++) {
	push(@Mod,sprintf("%02X",$iData[$i]));
    }
    my $ModCmd = join('',@Mod);

    my $lModCmd = length($ModCmd);
    my $HexSend = sprintf("0000000000%02X",($lModCmd/2)).$ModCmd;
    my $RTUSend = $self->HexToRTU($HexSend);
    my $lRTUSend = length($RTUSend);
    if ($self->{DEBUG}) {
	printf "Sending \"$HexSend\"\n";
	printf "lRTUSend = $lRTUSend\n";
    }
    $bytes = $self->{CLIENT}->send($RTUSend,0);
    # Check to make sure the number of bytes sent matches the
    # length of the transfer string
    if ($bytes != $lRTUSend) {
	warn "Couldn't send full message: $!\n";
	return undef;
    }
    
    unless ($self->{SELECT}->can_read($self->{TIMEOUT})) {
	warn "Read on socket timed out: $!\n";
	return undef; # Error return
    }
    $rv = $self->{CLIENT}->recv($RTURecv, POSIX::BUFSIZ, 0);    
    unless(defined($rv)) {
	warn "Read on socket failed: $!\n";
	return undef;
    }
    $iRTURecv = length($RTURecv);
    # This is cheap; we should really interpret the modbus packet properly
    return undef if ($iRTURecv < 12);
    $HexRecv = $self->RTUToHex($RTURecv);
    if ($self->{DEBUG}) {
	printf "Received \"$HexRecv\"\n";
	printf "iRTURecv = $iRTURecv\n";
    }
    # Consistency check
    if ((substr($HexRecv,14,2)) eq (substr($HexSend,14,2))) {
	$DataLength = unpack('B',substr($RTURecv,8,1));
	my $Data = substr($RTURecv,9);
	return $Data
    }
    return undef    
}

sub ASCII_Cmd {
    my $self   = shift;
    my $Addr   = shift;
    my $regLen = shift;
    my $Cmd    = shift;

    my $szCmd  = $Cmd.'\r';
    my $iCmd   = length($szCmd);
    if ($iCmd % 2) {
	$iCmd++;
	$szCmd .= chr(0);
    }
    my $iReg = $iCmd/2;
    $iSend = $self->PresetMultiRegs($Addr,10000,$iReg,$iCmd,$szCmd);
    @Data = $self->ReadHoldingRegs($Addr,10000,$regLen);
    return undef unless(defined(@Data));

    undef(@outlist);
    foreach $d (@Data) {
	$msb = ($d & 0xFF00) >> 8;
	$lsb = ($d & 0xFF);
	push(@outlist,pack('CC',$msb,$lsb));
    }
    $response = join('',@outlist);
    return $response;
}


sub DumpBytes {
    my $self = shift;
    my $uString = shift;
    my @bytes = unpack('C*',$uString);
    foreach $byte (@bytes) {
	printf "<%02x>",$byte;
    }
}

# Start the real script here
package main;
use Getopt::Std;

#defaults
$addr       = '129.6.120.165';
$port       = 502;
$conv       = 1.0;
$offset     = 0.0;
$threshhold = 4096.0;
$compare    = 0;
$relation   = 1;
$chan       = 1;
$ModAddr    = 1;
$numChans   = 1;

$usage = 
    "Usage: check_ADAM [-H host] [-p port] [-c conversion] [-o offset] [-t thresh]"."\n".
    "       -H host         IP Address of ADAM module"."\n".
    "       -p port         TCP port of MODBUS service on ADAM module"."\n".
    "       -n channel      Channel to read on ADAM module"."\n".
    "       -c conversion   Conversion from raw units to engineering units"."\n".
    "       -o offset       Offset of conversion from proper units"."\n".
    "       -t threshold    Threshhold for \"abnormal\" operation"."\n".
    "       -r <gt/lt>      Threshhold is upper(gt)/lower(lt) bound"."\n".
    "       -h              Print this message"."\n";

# Exit codes
$EXIT_NORMAL  = 0;
$EXIT_WARNING = 1;
$EXIT_CRITICAL= 2;
$EXIT_UNKNOWN = 3;

# Process command line arguments
# Not currently validating script input
getopts('H:p:n:c:o:t:r:h') or die "Usage";
if ($opt_H) { $addr = $opt_H; }
if ($opt_p) { $port = $opt_p; }
if ($opt_n) { $chan = $opt_n; }
if ($opt_c) { $conv = $opt_c; }
if ($opt_o) { $offset = $opt_o; }
if ($opt_t) { $threshhold = $opt_t; 
	      $compare = 1;
}
if ($opt_r) { 
    if ($opt_r =~ /gt/i) {
	$relation = 1;
    } elsif ($opt_r =~ /lt/i) {
	$relation = -1;
    } else {
	print STDERR "Relationship must either be \"gt\" or \"lt\"\n";
	exit($EXIT_UNKNOWN);
    }
}
if ($opt_h) {
    print STDERR "$usage\n";
    exit($EXIT_UNKNOWN);
}

# Now do some real work
unless ($mbt = ADAMTCP->new(host => $addr, port=>$port,debug=>0))
{
    print STDERR "Can't create ADAMTCP object: $!\n";
    exit($EXIT_UNKNOWN);
}


@regs = $mbt->ReadInputRegs($ModAddr, $chan, $numChans);
unless (defined(@regs)) {
    print STDERR "Could not read input registers: $!\n";
    exit($EXIT_UNKNOWN);
}

foreach $r (@regs) {
    push(@eng,($conv * $r + $offset));
}

# Print results
print "$eng[0]\n";
if ($compare) {
    if (($relation > 0) && ($eng[0] > $threshhold)) {
	exit($EXIT_CRITICAL);
    } elsif (($relation < 0) && ($eng[0] < $threshhold)) {
	exit($EXIT_CRITICAL);
    }
}

exit($EXIT_NORMAL);

