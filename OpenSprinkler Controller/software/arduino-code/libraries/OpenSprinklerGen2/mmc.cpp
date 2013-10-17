#include "mmc.h"
#include "HW_AVR.h"
#include <pins_arduino.h>

static void spiSend(uint8_t data)
{
	SPDR = data;
	while (!(SPSR & (1 << SPIF)));
}

static uint8_t spiRec(void) 
{
	spiSend(0XFF);
	return SPDR;
}

static void spiSendLong(const uint32_t data)
{
	union 
	{
		unsigned long l;
		unsigned char c[4];
	} long2char;

	long2char.l = data;

	spiSend(long2char.c[3]);
	spiSend(long2char.c[2]);
	spiSend(long2char.c[1]);
	spiSend(long2char.c[0]);
}

uint8_t waitNotBusy(uint16_t timeoutMillis) 
{
	uint16_t t0 = millis();
	do 
	{
		if (spiRec() == 0XFF)	return true;
	}
	while (((uint16_t)millis() - t0) < timeoutMillis);
	return false;
}

uint8_t mmc::cardCommand(uint8_t cmd, uint32_t arg) 
{
	uint8_t status_;

	cbi(P_SS, B_SS);
	waitNotBusy(300);
	spiSend(cmd | 0x40);
	spiSendLong(arg);

	uint8_t crc = 0xFF;
	if (cmd == GO_IDLE_STATE)	crc = 0x95;  // correct crc for CMD0 with arg 0
	if (cmd == SEND_IF_COND)	crc = 0x87;  // correct crc for CMD8 with arg 0X1AA
	spiSend(crc);

	for (uint8_t i = 0; ((status_ = spiRec()) & 0X80) && i != 0XFF; i++);
	return status_;
}

uint8_t cardAcmd(uint8_t cmd, uint32_t arg) 
{
	mmc::cardCommand(APP_CMD, 0);
	return mmc::cardCommand(cmd, arg);
}

uint8_t waitStartBlock(void) 
{
	uint8_t status_;

	uint16_t t0 = millis();
	while ((status_ = spiRec()) == 0XFF) 
	{
		if (((uint16_t)millis() - t0) > SD_READ_TIMEOUT) 
		{
			mmc::_errorCode = SD_CARD_ERROR_READ_TIMEOUT;
			goto fail;
		}
	}
	if (status_ != STATUS_START_BLOCK) 
	{
		mmc::_errorCode = SD_CARD_ERROR_READ;
		goto fail;
	}
	return true;

fail:
	sbi(mmc::P_SS, mmc::B_SS);
	return false;
}

uint8_t setSckRate(uint8_t _speed) 
{
	if (_speed > 6) 
	{
		mmc::_errorCode = SD_CARD_ERROR_SCK_RATE;
		return false;
	}
	SPCR = B01010000 | _speed;
	return true;
}

uint8_t mmc::initialize(uint8_t speed) 
{
	uint16_t t0 = (uint16_t)millis();
	uint32_t arg;
	uint8_t status_;

	P_SS	= portOutputRegister(digitalPinToPort(_SS));
	B_SS	= digitalPinToBitMask(_SS);
	P_MISO	= portOutputRegister(digitalPinToPort(_MISO));
	B_MISO	= digitalPinToBitMask(_MISO);
	P_MOSI	= portOutputRegister(digitalPinToPort(_MOSI));
	B_MOSI	= digitalPinToBitMask(_MOSI);
	P_SCK	= portOutputRegister(digitalPinToPort(_SCK));
	B_SCK	= digitalPinToBitMask(_SCK);

	sbi(P_SS, B_SS);
	sbi(P_SCK, B_SCK);
	sbi(P_MISO, B_MISO);
	pinMode(_SS, OUTPUT);
	pinMode(_MOSI, OUTPUT);
	pinMode(_SCK, OUTPUT);
	pinMode(_MISO, INPUT);
	pinMode(_SS_HW, OUTPUT);
	digitalWrite(_SS_HW, HIGH); // disable any SPI device using hardware SS pin

	sbi(P_SS, B_SS);

	// Enable SPI, Master, clock rate f_osc/128
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1) | (1 << SPR0);
	// clear double speed
	SPSR &= ~(1 << SPI2X);

	for (uint8_t i = 0; i < 10; i++) spiSend(0XFF);

	cbi(P_SS, B_SS);

	while ((status_ = cardCommand(GO_IDLE_STATE, 0)) != STATUS_IN_IDLE) 
	{
		if (((uint16_t)millis() - t0) > SD_INIT_TIMEOUT) 
		{
			_errorCode = SD_CARD_ERROR_CMD0;
			goto fail;
		}
	}

	// check SD version
	if ((cardCommand(SEND_IF_COND, 0x1AA) & STATUS_ILLEGAL_COMMAND)) 
	{
		_card_type = SD_CARD_TYPE_SD1;
	} 
	else 
	{
		// only need last byte of r7 response
		for (uint8_t i = 0; i < 4; i++)	status_ = spiRec();
		if (status_ != 0XAA) 
		{
			_errorCode = SD_CARD_ERROR_CMD8;
			goto fail;
		}
		_card_type = SD_CARD_TYPE_SD2;
	}

	// initialize card and send host supports SDHC if SD2
//	arg = _card_type == SD_CARD_TYPE_SD2 ? 0X40000000 : 0;
	arg=0;

	while ((status_ = cardAcmd(SD_SEND_OP_COND, arg)) != STATUS_READY) 
	{
		// check for timeout
		if (((uint16_t)millis() - t0) > SD_INIT_TIMEOUT) 
		{
			_errorCode = SD_CARD_ERROR_ACMD41;
			goto fail;
		}
	}
	// if SD2 read OCR register to check for SDHC card
	if (_card_type == SD_CARD_TYPE_SD2) 
	{
		if (cardCommand(READ_OCR, 0)) 
		{
			_errorCode = SD_CARD_ERROR_CMD58;
			goto fail;
		}
		if ((spiRec() & 0XC0) == 0XC0)	_card_type = SD_CARD_TYPE_SDHC;
		// discard rest of ocr - contains allowed voltage range
		for (uint8_t i = 0; i < 3; i++) spiRec();
	}
	sbi(P_SS, B_SS);

	return setSckRate(speed);;

fail:
	sbi(P_SS, B_SS);
	return _errorCode;
}

byte mmc::readSector(byte *buffer, uint32_t sector)
{
	uint8_t status_, tries;

	if (_card_type != SD_CARD_TYPE_SDHC)	sector <<= 9;

	tries=0;
	status_ = cardCommand(READ_SINGLE_BLOCK, sector);

	while ((status_) and (tries<SD_READ_RETRIES))
	{
		status_ = cardCommand(READ_SINGLE_BLOCK, sector);
	}
	if (status_)
	{
		_errorCode = SD_CARD_ERROR_CMD17;
		goto fail;
	}

	status_ = waitStartBlock();
	if (!status_) 
	{
		_errorCode=status_;
		goto fail;
	}

	SPDR = 0XFF;

	for (uint16_t i = 0; i < 511; i++) 
	{
		while (!(SPSR & (1 << SPIF)));
		buffer[i] = SPDR;
		SPDR = 0XFF;
	}
	// wait for last byte
	while (!(SPSR & (1 << SPIF)));
	buffer[511] = SPDR;
	sbi(P_SS, B_SS);
	
	return RES_OK;

fail:
	sbi(P_SS, B_SS);
	return _errorCode;
}

byte mmc::writeSector(const byte *buffer, uint32_t sector)
{
	uint8_t status_;

	if (_card_type != SD_CARD_TYPE_SDHC) sector <<= 9;

	if (cardCommand(WRITE_BLOCK, sector)) 
	{
		_errorCode = SD_CARD_ERROR_CMD24;
		goto fail;
	}

	SPDR = DATA_START_BLOCK;

	for (uint16_t i = 0; i < 512; i += 2) 
	{
		while (!(SPSR & (1 << SPIF)));
		SPDR = buffer[i];
		while (!(SPSR & (1 << SPIF)));
		SPDR = buffer[i+1];
	}

	while (!(SPSR & (1 << SPIF)));

	spiSend(0xff);  // dummy crc
	spiSend(0xff);  // dummy crc

	status_ = spiRec();
	if ((status_ & DATA_RES_MASK) != DATA_RES_ACCEPTED) 
	{
		_errorCode = SD_CARD_ERROR_WRITE;
		goto fail;
	}
  
	// wait for flash programming to complete
	if (!waitNotBusy(SD_WRITE_TIMEOUT)) 
	{
		_errorCode = SD_CARD_ERROR_WRITE_TIMEOUT;
		goto fail;
	}
	// response is r2 so get and check two bytes for nonzero
	if (cardCommand(SEND_STATUS, 0) || spiRec()) 
	{
		_errorCode = SD_CARD_ERROR_WRITE_PROGRAMMING;
		goto fail;
	}
	sbi(P_SS, B_SS);
	return RES_OK;

fail:
	sbi(P_SS, B_SS);
	return _errorCode;
}

void mmc::setSSpin(const uint8_t _pin)
{
	_SS=_pin;
}
