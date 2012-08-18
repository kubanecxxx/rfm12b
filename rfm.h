#ifndef __RFM_SET
#define __RFM_SET

#define wait_spi_complete() while(!(SPSR & _BV(SPIF))); bit_clr(SPSR,SPIF)

unsigned int rf_writecmd(unsigned int cmd);
void rf_init(void);
void rf_send(unsigned char data);
void rf_sleep(void);
void rf_receiver(void);
void rf_transmitter(void);
void rf_fifo_reset(void);
void rf_prepare(void);
unsigned char rf_read(void);
uint16_t rf_getStatus(void);

#endif 
