OpenSprinkler v1.3 (minor update)

NOTE: functionally the same as v1.2. 

Changes from v1.2
=================
- Added shift register ~OE (output enable) line. More robust valve reset control during start-up. Extension board connectors are changed to 2x4 format.

- TXD/RXD are now used as standard I/O pins. This frees up A2 and A3 analog pins. Thus TXD/RXD can no longer be used for serial communication. If you need serial communication, use software serial instead.

- Added cell battery holder slot (experimental, not functional yet). Removed indicator LED.


