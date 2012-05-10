iotest
======

iotest is a simple program for testing the effects of spatial locality. On start up, it does the following:

1. Open a block device file for reading (/dev/sda, for example) using the O_DIRECT flag.
2. For each 512 byte sector, do the following:
  1. Submit a read request for that sector and the next sector
  2. Seek back two sectors
  3. Repeat from 1 for a number of trials.
  4. Print out the sector number and the total amount of time taken by all the trials.
