Source Object (low level) cache backing store
=============================================

[TOC]

# Introduction

The source object (referred to as low level in the code) content cache
provides a unified API for the rest of the browser to retrieve objects
(HTML files, images etc.) from a URL.

The cache initialy always fufuls these requsts by using the fetcher
system to retrive data according to the URL scheme (network for HTTP,
disc for file etc.) and storing the result in memory.

The cache also provides a system to extend the life of source objects
in memory when they are no longer immediately being used. Only fetch
types where there are well defined rules on caching are considered, in
practice this limits the cache to URLS with HTTP(S) schemes. The
section in RFC2616 [1] on caching specifies these rules.

To further extend the objects lifetime they can be pushed into a
backing store where the objects are available for reuse less quickly
than from memory but faster than retrieving from the network again.

The backing store implementation provides a key:value infrastructure
with a simple store, retrieve and invalidate interface.

The key is the object URL which by definition is unique for a source
object. The value is the source object data *and* the associated
metadata

# Generic filesystem backing store

Although the backing store interface is fully pluggable a generic
implementation based on storing objects on the filesystem in a
hierarchy of directories.

The option to alter the backing store format exists and is controlled
by a version field. It is implementation defined what happens if a
version mis-match occurs.

As the backing store only holds cache data one should not expect a
great deal of effort to be expended converting formats (i.e. the cache
may simply be discarded).

## Layout version 2.02

The version 2 layout stores cache entries in a hash map thus only uses
memory proportional to the number of entries present removing the need
for large fixed size indexes.

The object identifier is generated from nsurl_hash() and data entries
are stored in either a fixed size disc blocks or in separate files on disc.

The file path if stored on disc must conform to the limitations of all
the filesystems the cache can be placed upon.

From http://en.wikipedia.org/wiki/Comparison_of_file_systems#Limits the relevant subset is:
 - path elements no longer than 8 characters
 - acceptable characters are A-Z, 0-9
 - short total path lengths (255 or less)
 - no more than 77 entries per directory (6bits worth)

The short total path lengths mean the encoding must represent as much
data as possible in the least number of characters.

To achieve all these goals we use RFC4648 base32 encoding which packs
five bits into each character of the filename. By splitting the 32bit
identifier using six bits per directory level only five levels of
directory are required with a maximum of 64 entries per
directory. This requires a total path length of 22 bytes (including
directory separators) BA/BB/BC/BD/BE/ABCDEFG

Files that are under 8KiB in size are stored in "small block files"
these are pre allocated 8 Megabyte files on disc in which remove the
need to have many, many small files stored on disc at the expensie of
a some amount of wasted space for files that are smaller than the 8K
block size.

## Layout version 1.1

An object has an identifier value generated from the URL (the unique
key). The value used is obtained using nsurl_hash() which is currently
a 32 bit FNV so is directly usable.

This identifier is adequate to ensure the collision rate for the
hashed URL values (a collision for every 2^16 URLs added) is
sufficiently low the overhead of returning the wrong object (which
backing stores are permitted to do) is not significant.

An entry list is maintained which contains all the metadata about a
given identifier. This list is limited in length to constrain the
resources necessary to maintain it. It is made persistent to avoid the
overhead of reconstructing it at initialisation and to keep the data
used to improve the eviction decisions.

Each object is stored and retrieved directly into the filesystem using
a filename generated from a RFC4648 base32 encoding of an address
value. The objects address is derived from the identifier by cropping
it to a shorter length.

A mapping between the object address and its entry is maintained which
uses storage directly proportional to the size of the address length.

The cropping length is stored in the control file with the default
values set at compile time. This allows existing backing stores to
continue operating with existing data independently of new default
setting. This setting gives some ability to tune the default cache
index size to values suitable for a specific host operating system.

E.g. Linux based systems can easily cope with several megabytes of
mmaped index but RISC OS might want to limit this to a few megabytes
of heap at most.

The files are stored on disc using their base32 address value.
By creating a directory for each character of the encoded filename
(except the last which is of course the leafname) we create a
directory structure where no directory has more than 32 entries.

E.g. A 19bit address of 0x1 would be base32 encoded into AAAB
resulting in the data being stored in a file path of
"/store/prefix/d/B/A/A/BAAAAA".

An address of 0x00040001 encodes to BAAB and a file path of
"/store/prefix/m/B/A/A/BAABAAA"

## Layout Version 1.0

The version 1.0 layout was identical to the 1.1 except base64url
encoding was used, this proved problematic as some systems filesystems
were case insensitive so upper and lower case letters collided.

There is no upgrade provision from the previous version simply delete
the cache directory.

## Control files

### control

A control file is used to hold a list of values describing how the
other files in the backing store should be used.

### entries

this file contains a table of entries describing the files held on the
filesystem.

Each control file table entry is 28 bytes and consists of

 - signed 64 bit value for last use time
 - 32bit full url hash allowing for index reconstruction and
   additional collision detection. Also the possibility of increasing
   the ADDRESS_LENGTH although this would require renaming all the
   existing files in the cache and is not currently implemented.
 - unsigned 32bit length for data
 - unsigned 32bit length for metadata
 - unsigned 16bit value for number of times used.
 - unsigned 16bit value for flags
 - unsigned 16bit value for data block index (unused)
 - unsigned 16bit value for metatdata block index (unused)

### Address to entry index

An entry index is held in RAM that allows looking up the address to
map to an entry in the control file.

The index is the only data structure whose size is directly dependant
on the length of the hash specifically:

    (2 ^ (ADDRESS_BITS - 3)) * ENTRY_BITS) in bytes

where ADDRESS_BITS is how long the address is in bits and ENTRY_BITS
is how many entries the control file (and hence the while
cache) may hold.

## RISCOS values

By limiting the ENTRY_BITS size to 14 (16,384 entries) the entries
list is limited to 448kilobytes.

The typical values for RISC OS would set ADDRESS_BITS to 18. This
spreads the entries over 262144 hash values which uses 512 kilobytes
for the index. Limiting the hash space like this reduces the
effectiveness of the cache.

A small ADDRESS_LENGTH causes a collision (two URLs with the same
address) to happen roughly for every 2 ^ (ADDRESS_BITS / 2) = 2 ^ 9 =
512 objects stored. This roughly translates to a cache miss due to
collision every ten pages navigated to.

## Larger systems

In general ENTRY_BITS set to 16 as this limits the store to 65536
objects which given the average size of an object at 8 kilobytes
yields half a gigabyte of disc used which is judged to be sufficient.

For larger systems e.g. those using GTK frontend we would most likely
select ADDRESS_BITS as 22 resulting in a collision every 2048 objects
but the index using some 8 Megabytes

## Typical values

### Example 1

For a store with 1034 objects generated from a random navigation of
pages linked from the about:welcome page.

Metadata total size is 593608 bytes an average of 574 bytes. The
majority of the storage is used to hold the URLs and headers.

Data total size is 9180475 bytes a mean of 8879 bytes 1648726 in the
largest 10 entries which if excluded gives 7355 bytes average size

### Example 2

355 pages navigated in 80 minutes from about:welcome page and a
handful of additional sites (google image search and reddit)

2018 objects in cache at quit. 400 objects from news.bbc.co.uk alone

Metadata total 987,439 bytes mean of 489 bytes

data total 33,127,831 bytes mean of 16,416 bytes

with one single 5,000,811 byte gif

data totals without gif is 28,127,020 mean 13,945

[1] http://tools.ietf.org/html/rfc2616#section-13
