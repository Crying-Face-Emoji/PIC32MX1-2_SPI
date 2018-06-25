/** Descriptive File Name

   @Title
	UART for PIC32MX1XX/2XX

  @Author
	Name: Rafael Silva
	Github: github.com/Crying-Face-Emoji
 
  @File Name
	UART_pic32mx.h

  @Dependencies
	HardwareConfig.h
	stdint.h	
	xc.h		
	sys/attribs.h	
  
	interrupts must be enabled
	multivectored mode must be enabled
	pins must be set to input/output correspondingly
	pps must be configured
  
  @Software
	Compiler: xc32-gcc (v2.05) 
	IDE: MPLAB X v4.00

  @Hardware
	Any PIC32MX from the 1XX/2XX family
 
  @Description
	Header file for the "UART for PIC32MX1XX/2XX" library
	includes all file inclusions, error checking and declarations
	Created on May 19, 2018, 4:00 PM
 
   @summary
	macros:
	UART_FIFO_SIZE			// size of the rx buffer on all enabled uarts
	GetPeripheralClock			// corresponds to the frquency(Hz) of the peripheral	s
 
	included funcions:
	UARTx.Init(Baud, DoubleSpeed)	// initializes module with given baud
	UARTx.BytesAvailable()		// Returns number of bytes available to read in the RX buffer
	UARTx.Flush()				// "clears" the RX buffer, flushes all pending data
	UARTx.ReadByte()			// return 1 byte of data from the bottom of the FIFO (removes from stack)
	UARTx.PeekByte()			// return 1 byte of data from the bottom of the FIFO (doesn't remove from stack)
	UARTx.Read(Dest , Count)		// puts x(Count) bytes in given location, Dest(pointer) (removes from stack)
	UARTx.WriteByte(Data)		// send 1 given byte
	UARTx.Write(Src, Count)		// send x(Count) bytes from src(pointer)
	UARTx.printf(Fmt, ...)			// Print/Send formatted data (behaves like normal printf)
 
	Variables:
	RX_read_index		// read index in buffer
	RX__write_index		// write index in buffer
	RXBuffer[]			// buffer with receive data, array

	base_addr			// base memory address of module, used for initialization, constant
 **/
 /* *************************************************************************** */

/* TODO: */
/*
 * auto pps configure and port init
 * auto set double speed, by calculating error
 * properly coment the code
 */

#ifndef UART_PIC32MX_H
#define	UART_PIC32MX_H

/* **************************************************************************	*/
/* Section: File inclusions							*/
/* **************************************************************************	*/
//this file is needed for the peripheral clock and uart enables defenition,alternatively you can use your own header
#include "../HardwareConfig.h"

#include <stdint.h>		// declare sets of integer types having specified widths
#include <xc.h>			// std pic32 lib
#include <sys/attribs.h>	// needed for ISR's and addresses

/* **************************************************************************	*/
/* Section: error checking							*/
/* **************************************************************************	*/
// sets the size of the rx buffer on all enabled uarts (must be a power of 2)
#ifndef UART_FIFO_SIZE	
#warning "in UART_pic32mx UART_FIFO_SIZE was not defined, defaulting to 128"
#define UART_FIFO_SIZE 128	// must be a power of 2
#endif
#if (UART_FIFO_SIZE % 2)
#error "in UART_pic32mx UART_FIFO_SIZE must be a power of 2"
#endif

// peripheral clock, used for baud rate setting
#ifndef GetPeripheralClock
#warning "in UART_pic32mx GetPeripheralClock() was not defined, defaulting to 8MHz"
#define GetPeripheralClock()	(8000000)
#endif

/* **************************************************************************	*/
/* Section: Class Declaration/defenition					*/
/* **************************************************************************	*/
class UART_pic32mx
{
public:
/** 
  @Function
	inline UART_pic32mx(volatile uint32_t module_adr) : base_adr(module_adr) {}

  @Summary
	UARTx Class Constructor funcion

  @Description
	initializes a new classe with a given new address
	this address is an address from the pic's virtual memory corresponding to UxMODE register

  @Precondition
	given address must not have beed used before, aka one claas per peripheral.

  @Parameters
	@param base_address virtual memory address from desired module's UxMODE register, available in sys/attribs.h

  @Returns
	void

  @Remarks
	this library already automatically call this funcion, you should not need to use it

  @Example
@code
UART_pic32mx UART1 = UART_pic32mx(_UART1_BASE_ADDRESS);
 */
	inline UART_pic32mx(volatile uint32_t module_adr) : base_adr(module_adr) {}
	
/**
 @Function
   void Init(uint32_t Baud, bool DoubleSpeed);

 @Summary
	Initializes the peripheral with given baudrate, configures all registers associated with the module

 @Description
   configures all registers associated with the module

@Precondition
	peripheral pin select and the corresponding ports must be configured

 @Parameters
	@param Baud Baud rate desired

	@param DoubleSpeed enable/disable double speed mode [1/0]

 @Returns
	void

 @Remarks
	"none"

 @Example
@code
ANSELBCLR = 0xFFFFFFFF;	// Configure Port as Digital
TRISBCLR = 0xFFFFFFBF;		// Configure port for output except rx
LATBCLR = 0xFFFFFFFF;		// Set out state to 0

CFGCONbits.IOLOCK = 0;			// Unlock pps
RPB7R = 0b0001; // 0001 = U1TX		// PPS configure RB7 -> U1TX
U1RXR = 0b0001; // 0001 = RPB6		// PPS configure RB6 -> U1RX
CFGCONbits.IOLOCK = 1;			// Lock pps

UART1.Init(UART_BAUD, 1);		// initalize UART 1

INTCONbits.MVEC = 1;		// Enable multivectored mode(interrupts)
asm volatile ("ei");			// Enable interrupts. 
*/
	void Init(uint32_t Baud, bool DoubleSpeed);

/** 
  @Function
    inline uint16_t BytesAvailable()

  @Summary
	Returns number of bytes available to read in receive buffer

  @Description
	checks the spaces between write index and read index in receive buffer, wich corresponds to the number of bytes not read yet

  @Precondition
	peripheral must be initialized - init()

  @Parameters
    "none"

  @Returns
 unsigned integer 16bit(uint16_t), the number, in decimal, of available bytes

  @Remarks
	"none"
  @Example
@code
char buffer[64];

if(UARTx.BytesAvailable())
{
	UARTx.Read(buffer, UARTx.BytesAvailable());
}	
 */
	inline uint16_t BytesAvailable()
	{
		return (UART_FIFO_SIZE + this->RX_write_index - this->RX_read_index) % UART_FIFO_SIZE;
	}

/** 
  @Function
    inline void Flush()

  @Summary
	flushes receive buffer

  @Description
	iquals the indexes, wich corresponds to 0 bytes available

  @Precondition
	peripheral must be initialized - init()

  @Parameters
	"none"

  @Returns
	void

  @Remarks
	"none"
  @Example
@code
UARTx.Flush();
 */
	inline void Flush()
	{
		this->RX_read_index = this->RX_write_index;
	}

	uint8_t ReadByte();

	uint8_t PeekByte();

	inline void Read(uint8_t* Dest, uint16_t Count)
	{
		while(Count--)
			*(Dest++) = this->ReadByte();
	}

	inline void Read(void* Dest, uint16_t Count)
	{
		this->Read((uint8_t*)Dest, Count);
	}

	void WriteByte(uint8_t Data);

	inline void Write(uint8_t* Src, uint16_t Count)
	{
		while(Count--)
		this->WriteByte(*(Src++));
	}

	inline void Write(void* Src, uint16_t Count)
	{
		this->Write((uint8_t*)Src, Count);
	}

/** 
  @Function
    void Printf(const char* Fmt, ...);

  @Summary
	Write formatted data to UARTx

  @Description
	Writes the C string pointed by format to the UARTx. If format includes format specifiers (subsequences beginning with %), the additional arguments following format are formatted and inserted in the resulting string replacing their respective specifiers.

  @Precondition
	peripheral must be initialized - init();

  @Parameters
    @param Format C string that contains the text to be written to stdout. It can optionally contain embedded format specifiers that are replaced by the values specified in subsequent additional arguments and formatted as requested.
    
    @param arguments arguments corresponding to data format

  @Returns
	void

  @Remarks
	Behaves like standard Printf funcion

  @Example
    @code
UART1.Printf("Hello world\n");
 
UART1.Printf("i can format shit like %d\n", variable);

buffer[] = "can you do this?\n";
 
UART1.Printf(buffer);
 */
	void Printf(const char* Fmt, ...);

	volatile uint8_t RXBuffer[UART_FIFO_SIZE + 1];
	volatile uint16_t RX_write_index;
	volatile uint16_t RX_read_index;

	volatile uint32_t const base_adr;
};

/* **************************************************************************	*/
/* Section: class initialization according to UARTx_EN in hardware config	*/
/* **************************************************************************	*/
#ifdef UART1_EN
extern UART_pic32mx UART1;
#endif
#ifdef UART2_EN
extern UART_pic32mx UART2;
#endif

#endif	/* UART_PIC32MX_H */
