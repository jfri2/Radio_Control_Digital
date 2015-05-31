/*
 * Digital_Channel_Select.c
 *
 * Created: 4/15/2015 10:36:06 AM
 *  Author: John
 */ 

#define F_CPU 16000000L
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
//#include "serial.c"	
#include "three_aray.h"

//SPI Definition Macro
#define SCK 5
#define MOSI 3
//#define MISO 4		//not used as this SPI DAC does not output anything
#define CS 2
#define LDAC 0

//Function prototypes
void spi_master_init(void);
uint8_t spi_master_send_recieve(uint8_t);
void mcp4921_spi_send(uint16_t);

/****** General Device and SPI Bus Information *****

	Set LDACn high	//MCP4921 Vout stops being updated
	Hold CSn high	//MCP4921 is not receiving data
	CSn low			//MCP4921 is ready to receive data
	Send 4 config bits and 12 data bits to SDI pin on rising edge of SCK	//data is temporarly stored on dac
	Set CSn high	//data is latched into dac input register
	Set LDACn low	//Vout is updated to current value
	Set LDACn high	//Vout stops being updated
	
	
	All 16 clock pulses are needed to write data
	
	
	Format for writing to DAC and outputting
	
	bit:	15		14		13		12		11		...		0
			wr/i	buf		ga		shdn	data11	data	data0	
			
			wr/i = 0, wright to dac register
				 = 1, ignore this command
			buf  = 0, input unbuffered
				 = 1, input buffered
			ga	 = 0, gain of x2
				 = 1, gain of x1
			shdn = 0, shutdown device, analog output not available, Vout connected to 500ohm
				 = 1, active operation, Vout available
				 
			data11 = MSB
			data0  = LSB
			
	For this application: we will make the first four bits 0011
			
	PB5: SCK
	PB4: MISO
	PB3: MOSI
	PB2: SS
	
	PB0: LDAC
	
	SPCR = SPI Control Register	
	Contains bits to init SPI and control it
	7		6		5		4		3		2		1		0
	SPIE	SPE		DORD	MSTR	CPOL	CPHA	SPR1	SPR0
	
	SPIE = SPI Interrupt (needs global interrupts enabled to function), set to 1 to enable SPI interrupts
	SPE  = SPI Enable, set to 1 to enable SPI as a whole, when enabled, normal I/O of pin is overridden
	DORD = Data Order, set to 1 to transmit LSB first, set to 0 to transmit MSB first
	MSTR = Master/Slave select, set to 1 for master mode, set to 0 for slave mode
	CPOL = Clock polarity, selects clock polarity when bus is idle, set to 1 to make SCK high when idle, 0 to make SCK low if idle
	CPHA = Clock Phase, set to 1 to sample data at leading edge of SCK, 0 for falling edge of SCK.
	SPR1, SPR0 = SPI Clock Rate select, used along with SPI2x in SPSR register, choose osc frequency divider.
	
	SPSR = SPI Status Register
	Register used to read the status of the bus lines
	7		6		5		4		3		2		1		0
	SPIF	WCOL	RSVRD	RSVRD	RSVRD	RSVRD	RSVRD	SPI2x
	
	SPIF = SPI Interrupt Flag, Set when serial transfer complete or SPI interrupt (SPIE enabled), cleared when corresponding ISR is executed
	WCOL = Write Collision Flag, Set when data is written on the SPI data register when there is an impending transfer or lines are busy. Cleared by reading SPI data register when WCOL is set. This error does not occur if we are reading lines right
	SPI2x = double speed mode that reduces frequency divider from 4x to 2x 
	
	SPDR = SPI Data Register
	Used to read/write the data, data transfer takes place here	
	7		6		5		4		3		2		1		0
	MSB		d6		d5		d4		d3		d2		d1		LSB
	
	
	CS/SS = Chip/Slave Select, Active LOW
	
	
	
*/


int main(void)
{
	spi_master_init();			//init spi
    while(1)
    {

    }
}

void spi_master_init(void)
{
	DDRB = (1 << MOSI)|(1 << SCK)|(1 << LDAC);		//sets mosi, sck, and ldac bits as outputs	
	SPCR = (1 << SPE)|(1 << MSTR)|(1 << SPR1);		//enable spi, set as master. Prescalar = clk/16 (16 MHz clk -> 250 KHz)
}

uint8_t spi_master_send_recieve(uint8_t data)
{
	uint8_t return_data = 0;
	SPDR = data;
	while(!(SPSR & (1 << SPIF)));		//wait until transmission is complete
	return(return_data);
}

void mcp4921_spi_send(uint16_t voltage)
{
	uint8_t upper_byte;
	uint8_t lower_byte;
	
	upper_byte = (voltage >> 8);		
	upper_byte = upper_byte & 0x0F;
	upper_byte = upper_byte | 0b00110000;	//adds control values
	
	lower_byte = (voltage & 0x00FF);
	
	//make sure ldac is high
	//send data (cs goes high and low here)
	//drive ldac low for minimum 60ns (tls = 40ns min, tld = 100ns min)
	//set ldac high	
	
	if (~(PORTB && (PORTB | 0x01)))		//if ldac is not high
	{
		PORTB = PORTB | (1 << 0);			//set ldac high
	}
	
	spi_master_send_recieve(upper_byte);	//send upper byte of data, no data to receive
	spi_master_send_recieve(lower_byte);	//send lower byte of data, no data to receive
	
	PORTB = PORTB & ~(1 << 0);			//set ldac low
	_delay_us(10);						//but only for a ten microseconds
	PORTB = PORTB | (1 << 0);			//set ldac high
}

