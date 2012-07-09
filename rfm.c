#include "rfm_port.h"
#include "rfm.h"

//------------------------------------
//spi send and receive two bytes simultaneously
//-----------------------------------
unsigned int rf_writecmd(unsigned int cmd)
{
	unsigned int receive = 0;

	receive = low_level_spi_in_out(cmd);
	return receive;
}
//------------------------------------
//rfm initialization
//-----------------------------------
void rf_init(void)
{
	low_level_spi_init();
	rf_writecmd(0x80E7); //EL,EF,868band,12.0pF
    rf_writecmd(0xA640); //frequency select
//	rf_writecmd(0xA3E8); //frequency select
	rf_writecmd(0xC623); //4.8kbps
	rf_writecmd(0x94A0); //VDI,FAST,134kHz,0dBm,-103dBm
	rf_writecmd(0xC2AC); //AL,!ml,DIG,DQD4
	rf_writecmd(0xCA81); //FIFllO8,SYNC,!ff,DR
	rf_writecmd(0xCED4); //SYNC=2DD4
	rf_writecmd(0xC483); //@PWR,NO RSTRIC,!st,!fi,OE,EN
	rf_writecmd(0x9850); //!mp,90kHz,MAX OUT
	rf_writecmd(0xCC17); //OB1, COB0, LPX, Iddy, CDDITCBW0
	rf_writecmd(0xE000); //NOT USED
	rf_writecmd(0xC800); //NOT USED
	rf_writecmd(0xC040); //1.66MHz,2.2V
}
//------------------------------------
//Data send
//-----------------------------------
void rf_send(unsigned char data)
{

	low_level_wait_nirq_low();
	rf_writecmd(0xB800 + data);
}
//------------------------------------
//Sleep
//-----------------------------------
void rf_sleep(void)
{
	rf_writecmd(0x8201);
	rf_writecmd(0x0000);
}
//------------------------------------
//Transmitter
//-----------------------------------
void rf_transmitter(void)
{
	rf_writecmd(0x8239); //!er,!ebb,ET,ES,EX,!eb,!ew,DC
	rf_writecmd(0x0000);
}
//------------------------------------
//Receiver
//-----------------------------------
void rf_receiver(void)
{
	rf_writecmd(0x8299); //er,!ebb,!ET,ES,EX,!eb,!ew,DC (bug was here)
	rf_writecmd(0x0000);
}
//----------------------------------
//Buffer reset
//----------------------------------
void rf_fifo_reset()
{
	rf_writecmd(0xCA81);
	rf_writecmd(0xCA83);
}
//----------------------------------
//Pre send
//----------------------------------
void rf_prepare()
{
	rf_writecmd(0);
	rf_send(0xAA);
	rf_send(0xAA);
	rf_send(0xAA);
	rf_send(0x2D);
	rf_send(0xD4);
}
//----------------------------------
//Read
//----------------------------------
unsigned char rf_read(void)
{
	low_level_wait_ffit_high();
	return (rf_writecmd(0xB000) & 0xff);
}
