#define DATA_START_BLOCK				0xFE
#define DATA_RES_MASK					0X1F
#define DATA_RES_ACCEPTED				0X05

// SD/MMC commands
#define GO_IDLE_STATE					0
#define SEND_OP_COND					1
#define SWITCH_FUNC						6
#define SEND_IF_COND					8
#define SEND_CSD						9
#define SEND_CID						10
#define STOP_TRANSMISSION				12
#define SEND_STATUS						13
#define SET_BLOCKLEN					16
#define READ_SINGLE_BLOCK				17
#define READ_MULTIPLE_BLOCK				18
#define WRITE_BLOCK						24
#define WRITE_MULTIPLE_BLOCK			25
#define PROGRAM_CSD						27
#define SET_WRITE_PROT					28
#define CLR_WRITE_PROT					29
#define SEND_WRITE_PROT					30
#define ERASE_WR_BLK_STAR_ADDR			32
#define ERASE_WR_BLK_END_ADDR			33
#define ERASE							38
#define LOCK_UNLOCK						42
#define APP_CMD							55
#define GEN_CMD							56
#define READ_OCR						58
#define CRC_ON_OFF						59

// SD ACMDs
#define SD_STATUS						13
#define SD_SEND_NUM_WR_BLOCKS			22
#define SD_SET_WR_BLK_ERASE_COUNT		23
#define SD_SEND_OP_COND					41
#define SD_SET_CLR_CARD_DETECT			42
#define SD_SEND_SCR						51

// R1 status bits
#define STATUS_READY					0x00
#define STATUS_IN_IDLE					0x01
#define STATUS_ERASE_RESET				0x02
#define STATUS_ILLEGAL_COMMAND			0x04
#define STATUS_CRC_ERROR				0x08
#define STATUS_ERASE_SEQ_ERROR			0x10
#define STATUS_ADDRESS_ERROR			0x20
#define STATUS_PARAMETER_ERROR			0x40
#define STATUS_START_BLOCK				0xFE

// SD card type
#define SD_CARD_TYPE_SD1				1
#define SD_CARD_TYPE_SD2				2
#define SD_CARD_TYPE_SDHC				3

//SD card errors
#define SD_CARD_ERROR_CMD0				0x01
#define SD_CARD_ERROR_CMD8				0x02
#define SD_CARD_ERROR_CMD17				0x03
#define SD_CARD_ERROR_CMD24				0x04
#define SD_CARD_ERROR_CMD58				0x06
#define SD_CARD_ERROR_ACMD41			0x08
#define SD_CARD_ERROR_READ				0x0D
#define SD_CARD_ERROR_READ_TIMEOUT		0x0F
#define SD_CARD_ERROR_WRITE				0x11
#define SD_CARD_ERROR_WRITE_PROGRAMMING	0x14
#define SD_CARD_ERROR_WRITE_TIMEOUT		0x15
#define SD_CARD_ERROR_SCK_RATE			0x16
