#
# Public suffix C include generator
#
# Copyright 2016 Vincent Sanders <vince@kyllikki.og>
#
# Permission to use, copy, modify, and/or distribute this software for
# any purpose with or without fee is hereby granted, provided that the
# above copyright notice and this permission notice appear in all
# copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
# WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
# AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
# DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
# OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
# TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.


# This program converts the public suffix list data [1] into a C
#  program with static data representation and acessor function.
#
# The actual data list [2] should be placed in a file effective_tld_names.dat
#
# The C program is written to stdout, the typical 160K input file
#  generates 500K of program and compiles down to a 100K object file
#
# There is a single exported function
#
# const char *getpublicsuffix(const char *hostname)
#
# This returns the public suffix of the passed hostname or NULL if
#  there was an error processing the hostname. The returned pointer is
#  within the passed hostname so if the returned pointer is the same as
#  hostname the whole hostname is a public suffix otherwise the passed
#  hostname has a private part.
#
# The resulting C file is mearly a conversion of the input data (the
#  added c code is from this source and licenced under the same terms)
#  and imposes no additional copyright above that of the source data
#  file.
#
# Note: The pnode structure is built assuming there will never be more
#  label nodes than can fit in an unsigned 16 bit value (65535) but as
#  there are currently around 8000 nodes there is space for another
#  58,000 before this becomes an issue.
#
# [1] https://publicsuffix.org/
# [2] https://publicsuffix.org/list/effective_tld_names.dat


# debian package for ordered hashes: libtie-ixhash-perl
# debian package for punycode encode: libidna-punycode-perl

use strict;
use warnings;
use utf8;
use File::Basename;
use Tie::IxHash;
use IDNA::Punycode;

sub treesubdom
{
    my ($tldtree_ref, $nodeidx_ref, $strtab_ref, $stridx_ref, $parts_ref) = @_;

    my $domelem = pop @{$parts_ref}; # Doamin element
    my $isexception = 0;
    tie my %node, 'Tie::IxHash'; # this nodes hash

    # deal with explicit domain exceptions
    $isexception = ($domelem =~ s/\A!//);
    if ($isexception != 0) {
	$node{"!"} = {};
	$$nodeidx_ref += 1;
    }
    my $domelem_puny = encode_punycode($domelem);

    # Update string table
    if (! exists $strtab_ref->{$domelem_puny}) {
	# add to string table
	$strtab_ref->{$domelem_puny} = $$stridx_ref;
	{
	    use bytes;
	    # update the character count index
	    $$stridx_ref += length($domelem_puny);
	}

    }

    # link new node list into tree
    if (! exists $tldtree_ref->{$domelem_puny}) {
	$tldtree_ref->{$domelem_puny} = \%node;
	$$nodeidx_ref += 1;
    }

    # recurse down if there are more parts to the domain
    if (($isexception == 0) && (scalar(@{$parts_ref}) > 0)) {
	treesubdom($tldtree_ref->{$domelem_puny}, $nodeidx_ref, $strtab_ref, $stridx_ref, $parts_ref);
    }
}

# output an array of bytes in binary
sub phexbits
{
    use bytes;
    my %btoh = (
	"0000" => "0",
	"1000" => "1",
	"0100" => "2",
	"1100" => "3",
	"0010" => "4",
	"1010" => "5",
	"0110" => "6",
	"1110" => "7",
	"0001" => "8",
	"1001" => "9",
	"0101" => "A",
	"1101" => "B",
	"0011" => "C",
	"1011" => "D",
	"0111" => "E",
	"1111" => "F",
	);
    my ($str) = @_;
    my @nyb = unpack "a4" x ((length($str)/4)-1) . "a*", $str;
    my $ret = "   ";
    my $count = 0;

    do {
	my $a = shift @nyb;
	my $b = shift @nyb;
	my $c = shift @nyb;
	my $d = shift @nyb;
	my $e = shift @nyb;
	my $f = shift @nyb;
	my $g = shift @nyb;
	my $h = shift @nyb;

	$ret = $ret . " 0x" . $btoh{$h} . $btoh{$g}. $btoh{$f} . $btoh{$e}. $btoh{$d} . $btoh{$c}. $btoh{$b} . $btoh{$a} . ",";
	$count += 1;
	if ($count == 6) {
	    $ret = $ret . "\n   ";
	    $count = 0;
	}

    } while (@nyb > 0);

    if ($count != 0) {
	$ret = $ret . "\n";
    }

    return $ret;
}


# produce encode and decode dictionary from a tree
sub walk_huffman_tree {
    my ($node, $code, $h, $rev_h) = @_;

    my $c = $node->[0];
    if (ref $c) {
	walk_huffman_tree($c->[$_], $code.$_, $h, $rev_h) for 0,1;
    } else {
	$h->{$c} = $code; $rev_h->{$code} = $c;
    }

    $h, $rev_h;
}

# use huffman encoding to generate a dictionary from a string
#
# returns a pair of hashes for encode and decode
sub mk_huffman_tree {
    my ($domelem_array_ref) = @_;

    # generate string table to get source of huffman frequencies
    my $stringtable = "*!";	# table being generated
    for my $domelem (@$domelem_array_ref) {
	my $substridx = index($stringtable, $domelem);
	if ($substridx == -1) {
	    # no existing string match to put complete label into table
	    #use bytes;
	    $stringtable .= $domelem;
	}
    }

    my (%freq, @nodes);
    for my $c (split //, $stringtable) {
	$freq{$c}++;
    }

    @nodes = map([$_, $freq{$_}], keys %freq);

    # build priority queue
    do {
	@nodes = sort {$a->[1] <=> $b->[1]} @nodes;
	my ($x, $y) = splice @nodes, 0, 2;
	push @nodes, [[$x, $y], $x->[1] + $y->[1]]
    } while (@nodes > 1);

    return @nodes;
}

sub encode {
	my ($str, $dict) = @_;
	join '', map $dict->{$_}//die("bad char $_"), split '', $str
}

# recursively walk the tree and generate nodes
#
# each huffman graph element is made up from three nodes
# [
#   [
#     [ l ],
#     [ r ]
#   ],
#   count
# ]
#
# the l and r nodes may either be additional graph elements or terminal elements
#
sub generate_huffman_node
{
    my ($dict, $node, $htableidx) = @_;

    if (! ref $node->[0]) {
	die("bad huffman tree");
    }

    my $htable = "";

    my $l = $node->[0]->[0];
    my $r = $node->[0]->[1];

    my $lhtable = "";
    my $rhtable = "";
    my $lhtableidx = 0;
    my $rhtableidx = 0;
    $htableidx += 2;

    if (ref $l->[0]) {
	($lhtable, $lhtableidx) = generate_huffman_node($dict, $l, $htableidx);
	$htable .= "    { 0, " . $htableidx. " },\n";
    } else {
	$htable .= "    { 1, '" . $l->[0] . "' }, /* '" . $l->[0] . "': " . $dict->{$l->[0]} . " */\n";
	$lhtableidx = $htableidx;
    }

    if (ref $r->[0]) {
	($rhtable, $rhtableidx) = generate_huffman_node($dict, $r, $lhtableidx);
	$htable .= "    { 0, " . $lhtableidx . " },\n" . $lhtable . $rhtable;
    } else {
	$htable .= "    { 1, '" . $r->[0] . "' }, /* '" . $r->[0] . "': " . $dict->{$r->[0]} . " */\n" . $lhtable;
	$rhtableidx = $lhtableidx;
    }

    $htable, $rhtableidx;

}

# generate c code to represent encoding used
sub generate_huffman_table
{
    my ($hnodes_ref, $dict) = @_;
    my ($htable, $htablesize) = generate_huffman_node($dict, $hnodes_ref->[0], 0);
    my $ret;

    $ret .= "/**\n";
    $ret .= " * Huffman coding node\n";
    $ret .= " */\n";
    $ret .= "struct hnode {\n";
    $ret .= "    uint8_t term:1; /**< non zero if the node terminates a code */\n";
    $ret .= "    uint8_t value:7; /**< value in node */\n";
    $ret .= "};\n\n";

    $ret .= "/**\n";
    $ret .= " * Huffman decoding table\n";
    $ret .= " *\n";
    $ret .= " * nodes are in pairs even nodes are for 0bit odd for 1\n";
    $ret .= " */\n";
    $ret .= "static const struct hnode htable[" . $htablesize . "] = {\n";
    $ret .= $htable;
    $ret .= "};\n\n";

    return $ret;
}

# output string table
#
# array of characters the node table below directly indexes entries.
sub generate_string_table
{
    my ($tldtree_ref, $nodeidx_ref, $strtab_ref, $stridx_ref) = @_;

    # obtain sorted array of domain label strings
    my $labcount = 0; # total number of labels
    my $labsize = 0; # total size of labels
    my @tmp_array;
    foreach my $key (keys %$strtab_ref) {
	use bytes;
	push(@tmp_array, $key);
	$labcount += 1;
	$labsize += length($key);
    }
    my @domelem_array = sort { length($b) <=> length($a) } @tmp_array;

    # build huffman tree
    my @hnodes = mk_huffman_tree(\@domelem_array);

    # generate huffman encoding dictionary
    my ($h, $rev_h) = walk_huffman_tree($hnodes[0], '', {}, {});

    # generate bit table using huffman encoding
    my $bittable = encode("*", $h); # table being generated
    my $exceptidx = length($bittable);
    $bittable .= encode("!", $h);
    my $bittablesize = length($bittable);
    my $labfullcount = 2; # labels inserted into the table in full
    for my $domelem (@domelem_array) {
	my $domelembits = encode($domelem, $h);
	my $substridx = index($bittable, $domelembits);
	if ($substridx != -1) {
	    # found existing string match so use it
	    $strtab_ref->{$domelem} = $substridx;
	} else {
	    # no existing string match to put complete label into table
	    use bytes;
	    $strtab_ref->{$domelem} = $bittablesize;
	    $bittable .= $domelembits;
	    $bittablesize += length($domelembits);
	    $labfullcount += 1;
	}
    }
    my $srounding = $bittablesize % 32;
    if ($srounding > 0) {
	# ensure bittable is a multiple of 32 bits long for array generation
	$bittable .= '0' x (32 - $srounding);
	$bittablesize += (32 - $srounding);
    }

    print "enum stab_entities {\n";
    print "    STAB_WILDCARD = 0,\n";
    print "    STAB_EXCEPTION = " . $exceptidx . "\n";
    print "};\n\n";

    # output the node table to allow decoding
    print generate_huffman_table(\@hnodes, $h);


    print "/**\n";
    print " * Domain label string table huffman encoded.\n";
    print " * " . $labcount . " labels(" . $labsize * 8 . " bits) reduced to " . $labfullcount . " labels(" . $bittablesize . " bits)\n";
    print " */\n";
    print "static const uint32_t stab[" . ($bittablesize / 32) . "] = {\n";
    print phexbits($bittable);
    print "};\n\n";

}


# Output the length of the string
sub pstr_len
{
    use bytes;

    my ($str) = @_;
    my $ret;

    my @bytes = unpack('C*', $str);

    $ret = $ret . sprintf("%d", scalar(@bytes));

    return $ret;
}

# generate all the children of a parent node and recurse into each of
#  those updating optidx to point to the next free node
sub calc_pnode
{
    my ($parent_ref, $strtab_ref, $opidx_ref, $nodecount_ref) = @_;
    my $our_dat;
    my $child_dat = "";
    my $startidx = $$opidx_ref;
    my $lineidx = -1;

    # update the output index to after this node
    # need to allow for an additional node for each entry with children

    # iterate over each child element domain/ref pair
    while ( my ($cdom, $cref) = each(%$parent_ref) ) {
	if (scalar keys (%$cref) != 0) {
	    $$opidx_ref += 2;
	} else {
	    $$opidx_ref += 1;
	}
    }

    # entry block
    if ($startidx == ($$opidx_ref - 1)) {
	$our_dat = "\n    /* entry " . $startidx . " */\n";
    } else {
	$our_dat = "\n    /* entries " . $startidx . " to " . ($$opidx_ref - 1) . " */\n";
    }

    # iterate over each child element domain/ref pair
    while ( my ($cdom, $cref) = each(%$parent_ref) ) {
	my $child_count = scalar keys (%$cref);

	$$nodecount_ref += 1; # keep count of number of nodes in tree

	$our_dat .= "    { .label = {" . sprintf("% 7d", $strtab_ref->{$cdom}) . ",". sprintf("% 3d", pstr_len($cdom));
	if ($child_count == 0) {
	    # complete label for no children
	    $our_dat .= ", 0 } },";
	} else {
	    # complete label with children
	    $our_dat .= ", 1 } }, ";
	    $our_dat .= "{ .child = { " . $$opidx_ref . ", " . $child_count . " } },";
	    $child_dat .= calc_pnode($cref, $strtab_ref, $opidx_ref, $nodecount_ref);
	}
	$our_dat .= " /* " . $cdom . " */\n";
    }

    return $our_dat . $child_dat;
}

# main
binmode(STDOUT, ":utf8");

my ($filename) = @ARGV;

if (not defined $filename) {
    die "need filename\n";
}

open(my $fh, '<:encoding(UTF-8)', $filename)
    or die "Could not open file '$filename' $!";

tie my %tldtree, 'Tie::IxHash'; # node tree
my $nodeidx = 1; # count of nodes allowing for the root node

tie my %strtab, 'Tie::IxHash'; # string table
my $stridx = 0;

# put the wildcard match at 0 in the string table
$strtab{'*'} = $stridx;
$stridx += 1;

# put the invert match at 1 in the string table
$strtab{'!'} = $stridx;
$stridx += 1;

# read each line from prefix data and inject into hash tree
while (my $line = <$fh>) {
    chomp $line;

    if (($line ne "") && ($line !~ /\/\/.*$/)) {

	# print "$line\n";
	my @parts=split("\\.", $line);

	# recusrsive call to build tree from root

	treesubdom(\%tldtree, \$nodeidx, \%strtab, \$stridx, \@parts);
    }
}


# C program header
print "/*\n";
print " * Generated with the genpubsuffix tool.\n";
print " *  From file " . basename($filename) . "\n";
print " *  Converted on " . localtime() . "\n";
print " */\n\n";

print "/**\n";
print " * Public suffix list graph node\n";
print " */\n";
print "union pnode {\n";
print "    struct {\n";
print "        unsigned int idx:24; /**< index of domain element in string table */\n";
print "        unsigned int len:6; /**< length of domain element in string table */\n";
print "        unsigned int children:1; /**< has children */\n";
print "    } label;\n";
print "    struct {\n";
print "        uint16_t index; /**< index of first child node */\n";
print "        uint16_t count; /**< number of children of this node */\n";
print "    } child;\n";
print "};\n\n";

generate_string_table(\%tldtree, \$nodeidx, \%strtab, \$stridx);

# output static node array
#
# The constructed array of nodes has all siblings sequentialy and an
# index/count to its children. This yeilds a very compact data
# structure easily traversable.
#
# Additional flags for * (match all) and ! (exception) are omitted as
# they can be infered by having a node with a label of 0 (*) or 1 (!)
# as the string table has those values explicitly created.
#
# As labels cannot be more than 63 characters a byte length is more
# than sufficient.

my $opidx = 2; # output index of node
my $opnodes = ""; # output pnode initialisers
my $opnodecount = 1; # output domain label nodes

# root node initialiser
$opnodes .= "    /* root entry */\n";
$opnodes .= "    { .label = { 0, 0, 1 } }, { .child = { " . $opidx . ", " . scalar keys(%tldtree) . " } },";

# generate node initialiser
$opnodes .= calc_pnode(\%tldtree, \%strtab, \$opidx, \$opnodecount);

print "/**\n";
print " * PSL represented as a directed acyclic graph\n";
print " * There are " . $opnodecount . " labels in " . $opidx . " nodes\n";
print " */\n";
print "static const union pnode pnodes[" . $opidx . "] = {\n";
print $opnodes; # output node initialisors
print "\n};\n\n";
