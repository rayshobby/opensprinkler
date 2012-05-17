// Example code for OpenSprinkler

/* Macro definitions
   Creative Commons Attribution-ShareAlike 3.0 license
   Mar 2012 @ Rayshobby.net
*/

// ====== Schedule Defines ======
// !!! If you change the number of slots per hour,
//     please remember to perform a reset !!!
#define SC_SLOTS_PER_HOUR    6   // number of schedule slots per hour
																 // !!! this should be an even number
																 // for memory alignment purpose
#define SC_NUM_SLOTS_PER_DAY (24*SC_SLOTS_PER_HOUR) 
#define SC_SLOT_LENGTH       (60/SC_SLOTS_PER_HOUR)  // length (in minutes) of each schedule slot
#define SC_BYTES_PER_BOARD	 (SC_NUM_SLOTS_PER_DAY*7) // number of bytes required for each board

