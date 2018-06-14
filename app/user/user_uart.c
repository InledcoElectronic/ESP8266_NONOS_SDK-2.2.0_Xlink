/*
 * uart0.c
 *
 *  Created on: 2017��9��6��
 *      Author: liruya
 */

#include "user_uart.h"
#include "driver/uart.h"
#include "driver/uart_register.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "os_type.h"
#include "user_interface.h"

#define RX_BUFFER_SIZE		255

typedef struct{
	uint8_t buffer[RX_BUFFER_SIZE];
	uint8_t len;
}rx_buffer_t;

LOCAL rx_buffer_t rx_buffer;
LOCAL void uart_intr_handler( void *para );
LOCAL uart_rx_cb_t uart0_rx_cb;
LOCAL uart_tx_empty_cb_t uart0_tx_empty_cb;
LOCAL uart_tx_empty_cb_t uart1_tx_empty_cb;

/**
 * frame_interval: interval between two frames, unit bit
 */
void ICACHE_FLASH_ATTR uart0_init( uint32_t rate, uint32_t frame_interval )
{
	ETS_UART_INTR_ATTACH( uart_intr_handler, NULL );
	PIN_PULLUP_DIS( PERIPHS_IO_MUX_U0TXD_U );
	PIN_FUNC_SELECT( PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD );
	uart_div_modify( UART0, UART_CLK_FREQ / rate );
	WRITE_PERI_REG( UART_CONF0( UART0 ), ( PARITY_NONE << UART_PARITY_S ) |
			( STOP_BITS_ONE << UART_STOP_BIT_NUM_S ) | ( DATA_BITS_EIGHT << UART_BIT_NUM_S ) );
	//clear rx and tx fifo,not ready
	SET_PERI_REG_MASK( UART_CONF0( UART0 ), UART_RXFIFO_RST | UART_TXFIFO_RST );//RESET FIFO
	CLEAR_PERI_REG_MASK( UART_CONF0( UART0 ), UART_RXFIFO_RST | UART_TXFIFO_RST );

	WRITE_PERI_REG( UART_CONF1( UART0 ),
			( (UART_RXFIFO_FULL_THRHD & 96 ) << UART_RXFIFO_FULL_THRHD_S )
			| ( ( UART_RX_TOUT_THRHD & frame_interval ) << UART_RX_TOUT_THRHD_S )
			| UART_RX_TOUT_EN
			| ( (UART_TXFIFO_EMPTY_THRHD & 1) << UART_TXFIFO_EMPTY_THRHD_S ) );

	//clear all interrupt
	WRITE_PERI_REG( UART_INT_CLR( UART0 ), 0xFFFF );
	//enable rx_interrupt
	SET_PERI_REG_MASK( UART_INT_ENA( UART0 ),
			UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_OVF_INT_ENA | UART_RXFIFO_TOUT_INT_ENA | UART_TXFIFO_EMPTY_INT_ENA | UART_FRM_ERR_INT_ENA );
}

void ICACHE_FLASH_ATTR uart1_init( uint32_t rate )
{
	PIN_FUNC_SELECT( PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK );
	uart_div_modify( UART1, UART_CLK_FREQ / rate );
	WRITE_PERI_REG( UART_CONF0( UART1 ), ( PARITY_NONE << UART_PARITY_S ) |
			( STOP_BITS_ONE << UART_STOP_BIT_NUM_S ) | ( DATA_BITS_EIGHT << UART_BIT_NUM_S ) );
	//clear rx and tx fifo,not ready
	SET_PERI_REG_MASK( UART_CONF0( UART1 ), UART_RXFIFO_RST | UART_TXFIFO_RST );//RESET FIFO
	CLEAR_PERI_REG_MASK( UART_CONF0( UART1 ), UART_RXFIFO_RST | UART_TXFIFO_RST );
	WRITE_PERI_REG( UART_CONF1( UART1 ),
				( ( (UART_RXFIFO_FULL_THRHD & 1) << UART_RXFIFO_FULL_THRHD_S ) ) | ( (UART_TXFIFO_EMPTY_THRHD & 1) << UART_TXFIFO_EMPTY_THRHD_S ) );
	//clear all interrupt
	WRITE_PERI_REG( UART_INT_CLR( UART1 ), 0xFFFF );
	SET_PERI_REG_MASK( UART_INT_ENA( UART1 ), UART_TXFIFO_EMPTY_INT_ENA );
//	SET_PERI_REG_MASK( UART_INT_ENA( UART1 ), UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_OVF_INT_ENA);
}

void ICACHE_FLASH_ATTR uart_enable_isr ()
{
	ETS_UART_INTR_ENABLE();
}

void ICACHE_FLASH_ATTR uart_disable_isr ()
{
	ETS_UART_INTR_DISABLE();
}

STATUS uart0_send_byte_nowait( uint8_t byte )
{
	uint8 fifo_cnt = ( ( READ_PERI_REG( UART_STATUS( UART0 ) )
			>> UART_TXFIFO_CNT_S ) & UART_TXFIFO_CNT );
	if ( fifo_cnt < UART_TXFIFO_EMPTY_THRHD )
	{
		WRITE_PERI_REG( UART_FIFO( UART0 ), byte );
		return OK;
	}
	return BUSY;
}

uint8_t uart0_send_byte( uint8_t byte )
{
	while ( ( ( READ_PERI_REG( UART_STATUS( UART0 ) ) >> UART_TXFIFO_CNT_S ) & UART_TXFIFO_CNT ) >= UART_TXFIFO_EMPTY_THRHD );
	WRITE_PERI_REG( UART_FIFO( UART0 ), byte );
	return byte;
}

void ICACHE_FLASH_ATTR uart0_send_buffer( uint8_t *buf, uint32_t len )
{
	uint32_t i;
	for ( i = 0; i < len; i++ )
	{
		uart0_send_byte( *( buf + i ) );
	}
}

STATUS uart1_send_byte_nowait( uint8_t byte )
{
	uint8 fifo_cnt = ( ( READ_PERI_REG( UART_STATUS( UART1 ) ) >> UART_TXFIFO_CNT_S ) & UART_TXFIFO_CNT );
	if ( fifo_cnt < UART_TXFIFO_EMPTY_THRHD )
	{
		WRITE_PERI_REG( UART_FIFO( UART1 ), byte );
		return OK;
	}
	return BUSY;
}

uint8_t uart1_send_byte( uint8_t byte )
{
	while ( ( ( READ_PERI_REG( UART_STATUS( UART1 ) ) >> UART_TXFIFO_CNT_S ) & UART_TXFIFO_CNT ) >= UART_TXFIFO_EMPTY_THRHD );
	WRITE_PERI_REG( UART_FIFO( UART1 ), byte );
	return byte;
}

void ICACHE_FLASH_ATTR uart1_send_buffer( uint8_t *buf, uint32_t len )
{
	uint32_t i;
	for ( i = 0; i < len; i++ )
	{
		uart1_send_byte( *( buf + i ) );
	}
}

void ICACHE_FLASH_ATTR uart0_set_rx_cb( uart_rx_cb_t cb )
{
	uart0_rx_cb = cb;
}

void ICACHE_FLASH_ATTR uart0_set_tx_empty_cb( uart_tx_empty_cb_t cb )
{
	uart0_tx_empty_cb = cb;
}

void ICACHE_FLASH_ATTR uart1_set_tx_empty_cb( uart_tx_empty_cb_t cb )
{
	uart1_tx_empty_cb = cb;
}

LOCAL void uart_intr_handler( void *para )
{
	uint32 status0 = READ_PERI_REG( UART_INT_ST(UART0) );
	uint32 status1 = READ_PERI_REG( UART_INT_ST(UART1) );
	if ( UART_FRM_ERR_INT_ST == ( status0 & UART_FRM_ERR_INT_ST ) )
	{
		WRITE_PERI_REG( UART_INT_CLR(UART0), UART_FRM_ERR_INT_CLR );
	}
	else if ( UART_RXFIFO_TOUT_INT_ST == ( status0 & UART_RXFIFO_TOUT_INT_ST ) ||
			UART_RXFIFO_OVF_INT_ST == ( status0 & UART_RXFIFO_OVF_INT_ST ) ||
			UART_RXFIFO_FULL_INT_ST == ( status0 & UART_RXFIFO_FULL_INT_ST ) )
	{
		rx_buffer.len = 0;
		uint8_t len = ( READ_PERI_REG( UART_STATUS( UART0 ) )
				>> UART_RXFIFO_CNT_S ) & UART_RXFIFO_CNT;
		while ( rx_buffer.len < len )
		{
			rx_buffer.buffer[rx_buffer.len++] =
					READ_PERI_REG( UART_FIFO( UART0 ) ) & 0xFF;
		}
		WRITE_PERI_REG( UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR );
		if ( uart0_rx_cb != NULL )
		{
			uart0_rx_cb( rx_buffer.buffer, len );
		}
	}
	else if ( UART_TXFIFO_EMPTY_INT_ST == ( status0 & UART_TXFIFO_EMPTY_INT_ST ) )
	{
		CLEAR_PERI_REG_MASK( UART_INT_ENA(UART0), UART_TXFIFO_EMPTY_INT_ENA );
		WRITE_PERI_REG( UART_INT_CLR(UART0), UART_TXFIFO_EMPTY_INT_CLR );
		if ( uart0_tx_empty_cb != NULL )
		{
			uart0_tx_empty_cb();
		}
	}
	else
	{
	}

//	uart1 isr
	if ( UART_FRM_ERR_INT_ST == ( status1 & UART_FRM_ERR_INT_ST ) )
	{
		WRITE_PERI_REG( UART_INT_CLR( UART1 ), UART_FRM_ERR_INT_CLR );
	}
	else if ( UART_TXFIFO_EMPTY_INT_ST == ( status1 & UART_TXFIFO_EMPTY_INT_ST ) )
	{
		CLEAR_PERI_REG_MASK( UART_INT_ENA( UART1 ), UART_TXFIFO_EMPTY_INT_ENA );
		WRITE_PERI_REG( UART_INT_CLR( UART1 ), UART_TXFIFO_EMPTY_INT_CLR );
		if ( uart1_tx_empty_cb != NULL )
		{
			uart1_tx_empty_cb();
		}
	}
	else
	{
	}
}
