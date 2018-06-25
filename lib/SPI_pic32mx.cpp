
#include "SPI_pic32mx.h"

void SPI::init(uint8_t mode, uint32_t speed)
{
    SPI1BRGCLR = 0xFFFFFFFF;
    SPI1CONCLR = 0xFFFFFFFF;
    SPI1CON2CLR = 0xFFFFFFFF;
    SPI1STATCLR = 0xFFFFFFFF;
    
    SPI1BRG = ( (GetPeripheralClock() / (speed * 2) ) ) - 1;
    SPI1CONSET = 0x00008020 | (mode << 6);
}

void SPI::transmit(uint8_t* buff_tx, uint8_t* buff_rx,uint8_t length)
{
	if(length <= 0) return;
	
	if(length > 8)
	{
		SPI::transmit(buff_tx, buff_rx,(length - 8));
		buff_tx += (length - 8);
		buff_rx += (length - 8);
		length = 8;
	}
	
	uint8_t count = 0;
	while(count < length)
	{
		while(SPI1STATbits.SPITBF); 
		SPI1BUF = *(buff_tx+count);
		count++;
	}
	
	while(SPI1STATbits.SPIBUSY);

	while(count < length)
	{
		*(buff_rx + count) = SPI1BUF;
		count++;
	}

	while(--length);
}