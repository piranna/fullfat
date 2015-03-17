# FullFAT 2.0.0-RTM -- NOW AVAILABLE #
The core library is now frozen, fully tested and very stable.

## Major improvements over RC1.1 ##
  * Cache Write Through - Prevent FS corruption from sudden power failure in mobile devices.
    * This is required if you keep your files open indefinitely.
  * More testsuite stuff.
  * Fixed many problems with unaligned access.
  * Other bug-fixes for problems introduced.
  * Perfect free-space reporting.

# FullFAT 2.0.0-RC1.1 -- NOW AVAILABLE #
This is a must have for all developers using FullFAT. Going from BETA we have made some huge changes to several important parts of FullFAT:

The library code is now FEATURE freezed. Only bug-fixes/feature fixes will be added to this code-base before we declare a 2.0.0-final.

Also DEMO/Documentation/fs-manager will be completed for the final release.

## Major improvements over beta2 ##
  * Unaligned access optimisation. (Huge read/write performance gains).
  * Re-architected buffer management for FAT traversal and Directory management. (1000x perfomance gains) Thanks to Hein\_\_Tibosch.
  * Beginnings of a new TESTSUITE in the demo.
  * FORMAT FAT16/FAT32
  * Write All FAT Mirrors.
  * Sync all FAT Mirrors on unmount.
  * Write Free Cluster Count to Disk.
  * Support for FSINFO sector (FAT32).
  * Linux demo is now fully featured.

## What's Coming Soon? ##
  * Much improved documentation in the Doc dir, Doxygen/Tex/ and PDF from TEX.
  * Demo environment will be completed to include a full set of typical unix commands for handling files.
  * FORMAT with advanced features.
  * Filesystem manager, providing full stdio interface.

## Bug Fix Releases (From RC1) ##
List of all the releases that patch the RC1.
### RC1.1 ###
We fixed RC1! There was a bug with FAT16 traversal/dirent creation and so we've now bumped the RC1 release to RC1.1.

This is an important update for all developers.

# FullFAT 2.0.0-beta2 -- NOW AVAILABLE #
It's finally here, this is a beta release, which means that the core code base is complete. (The FullFAT library). However demo's and documentation are not 100% complete.

# FullFAT #
FullFAT is a fully featured FAT 12/16/32 library.
It features optional LFN (Long File-name support)

  * Fast and Efficient with Low Memory Footprint
  * Scalable from Embedded Systems to Desktop OS’s
  * **Thread Safe**
  * **Removable Media Support**
  * **UNICODE Support**
  * Multiple File Open
  * LFN Support (optional).
  * Fully Featured
  * Optional Caching
  * Safe Caching behaviour
  * Customisable Caching behaviour
  * Multiple & Single Block Reading
  * Platform independent, no assumptions about Endianess
  * Easily integrated into current OS’s and Frameworks

FullFAT provides the most comprehensive, free and open-source FAT filesystem solution.
It can be used commercially during prototyping for free, and products containing FullFAT must obtain a commercial license.

## Further Info ##
Please see http://www.fullfat-fs.co.uk for more information.

## Commercial Licensing ##
Commercial licenses are available from http://www.fullfat-fs.co.uk
