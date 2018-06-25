/*
 * File:   UART_pic32mx.cpp
 * Author: Rafael Silva
 *
 * Created on May 19, 2018, 4:00 PM
 */

#include "UART_pic32mx.h"

#define  ADR_REG(_adr)	(*(volatile uint32_t*)(_adr))

#define UxMODE_offset	0x00000000
#define UxSTA_offset	0x00000010
#define UxTXREG_offset	0x00000020
#define UxRXREG_offset	0x00000030
#define UxBRG_offset	0x00000040

#define UART_FIFO_MSK  (UART_FIFO_SIZE - 1)

#ifdef UART1_EN
UART_pic32mx UART1 = UART_pic32mx(_UART1_BASE_ADDRESS);
#endif
#ifdef UART2_EN
UART_pic32mx UART2 = UART_pic32mx(_UART2_BASE_ADDRESS);
#endif

extern "C"
{
#ifdef UART1_EN

void __ISR(_UART_1_VECTOR, IPL4AUTO) Uart1IntHandler(void)
{
	UART1.RXBuffer[UART1.RX_write_index++] = U1RXREG;

	UART1.RX_write_index &= UART_FIFO_MSK;

	IFS1CLR = 0x00000380;	// Clear interrupt flag
}

#endif
#ifdef UART2_EN

void __ISR(_UART_2_VECTOR, IPL4AUTO) Uart2IntHandler(void)
{
	UART2.RXBuffer[UART2.RX_write_index++] = U2RXREG;

	UART2.RX_write_index &= UART_FIFO_MSK;

	IFS1CLR = 0x00E00000; 	// Clear interrupt flag
}

#endif
}

void UART_pic32mx::Init(uint32_t Baud, bool DoubleSpeed)
{
	memset((uint8_t*)this->RXBuffer, 0, UART_FIFO_SIZE);

	this->RX_write_index = 0;
	this->RX_read_index = 0;

	// UxMODE: UARTx Mode Register
	// UxSTA: UARTx Status and Control Register
	// UxTXREG: UARTx Transmit Register
	// UxRXREG: UARTx Receive Register
	// UxBRG: UARTx Baud Rate Register

	// U1RX – UART1 Receive Done 40 32 IFS1<8> IEC1<8> IPC8<4:2> IPC8<1:0> Yes
	// U2RX – UART2 Receiver 54 37 IFS1<22> IEC1<22> IPC9<12:10> IPC9<9:8> Yes

	if(!(this->base_adr & 0x00000F00))
	{
		IEC1CLR = 0x00000380;
		IPC8CLR = 0x0000001F;
		IPC8SET = 0x00000010;
		IFS1CLR = 0x00000380;
		IEC1SET = 0x00000100;
	}
	else
	{
		IEC1CLR = 0x00E00000;
		IPC9CLR = 0x00001F00;
		IPC9SET = 0x00001000;
		IFS1CLR = 0x00E00000;
		IEC1SET = 0x00400000;
	}


	ADR_REG(this->base_adr + UxBRG_offset) = ( (GetPeripheralClock() / ( (DoubleSpeed ? 4UL : 16UL)*Baud))-1);

	ADR_REG(this->base_adr + UxSTA_offset) = (uint32_t)((1 << 10) | (1 << 12));
	ADR_REG(this->base_adr + UxMODE_offset) = (uint32_t)((1 << 15) | (DoubleSpeed << 3));
}

uint8_t UART_pic32mx::ReadByte()
{
	uint8_t byte = this->RXBuffer[this->RX_read_index++];

	RX_read_index &= UART_FIFO_MSK;

	return byte;
}

uint8_t UART_pic32mx::PeekByte()
{
	uint8_t byte = this->RXBuffer[this->RX_read_index];
	return byte;
}

void UART_pic32mx::WriteByte(uint8_t Data)
{
	while((ADR_REG(this->base_adr + UxSTA_offset) & (1 << 9)));    // wait while TX buffer full

	ADR_REG(this->base_adr + UxTXREG_offset) = Data;
}

void UART_pic32mx::Printf(const char* Fmt, ...) // @Fmt string
{
	va_list args;
	va_start(args, Fmt);

	char* ptr;
	int numbytes;
	
	numbytes = vsnprintf(nullptr, 0, Fmt, args) + 1;
	
	ptr = (char*)malloc((numbytes) * sizeof(char));
	
	this->Write((uint8_t*)ptr, vsnprintf(ptr, numbytes, Fmt, args));
	
	free(ptr);

	va_end(args);
}
