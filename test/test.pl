#!/usr/bin/perl

# use strict;

#############################################
#
# Main routine
#
#############################################

#
# "constants"
#
my $ERRORS_FILE      = "errors.ini"; # all error codes are here
my $TESTS_FILE       = ".tests";     # each folder with tests has this file
my $SWITCH_OPT       = "/opt";       # switch: perf heavy tests are OK
my $SWITCH_FINAL     = "/final";     # switch: this is a final run, leave out version specific info (for comparison with future versions)
my $FAILLOG_FILE     = ".failures";  # suffix of a log storing summary of failures

my $OPTION_NOSEEDING = "NO_SEEDING"; # should skip seeding tests

#
# parse parameters
#

my $exe = shift; # exe location
my $out = shift; # output file location

if ( $exe eq ""
  || $out eq ""
  || $exe eq "-?" 
  || $exe eq "/?" ) {
    usage();
    exit(0);
}

my $bin_optimized = false; # we're running optimized binaries
my $run_final     = false; # we're running on the final build

my $switch;

while( $switch = shift ) {
    if ( $switch eq $SWITCH_OPT ) {
        $bin_optimized = true;
    }
    if ( $switch eq $SWITCH_FINAL ) {
        $run_final = true;
    }
}

#
# print header
#

print STDOUT "[" . $exe . "]\n";

#
# map error names to error codes
#

open ( ERRORS, "<$ERRORS_FILE" ) ||
    die "Can't open necessary file $ERRORS_FILE";

my %errors;

while (<ERRORS>) {
    if ( ($_ =~ /^#/) || ($_ =~ /^[ \t]*$/)) {
        next;
    }
    my ($name, $val) = ( $_ =~ /([a-zA-Z_]*)\s*([0-9]*)/ );
    $errors{$name} = $val;
    
}
close ERRORS;

#
# clean log files
#
unlink($out);
unlink($out . $FAILLOG_FILE);

#
# process each folder that doesn't start with $excludeprefix,
# and doesn't contain .
# if necessary, exclude perf heavy dirs
#

processCommandFile($exe, $TESTS_FILE, $out);

while(<*>) {
    if ( $_ =~ /__.*/ ) {
        next;
    }

    if ( $_ =~ /.*\..*/ ) {
        next;
    }
    
    if ( $bin_optimized eq false && $_ =~ /.*\+.*/ ) {
        next;
    }

    processCommandFile($exe, $_ . "/" . $TESTS_FILE, $out);
}

#############################################
#
# Parses each ini file and runs all commands
#
#############################################
sub processCommandFile {

    my $exe;  # which exe should be tested
    my $file; # which test case file to process
    my $out;  # output file

    ( $exe, $file, $out ) = @_;

    open ( CMDS, "<$file" ) ||
        warn "ERROR: Can't open file $file";

    while (<CMDS>) {
        if ( ($_ =~ /^#/) || ($_ =~ /^[ \t]*$/)) {
            next;
        }

        my $cmdline;   # what to run
        my $expresult; # what to expect
        my $options;   # options modifying the execution
        	
        #
        # are there options
        # 
        if ( $_ =~ /.*\[.*\].*/ ) {
            ($cmdline, $expresult, $options) = ( $_ =~ /\s*(.*)\s*->\s*(\S*)\s*\[(.*)\]\s*/ );
        } else {
            ($cmdline, $expresult) = ( $_ =~ /\s*(.*)\s*->\s*(\S*)\s*/ );
            $options = "";
        }
        
        my ($filepath) = ( $file =~ /^(.*\/).*$/ );
        
        # print STDOUT "$_ [ $cmdline ] [ $filepath ] [ $expresult ] [ $options ]\n";
        # next;
        processCommand( $exe, $filepath, $cmdline, $out, $errors{$expresult}, $options );
    }

    close CMDS;
}

#############################################
#
# Processes individual commands
#
#############################################
sub processCommand {

    my $exe;       # which exe should be tested
    my $filepath;  # 
    my $cmdline;   # what to run
    my $out;       # output file
    my $expresult; # what to expect
    my $options;   # options modifying the execution

    ($exe, $filepath, $cmdline, $out, $expresult, $options) = @_;

    if ( $cmdline eq "" ) {
	    return;
    }
    print STDOUT "$filepath$cmdline \n";

    logText( $out, "\n\n#################################################################\n\n\n" );

    #
    # read entire model file
    #
    my ($modelname) = split( ' ', $filepath . $cmdline );
    $modelname =~ s/\//\\/g;
    open (MODEL, $modelname);
    my @model = <MODEL>;
    close MODEL;
    
    logText( $out, "+++++++++++++++++++++++++++++++++++\n" );
    logText( $out, "MODEL:\n"     , @model   ."\n" );
    logText( $out, "ARGUMENTS:\n" . $cmdline ."\n\n" );

    $cmdline =~ s/%curdir%/$filepath/;
    $cmdline = $cmdline . " /v";    # add switch for verbose output

    system("$exe $filepath$cmdline 1>.stdout 2>.stderr");
    my $result  = $? / 256; # divide by 256 to get errorlevel

    open( OSTDOUT, ".stdout" );
    my @ostdout = <OSTDOUT>;
    my $ostdout_all = join( "", @ostdout );
    close OSTDOUT;

    open( OSTDERR, ".stderr" );
    my @ostderr = <OSTDERR>;
    my $ostderr_all = join( "", @ostderr );
    close OSTDERR;
    
    logText( $out, "EXPECTED: " . $expresult ."\n" );
    logText( $out, "ACTUAL:   " . $result    ."\n\n" );

    if ( $expresult ne $result ) {
        logText( $out, "!!! FAILURE !!!\n" );

        print STDOUT "ERROR: was expecting " . $expresult . ", got " . $result . "\n";
        logText( $out . $FAILLOG_FILE, "EXP: " . $expresult . " ACT: " . $result . "    " . $cmdline . "\n" );
    }

    if( $run_final eq false ) {
        logText( $out, "+++++++++++++++++++++++++++++++++++\n\n" );
        logText( $out, "STDOUT:" . "\n" );
        logText( $out, $ostdout_all . "\n" );
    }

    logText( $out, "+++++++++++++++++++++++++++++++++++\n\n" );
    logText( $out, "STDERR:" . "\n" );
    logText( $out, $ostderr_all  . "\n" );

    #
    # seeding testing
    #
    if( $result eq 0 and 
        not ($options =~ /.*$OPTION_NOSEEDING.*/) )
    {
        $cmdline = $cmdline . " /r /e:.stdout";
        system("$exe $filepath$cmdline 1>.stdout2 2>.stderr2");

        open( OSTDOUT2, ".stdout2" );
	my @ostdout2 = <OSTDOUT2>;
        close OSTDOUT2;

        logText( $out, "+++++++++++++++++++++++++++++++++++\n\n" );
        logText( $out, "SEEDING: " );

        if ( @ostdout eq @ostdout2 ) {
            logText( $out, "OK\n" );
        } else {
            logText( $out, "!!! FAILED !!!\n" );
            print STDOUT "ERROR: seeding failure\n";
            logText( $out . $FAILLOG_FILE, "Seeding failure    " . $cmdline . "\n" );
        }            
    }
    
    unlink(".stdout");
    unlink(".stderr");
    unlink(".stdout2");
    unlink(".stderr2");
}

#############################################
#
# Logs a line of text
#
#############################################
sub logText {
    my $outfile;
    my $line;

    ( $outfile, $line ) = @_;

    open (LOG, ">>$outfile" );
    print LOG  $line;
    close(LOG);
}

#############################################
#
# Displays usage info
#
#############################################
sub usage {
    print "\nperl test.pl bin log [switches]\n\n";
    print " bin    - location of pict.exe\n";
    print " log    - where to dump results\n";

    print "\nswitches:\n";
    print " /opt   - this run is on optimized binaries (perf heavy tests will be performed)\n";
    print " /final - creates a log without generation results (good for comparisons across builds)\n";
}
