#!/usr/bin/perl
#
# Copyright 2013 Vivek Dasmohapatra <vivek@collabora.co.uk>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
#   * The above copyright notice and this permission notice shall be included in
#     all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

=head1

Filter the NetSurf combined messages (i10n) file according to language
and platform and generate output in a selection of formats for use
both internally within netsurf and externally for translation
services.

=cut

use strict;

use Getopt::Long ();
use Fcntl qw( O_CREAT O_EXCL O_WRONLY O_APPEND O_RDONLY O_WRONLY );

use IO::Compress::Gzip;

use constant GETOPT_OPTS => qw( auto_abbrev no_getopt_compat bundling );
use constant GETOPT_SPEC =>
  qw( output|o=s
      input|i=s
      lang|l=s
      dlang|d=s
      plat|platform|p=s
      format|fmt|f=s
      warning|W=s
      gzip|z
      help|h|? );

# default option values:
my %opt = qw( dlang en plat any format messages warning none );

sub input_stream  ();
sub output_stream ();
sub formatter     ();
sub static_section($);
sub usage         ();

sub main ()
{
    my $input;
    my $output;
    my $format;
    my $header;
    my $footer;
    my $opt_ok;

    # option parsing:
    Getopt::Long::Configure( GETOPT_OPTS );
    $opt_ok = Getopt::Long::GetOptions( \%opt, GETOPT_SPEC );

    # allow input and output to be specified as non-option arguments:
    if( @ARGV ) { $opt{input } ||= shift( @ARGV ) }
    if( @ARGV ) { $opt{output} ||= shift( @ARGV ) }

    # open the appropriate streams and get the formatter and headers:
    if( $opt_ok )
    {
        $input  = input_stream();
        $output = output_stream();
        $format = formatter();
        $header = static_section('header');
        $footer = static_section('footer');
    }

    # double check the options are sane (and we weren't asked for the help)
    if( !$opt_ok || $opt{help} || $opt{lang} !~ /^[a-z]{2}(?:_[A-Z]{2})?$/ || $opt{dlang} !~ /^[a-z]{2}(?:_[A-Z]{2})?$/ )
    {
        usage();
    }

    # we are good to go:
    print( $output $header );

    my $cur_key;

    my $dlang_key;
    my $dlang_val;

    my $tran_out = 1;
    my $tran_val;
    my $tran_key;

    while (<$input>)
    {
        # skip comment and empty lines
        /^#/    && next;
        /^\s*$/ && next;

        # only parsing things that look like message lines:
        if( /^([a-z]{2}(?:_[A-Z]{2})?).([^.]+).([^:]+):(.*)/ )
        {
            my( $lang, $plat, $key, $val ) = ( $1, $2, $3, $4 );

            # skip the line if it is not for our target platform
            if( $opt{plat} ne 'any' &&
                $opt{plat} ne $plat &&
                'all'      ne $plat )
            {
                next;
            }

            # On key change ensure a translation has been generated
            if ($cur_key ne $key)
            {
                if ($tran_out == 0)
                {
		    # No translaton for previous key
		    if ($cur_key eq $dlang_key)
		    {
			print( $output $format->( $dlang_key, $dlang_val ) );
			if( $opt{warning} eq "fb" )
			{
			    warn( "warning: $dlang_key missing translation in $opt{lang} using $opt{dlang} instead" );
			}
		    }
		    else
		    {
			# No translation and nothing in default language
			warn( "warning: $dlang_key missing translation in $opt{lang} and no fallback in $opt{dlang}" );
		    }
		}
		else
		{
		    if (($opt{dlang} ne $opt{lang} ) && ($tran_key eq $dlang_key) && ($tran_val eq $dlang_val))
		    {
			if( $opt{warning} eq "dup" )
			{
			    warn( "warning: $tran_key value in $opt{lang} is same as in default $opt{dlang}" );
			}
		    }
		}
		$cur_key = $key;
		$tran_out = 0;
	    }

	    # capture the key/value in the default language
	    if( $lang eq $opt{dlang} )
	    {
		$dlang_key = $key;
		$dlang_val = $val;
	    }

	    # output if its the target language
	    if( $lang eq $opt{lang} ) {
		print( $output $format->( $key, $val ) );
		$tran_out = 1;
		$tran_val = $val;
		$tran_key = $key;
	    }
	}
	else
	{
	    warn( "Malformed entry: $_" );
	}
    }

    print( $output $footer );
}

main();

sub usage ()
{
    my @fmt = map { s/::$//; $_ } keys(%{$::{'msgfmt::'}});
    print(STDERR <<TXT );
usage:
     $0 -l lang-code [-d def-lang-code] [-W warning] \
	   [-o output-file] [-i input-file] [-p platform] [-f format] [-z]

     $0 -l lang-code ... [input-file [output-file]]

     lang-code     : en fr ko ...  (no default)
     def-lang-code : en fr ko ...  (default 'en')
     warning       : none, all     (default 'none')
     platform      : any gtk ami   (default 'any')
     format        : @fmt (default 'messages')
     input-file    : defaults to standard input
     output-file   : defaults to standard output
TXT
    exit(1);
}

sub input_stream ()
{
    if( $opt{input} )
    {
	my $ifh;

	sysopen( $ifh, $opt{input}, O_RDONLY ) ||
	  die( "$0: Failed to open input file $opt{input}: $!\n" );

	return $ifh;
    }

    return \*STDIN;
}

sub underlying_output_stream ()
{
    if( $opt{output} )
    {
	my $ofh;

	sysopen( $ofh, $opt{output}, O_CREAT|O_EXCL|O_APPEND|O_WRONLY ) ||
	  die( "$0: Failed to open output file $opt{output}: $!\n" );

	return $ofh;
    }

    return \*STDOUT;
}

sub output_stream ()
{
    my $ofh = underlying_output_stream();

    if( $opt{gzip} )
    {
        $ofh = new IO::Compress::Gzip( $ofh, AutoClose => 1, -Level => 9 );
    }

    return $ofh;
}

sub formatter ()
{
    my $name = $opt{format};
    my $func = "msgfmt::$name"->UNIVERSAL::can("format");

    return $func || die( "No handler found for format '$name'\n" );
}

sub static_section ($)
{
    my $name = $opt{format};
    my $sect = shift();
    my $func = "msgfmt::$name"->UNIVERSAL::can( $sect );

    return $func ? $func->() : "";
}

# format implementations:
{
    package msgfmt::java;

    sub escape { $_[0] =~ s/([:'\\])/\\$1/g; $_[0] }
    sub format { return join(' = ', $_[0], escape( $_[1] ) ) . "\n" }
    sub header { "# autogenerated from " . ($opt{input} || '-stdin-') . "\n" }
}

{
    package msgfmt::messages; # native netsurf format

    sub format { return join( ":", @_ ) . "\n" }
    sub header
    {
	my $in = $opt{input} || '-stdin-';
	return <<TXT;
# This messages file is automatically generated from $in
# at build-time.  Please go and edit that instead of this.\n
TXT
    }
}

{
    package msgfmt::transifex;
    use base 'msgfmt::java';

    # transifex has the following quirks:
    # \ processing is buggy - they re-process every \\ as a \
    # so \\n, instead or producing literal '\n', is interpreted as \ ^J
    # Additionally, although the java properties format specifies
    # that ' should be \ escaped, transifex does not allow/support this:
    sub escape { $_[0] =~ s/(:|\\(?![abfnrtv]))/\\$1/g; $_[0] }
    sub format { return join(' = ', $_[0], escape( $_[1] ) ) . "\n" }
}

########### YAML ###########
#{
#    package msgfmt::yaml;
#    use YAML qw(Dump Bless);
#    print Dump %data;
#}

{
    package msgfmt::android;

    sub header { qq|<?xml version="1.0" encoding="utf-8"?>\n<resources>\n| }
    sub footer { qq|</resources>| }
    sub format
    {
	use HTML::Entities qw(encode_entities);
	my $escaped = encode_entities( $_[1], '<>&"' );
	qq|  <string name="$_[0]">$escaped</string>\n|;
    }
}
