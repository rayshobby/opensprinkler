#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
	uint8_t _SS		= 53;
	uint8_t _SS_HW	= 53;
	uint8_t _MOSI	= 51;
	uint8_t _MISO	= 50;
	uint8_t _SCK	= 52;
#elif defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__)
	uint8_t _SS		= 0;
	uint8_t _SS_HW	= SS;
	uint8_t _MOSI	= MOSI;
	uint8_t _MISO	= MISO;
	uint8_t _SCK	= SCK;
#else  
	uint8_t _SS		= 10;
	uint8_t _SS_HW	= 10;
	uint8_t _MOSI	= 11;
	uint8_t _MISO	= 12;
	uint8_t _SCK	= 13;
#endif

