#include "autogenerated/big_font.h"
#include "autogenerated/small_font.h"
#include "spi.h"
#include "common.h"
#include "clock.h"
#include "disp.h"
#include <util/delay.h>
#include <avr/io.h>

#define DRIVER_OUTPUT_CONTROL                       0x01
#define BOOSTER_SOFT_START_CONTROL                  0x0C
#define GATE_SCAN_START_POSITION                    0x0F
#define DEEP_SLEEP_MODE                             0x10
#define DATA_ENTRY_MODE_SETTING                     0x11
#define SW_RESET                                    0x12
#define TEMPERATURE_SENSOR_CONTROL                  0x1A
#define MASTER_ACTIVATION                           0x20
#define DISPLAY_UPDATE_CONTROL_1                    0x21
#define DISPLAY_UPDATE_CONTROL_2                    0x22
#define WRITE_RAM                                   0x24
#define WRITE_VCOM_REGISTER                         0x2C
#define WRITE_LUT_REGISTER                          0x32
#define SET_DUMMY_LINE_PERIOD                       0x3A
#define SET_GATE_TIME                               0x3B
#define BORDER_WAVEFORM_CONTROL                     0x3C
#define SET_RAM_X_ADDRESS_START_END_POSITION        0x44
#define SET_RAM_Y_ADDRESS_START_END_POSITION        0x45
#define SET_RAM_X_ADDRESS_COUNTER                   0x4E
#define SET_RAM_Y_ADDRESS_COUNTER                   0x4F
#define TERMINATE_FRAME_READ_WRITE                  0xFF

#define DISP_DD DDRC
#define DISP_DD_DC DDC0
#define DISP_DD_RST DDC1
#define DISP_DD_BUSY DDC2
#define DISP_DD_CS DDC3
#define DISP_PORT PORTC
#define DISP_DC PC0
#define DISP_RST PC1
#define DISP_BUSY PC2
#define DISP_CS PC3

const uint8_t lut_full_update[] =
{
	0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22,
	0x66, 0x69, 0x69, 0x59, 0x58, 0x99, 0x99, 0x88,
	0x00, 0x00, 0x00, 0x00, 0xF8, 0xB4, 0x13, 0x51,
	0x35, 0x51, 0x51, 0x19, 0x01, 0x00
};

const uint8_t lut_partial_update[] =
{
	0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x44, 0x12,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void disp_send(uint8_t data) {
	DISP_PORT &= ~(1 << DISP_CS);
	SPI_MasterTransmit(data);
	DISP_PORT |= (1 << DISP_CS);
}

void disp_sendCommand(uint8_t command) {
	DISP_PORT &= ~(1 << DISP_DC);
	disp_send(command);
}

void disp_sendData(uint8_t data) {
	DISP_PORT |= (1 << DISP_DC);
	disp_send(data);
}

void disp_setLut(const uint8_t *lut) {
	disp_sendCommand(WRITE_LUT_REGISTER);
	for (unsigned int iter = 0; iter < 30; ++iter) {
		disp_sendData(lut[iter]);
	}
}

void disp_init(void) {
	SPI_MasterInit();
	/* Set ports direction */
	DISP_DD |= (1 << DISP_DD_DC) | (1 << DISP_DD_RST) | (1 << DISP_DD_CS);
	DISP_DD &= ~(1 << DISP_DD_BUSY);

	/* Reset display */
	DISP_PORT &= ~(1 << DISP_RST);
	_delay_ms(200);
	DISP_PORT |= (1 << DISP_RST);
	_delay_ms(200);
	disp_sendCommand(DRIVER_OUTPUT_CONTROL);
	disp_sendData(0x27);
	disp_sendData(0x01);
	disp_sendData(0x00);

	disp_sendCommand(BOOSTER_SOFT_START_CONTROL);
	disp_sendData(0xD7);
	disp_sendData(0xD6);
	disp_sendData(0x9D);

	disp_sendCommand(WRITE_VCOM_REGISTER);
	disp_sendData(0xA8);

	disp_sendCommand(SET_DUMMY_LINE_PERIOD);
	disp_sendData(0x1A);

	disp_sendCommand(SET_GATE_TIME);
	disp_sendData(0x08);

	disp_sendCommand(BORDER_WAVEFORM_CONTROL);
	disp_sendData(0x03);

	disp_sendCommand(DATA_ENTRY_MODE_SETTING);
	disp_sendData(0x03);

	disp_setLut(lut_full_update);
}

void disp_prepareForMemoryWrite() {
	disp_sendCommand(SET_RAM_X_ADDRESS_START_END_POSITION);
	disp_sendData(0x00);
	disp_sendData(0x0F);

	disp_sendCommand(SET_RAM_Y_ADDRESS_START_END_POSITION);
	disp_sendData(0x00);
	disp_sendData(0x00);
	disp_sendData(0x27);
	disp_sendData(0x01);

	disp_sendCommand(SET_RAM_X_ADDRESS_COUNTER);
	disp_sendData(0x00);

	disp_sendCommand(SET_RAM_Y_ADDRESS_COUNTER);
	disp_sendData(0x00);
	disp_sendData(0x00);

	disp_sendCommand(DISPLAY_UPDATE_CONTROL_2);
	disp_sendData(0xC4);

	disp_sendCommand(WRITE_RAM);
}

void disp_setMemory(const uint8_t data) {
	disp_prepareForMemoryWrite();
	for (unsigned int iter = 0; iter < (DISP_WIDTH * DISP_HEIGHT) / 8; ++iter) {
		disp_sendData(data);
	}
}

void disp_writeMemory(const uint8_t image[][DISP_WIDTH]) {
	disp_prepareForMemoryWrite();
	for (unsigned int x = 0; x < DISP_WIDTH; ++x) {
		for (unsigned int y = 0; y < DISP_HEIGHT / 8; ++y) {
			disp_sendData(pgm_read_byte(&(image[y][DISP_WIDTH - x])));
		}
	}
}

void disp_writeBigTime(TIME *time, bool underscored[10]) {
	disp_prepareForMemoryWrite();
	unsigned int bigFontTopOffset = 1;
	unsigned int bigFontRightOffsetTab[5] = {
		(DISP_WIDTH - (DISP_BIG_FONT_WIDTH * 5)) / 2 + DISP_BIG_FONT_WIDTH * 4,
		(DISP_WIDTH - (DISP_BIG_FONT_WIDTH * 5)) / 2 + DISP_BIG_FONT_WIDTH * 3,
		(DISP_WIDTH - (DISP_BIG_FONT_WIDTH * 5)) / 2 + DISP_BIG_FONT_WIDTH * 2,
		(DISP_WIDTH - (DISP_BIG_FONT_WIDTH * 5)) / 2 + DISP_BIG_FONT_WIDTH * 1,
		(DISP_WIDTH - (DISP_BIG_FONT_WIDTH * 5)) / 2 + DISP_BIG_FONT_WIDTH * 0,
	};

	unsigned int smallFontTopOffset = 12;
	unsigned int smallFontRightOffsetTab[10] = {
		(DISP_WIDTH - (DISP_SMALL_FONT_WIDTH * 10)) / 2 + DISP_SMALL_FONT_WIDTH * 9,
		(DISP_WIDTH - (DISP_SMALL_FONT_WIDTH * 10)) / 2 + DISP_SMALL_FONT_WIDTH * 8,
		(DISP_WIDTH - (DISP_SMALL_FONT_WIDTH * 10)) / 2 + DISP_SMALL_FONT_WIDTH * 7,
		(DISP_WIDTH - (DISP_SMALL_FONT_WIDTH * 10)) / 2 + DISP_SMALL_FONT_WIDTH * 6,
		(DISP_WIDTH - (DISP_SMALL_FONT_WIDTH * 10)) / 2 + DISP_SMALL_FONT_WIDTH * 5,
		(DISP_WIDTH - (DISP_SMALL_FONT_WIDTH * 10)) / 2 + DISP_SMALL_FONT_WIDTH * 4,
		(DISP_WIDTH - (DISP_SMALL_FONT_WIDTH * 10)) / 2 + DISP_SMALL_FONT_WIDTH * 3,
		(DISP_WIDTH - (DISP_SMALL_FONT_WIDTH * 10)) / 2 + DISP_SMALL_FONT_WIDTH * 2,
		(DISP_WIDTH - (DISP_SMALL_FONT_WIDTH * 10)) / 2 + DISP_SMALL_FONT_WIDTH * 1,
		(DISP_WIDTH - (DISP_SMALL_FONT_WIDTH * 10)) / 2 + DISP_SMALL_FONT_WIDTH * 0,
	};

	char str_big[6] = ":::::";
	// TIME is coded in bcd
	str_big[0] = '0' + time->hours / 0x10;
	str_big[1] = '0' + time->hours % 0x10;
	str_big[3] = '0' + time->minutes / 0x10;
	str_big[4] = '0' + time->minutes % 0x10;

	char str_small[11] = "::/::/20::";
	str_small[0] = '0' + time->days / 0x10;
	str_small[1] = '0' + time->days % 0x10;
	str_small[3] = '0' + time->months / 0x10;
	str_small[4] = '0' + time->months % 0x10;
	str_small[8] = '0' + (time->years / 0x10) % 10;
	str_small[9] = '0' + time->years % 0x10;


	for (unsigned int x = 0; x < DISP_WIDTH; ++x) {
		for (unsigned int y = 0; y < DISP_HEIGHT / 8; ++y) {
			char character = '\0';
			//big font
			for (unsigned int charNumber = 0; charNumber < 5; ++charNumber) {
				if (x >= bigFontRightOffsetTab[charNumber] && x < bigFontRightOffsetTab[charNumber] + DISP_BIG_FONT_WIDTH &&
					y >= bigFontTopOffset && y < bigFontTopOffset + DISP_BIG_FONT_HEIGHT / 8) {
					character = str_big[charNumber];
					unsigned int bigFontRightOffset = bigFontRightOffsetTab[charNumber];
					switch (character) {
					case '0':
						disp_sendData(disp_readCompressed(b_0, y - bigFontTopOffset, DISP_BIG_FONT_WIDTH - x + bigFontRightOffset - 1, DISP_BIG_FONT_WIDTH));
						break;
					case '1':
						disp_sendData(disp_readCompressed(b_1, y - bigFontTopOffset, DISP_BIG_FONT_WIDTH - x + bigFontRightOffset - 1, DISP_BIG_FONT_WIDTH));
						break;
					case '2':
						disp_sendData(disp_readCompressed(b_2, y - bigFontTopOffset, DISP_BIG_FONT_WIDTH - x + bigFontRightOffset - 1, DISP_BIG_FONT_WIDTH));
						break;
					case '3':
						disp_sendData(disp_readCompressed(b_3, y - bigFontTopOffset, DISP_BIG_FONT_WIDTH - x + bigFontRightOffset - 1, DISP_BIG_FONT_WIDTH));
						break;
					case '4':
						disp_sendData(disp_readCompressed(b_4, y - bigFontTopOffset, DISP_BIG_FONT_WIDTH - x + bigFontRightOffset - 1, DISP_BIG_FONT_WIDTH));
						break;
					case '5':
						disp_sendData(disp_readCompressed(b_5, y - bigFontTopOffset, DISP_BIG_FONT_WIDTH - x + bigFontRightOffset - 1, DISP_BIG_FONT_WIDTH));
						break;
					case '6':
						disp_sendData(disp_readCompressed(b_6, y - bigFontTopOffset, DISP_BIG_FONT_WIDTH - x + bigFontRightOffset - 1, DISP_BIG_FONT_WIDTH));
						break;
					case '7':
						disp_sendData(disp_readCompressed(b_7, y - bigFontTopOffset, DISP_BIG_FONT_WIDTH - x + bigFontRightOffset - 1, DISP_BIG_FONT_WIDTH));
						break;
					case '8':
						disp_sendData(disp_readCompressed(b_8, y - bigFontTopOffset, DISP_BIG_FONT_WIDTH - x + bigFontRightOffset - 1, DISP_BIG_FONT_WIDTH));
						break;
					case '9':
						disp_sendData(disp_readCompressed(b_9, y - bigFontTopOffset, DISP_BIG_FONT_WIDTH - x + bigFontRightOffset - 1, DISP_BIG_FONT_WIDTH));
						break;
					case ':':
						disp_sendData(disp_readCompressed(b_colon, y - bigFontTopOffset, DISP_BIG_FONT_WIDTH - x + bigFontRightOffset - 1, DISP_BIG_FONT_WIDTH));
						break;
					}
					break;
				}
				else if (underscored[charNumber] && x >= bigFontRightOffsetTab[charNumber] && x < bigFontRightOffsetTab[charNumber] + DISP_BIG_FONT_WIDTH &&
					y == bigFontTopOffset + DISP_BIG_FONT_HEIGHT / 8 + 1) {
					character = '_';
					disp_sendData(0xBF);
				}

			}
			//small font
			if (character == '\0') {
				for (unsigned int charNumber = 0; charNumber < 10; ++charNumber) {
					if (x >= smallFontRightOffsetTab[charNumber] && x < smallFontRightOffsetTab[charNumber] + DISP_SMALL_FONT_WIDTH &&
						y >= smallFontTopOffset && y < smallFontTopOffset + DISP_SMALL_FONT_HEIGHT / 8) {
						character = str_small[charNumber];
						unsigned int smallFontRightOffset = smallFontRightOffsetTab[charNumber];
						switch (character) {
						case '0':
							disp_sendData(disp_readCompressed(s_0, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
							break;
						case '1':
							disp_sendData(disp_readCompressed(s_1, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
							break;
						case '2':
							disp_sendData(disp_readCompressed(s_2, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
							break;
						case '3':
							disp_sendData(disp_readCompressed(s_3, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
							break;
						case '4':
							disp_sendData(disp_readCompressed(s_4, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
							break;
						case '5':
							disp_sendData(disp_readCompressed(s_5, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
							break;
						case '6':
							disp_sendData(disp_readCompressed(s_6, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
							break;
						case '7':
							disp_sendData(disp_readCompressed(s_7, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
							break;
						case '8':
							disp_sendData(disp_readCompressed(s_8, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
							break;
						case '9':
							disp_sendData(disp_readCompressed(s_9, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
							break;
						case ':':
							disp_sendData(disp_readCompressed(s_colon, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
							break;
						case '/':
							disp_sendData(disp_readCompressed(s_slash, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
							break;
						}
						break;
					}
					else if (underscored[charNumber + LEN(bigFontRightOffsetTab)] &&
						x >= smallFontRightOffsetTab[charNumber] &&
						x < smallFontRightOffsetTab[charNumber] + DISP_SMALL_FONT_WIDTH &&
						y == smallFontTopOffset + DISP_SMALL_FONT_HEIGHT / 8 + 1) {
						character = '_';
						disp_sendData(0xBF);
					}
				}
			}
			if (character == '\0') {
				disp_sendData(0xFF);
			}
		}
	}
}

void disp_writeSmallTime(TIME *time, const uint8_t image[][DISP_WIDTH]) {
	disp_prepareForMemoryWrite();

	unsigned int smallFontTopOffset = 12;
	unsigned int smallFontRightOffsetTab[5] = {
		(DISP_WIDTH - (DISP_SMALL_FONT_WIDTH * 5)) / 2 + DISP_SMALL_FONT_WIDTH * 4,
		(DISP_WIDTH - (DISP_SMALL_FONT_WIDTH * 5)) / 2 + DISP_SMALL_FONT_WIDTH * 3,
		(DISP_WIDTH - (DISP_SMALL_FONT_WIDTH * 5)) / 2 + DISP_SMALL_FONT_WIDTH * 2,
		(DISP_WIDTH - (DISP_SMALL_FONT_WIDTH * 5)) / 2 + DISP_SMALL_FONT_WIDTH * 1,
		(DISP_WIDTH - (DISP_SMALL_FONT_WIDTH * 5)) / 2 + DISP_SMALL_FONT_WIDTH * 0,
	};

	char str_small[6] = ":::::";
	// TIME is coded in bcd
	str_small[0] = '0' + time->hours / 0x10;
	str_small[1] = '0' + time->hours % 0x10;
	str_small[3] = '0' + time->minutes / 0x10;
	str_small[4] = '0' + time->minutes % 0x10;

	for (unsigned int x = 0; x < DISP_WIDTH; ++x) {
		for (unsigned int y = 0; y < DISP_HEIGHT / 8; ++y) {
			char character = '\0';
			//small font
			for (unsigned int charNumber = 0; charNumber < 5; ++charNumber) {
				if (x >= smallFontRightOffsetTab[charNumber] && x < smallFontRightOffsetTab[charNumber] + DISP_SMALL_FONT_WIDTH &&
					y >= smallFontTopOffset && y < smallFontTopOffset + DISP_SMALL_FONT_HEIGHT / 8) {
					character = str_small[charNumber];
					unsigned int smallFontRightOffset = smallFontRightOffsetTab[charNumber];
					switch (character) {
					case '0':
						disp_sendData(disp_readCompressed(s_0, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
						break;
					case '1':
						disp_sendData(disp_readCompressed(s_1, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
						break;
					case '2':
						disp_sendData(disp_readCompressed(s_2, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
						break;
					case '3':
						disp_sendData(disp_readCompressed(s_3, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
						break;
					case '4':
						disp_sendData(disp_readCompressed(s_4, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
						break;
					case '5':
						disp_sendData(disp_readCompressed(s_5, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
						break;
					case '6':
						disp_sendData(disp_readCompressed(s_6, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
						break;
					case '7':
						disp_sendData(disp_readCompressed(s_7, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
						break;
					case '8':
						disp_sendData(disp_readCompressed(s_8, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
						break;
					case '9':
						disp_sendData(disp_readCompressed(s_9, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
						break;
					case ':':
						disp_sendData(disp_readCompressed(s_colon, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
						break;
					case '/':
						disp_sendData(disp_readCompressed(s_slash, y - smallFontTopOffset, DISP_SMALL_FONT_WIDTH - x + smallFontRightOffset - 1, DISP_SMALL_FONT_WIDTH));
						break;
					}
					break;
				}
			}
			if (character == '\0') {
				disp_sendData(disp_readCompressed(image, y, DISP_WIDTH - x, DISP_WIDTH));
			}
		}
	}
}

void disp_displayFrame() {
	disp_sendCommand(MASTER_ACTIVATION);
	disp_sendCommand(TERMINATE_FRAME_READ_WRITE);
}

uint8_t disp_readCompressed(const uint8_t img[], unsigned int y, unsigned int x, unsigned int width) {
	unsigned int imageOffset = y * width + x;
	unsigned int currentDecompressedIndex = 0;
	for(unsigned int compressedIndex = 0; compressedIndex <= imageOffset*2; compressedIndex += 2) {
		currentDecompressedIndex += pgm_read_byte(&img[compressedIndex]);
		if(currentDecompressedIndex >= imageOffset) {
			return pgm_read_byte(&img[compressedIndex+1]);
		}
	}
	return 0;
}
