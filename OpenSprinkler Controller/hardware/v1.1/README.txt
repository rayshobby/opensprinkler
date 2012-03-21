OpenSprinkler v1.1 (minor updates)

Changes from v1.0
=================
- The valve ports have been changed to a 'common wire' + 'individual wire' design, which is compatible with most commercial sprinkler controllers. For each valve, connect one end to the common wire, and the other end to the corresponding station port.

- Pinouts are added to link the master controller to extension boards. Each connector is 4pin + 2pin. The connector should go as (master->ext.): CLK->CLK, LAT->LAT, QH*->DATA, VCC->VCC, GND->GND. It is possible to link more extension boards by cascading them in the same way.

- A 750mA fuse is added for current protection.
