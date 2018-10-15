/*
  serial.c - Low level functions for sending and recieving bytes via the serial port
  Part of grbl_port_opencm3 project, derived from the Grbl work.

  Copyright (c) 2017 Angelo Di Chello
  Copyright (c) 2011-2015 Sungeun K. Jeon
  Copyright (c) 2009-2011 Simen Svale Skogsrud

  Grbl_port_opencm3 is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl_port_opencm3 is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl_port_opencm3.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "grbl.h"
#ifdef NUCLEO
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>

#endif

#ifdef USE_RX_DMA
uint8_t serial_rx_dma_data;
#endif
uint8_t serial_rx_buffer[RX_BUFFER_SIZE];
uint32_t serial_rx_buffer_head = 0;
volatile uint32_t serial_rx_buffer_tail = 0;

uint8_t serial_tx_buffer[TX_BUFFER_SIZE];
uint8_t serial_tx_buffer_head = 0;
volatile uint8_t serial_tx_buffer_tail = 0;


#ifdef ENABLE_XONXOFF
  volatile uint8_t flow_ctrl = XON_SENT; // Flow control state variable
#endif
  

// Returns the number of bytes used in the RX serial buffer.
uint8_t serial_get_rx_buffer_count()
{
  uint8_t rtail = serial_rx_buffer_tail; // Copy to limit multiple calls to volatile
  if (serial_rx_buffer_head >= rtail) { return(serial_rx_buffer_head-rtail); }
  return (RX_BUFFER_SIZE - (rtail-serial_rx_buffer_head));
}


// Returns the number of bytes used in the TX serial buffer.
// NOTE: Not used except for debugging and ensuring no TX bottlenecks.
uint8_t serial_get_tx_buffer_count()
{
  uint8_t ttail = serial_tx_buffer_tail; // Copy to limit multiple calls to volatile
  if (serial_tx_buffer_head >= ttail) { return(serial_tx_buffer_head-ttail); }
  return (TX_BUFFER_SIZE - (ttail-serial_tx_buffer_head));
}


void serial_init()
{
#ifdef NUCLEO
	/* Enable GPIOD clock for USART2. */
	rcc_periph_clock_enable(RCC_GPIOA);

	/* Enable clocks for USART2. */
	rcc_periph_clock_enable(RCC_USART2);
	
	/* Setup GPIO pins for USART2 transmit and receive. */
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, (GPIO2 | GPIO3));

	/* Setup USART2 TX pin as alternate function. */
	//gpio_set_af(GPIOA, GPIO_AF7, GPIO2 | GPIO3);
	
	/* Setup USART2 parameters. */
	// Set baud rate
	usart_set_baudrate(USART2, BAUD_RATE);
	usart_set_databits(USART2, 8);
	usart_set_stopbits(USART2, USART_STOPBITS_1);
	usart_set_mode(USART2, USART_MODE_TX_RX);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

#ifdef USE_RX_DMA
	usart_enable_rx_dma(USART2);
	serial_rx_dma_init();
#else
	usart_enable_rx_interrupt(USART2);
#endif
	/* Finally enable the USART. */
	usart_enable(USART2);
	
	nvic_enable_irq(NVIC_USART2_IRQ);
	
#else
  // Set baud rate
  #if BAUD_RATE < 57600
    uint16_t UBRR0_value = ((F_CPU / (8L * BAUD_RATE)) - 1)/2 ;
    UCSR0A &= ~(1 << U2X0); // baud doubler off  - Only needed on Uno XXX
  #else
    uint16_t UBRR0_value = ((F_CPU / (4L * BAUD_RATE)) - 1)/2;
    UCSR0A |= (1 << U2X0);  // baud doubler on for high baud rates, i.e. 115200
  #endif
  UBRR0H = UBRR0_value >> 8;
  UBRR0L = UBRR0_value;
            
  // enable rx and tx
  UCSR0B |= 1<<RXEN0;
  UCSR0B |= 1<<TXEN0;
	
  // enable interrupt on complete reception of a byte
  UCSR0B |= 1<<RXCIE0;
	  
  // defaults to 8-bit, no parity, 1 stop bit
#endif
}


// Writes one byte to the TX serial buffer. Called by main program.
// TODO: Check if we can speed this up for writing strings, rather than single bytes.
void serial_write(uint8_t data) {
  // Calculate next head
  uint8_t next_head = serial_tx_buffer_head + 1;
  if (next_head == TX_BUFFER_SIZE) { next_head = 0; }

  // Wait until there is space in the buffer
  while (next_head == serial_tx_buffer_tail) { 
    // TODO: Restructure st_prep_buffer() calls to be executed here during a long print.    
    if (sys_rt_exec_state & EXEC_RESET) { return; } // Only check for abort to avoid an endless loop.
  }

  // Store data and advance head
  serial_tx_buffer[serial_tx_buffer_head] = data;
  serial_tx_buffer_head = next_head;
#ifdef NUCLEO
	usart_enable_tx_interrupt(USART2);
#else
  // Enable Data Register Empty Interrupt to make sure tx-streaming is running
  UCSR0B |=  (1 << UDRIE0); 
#endif
}


// Fetches the first byte in the serial read buffer. Called by main program.
uint8_t serial_read()
{
  uint32_t tail = serial_rx_buffer_tail; // Temporary serial_rx_buffer_tail (to optimize for volatile)

  if (serial_rx_buffer_head == tail) {
    return SERIAL_NO_DATA;
  } else {
    uint8_t data = serial_rx_buffer[tail];

    tail++;
    if (tail == RX_BUFFER_SIZE) { tail = 0; }
    serial_rx_buffer_tail = tail;

#ifndef NUCLEO
    #ifdef ENABLE_XONXOFF
      if ((serial_get_rx_buffer_count() < RX_BUFFER_LOW) && flow_ctrl == XOFF_SENT) { 
        flow_ctrl = SEND_XON;
        UCSR0B |=  (1 << UDRIE0); // Force TX
      }
    #endif
#endif    
    return data;
  }
}

#ifdef NUCLEO
__attribute__( ( long_call, section(".ramcode") ) )
void usart2_isr(void)
{
#ifndef USE_RX_DMA
	if(((USART_SR(USART2) & USART_SR_RXNE)  & USART_CR1(USART2)) != 0)
	{		
	  uint8_t data = (uint8_t)USART_DR(USART2) & USART_DR_MASK;//usart_recv(USART2);
	  uint32_t next_head;
	  
	  // Pick off realtime command characters directly from the serial stream. These characters are
	  // not passed into the buffer, but these set system state flag bits for realtime execution.
	  switch (data) {
		case CMD_STATUS_REPORT: bit_true_atomic(sys_rt_exec_state, EXEC_STATUS_REPORT); break; // Set as true
		case CMD_CYCLE_START:   bit_true_atomic(sys_rt_exec_state, EXEC_CYCLE_START); break; // Set as true
		case CMD_FEED_HOLD:     bit_true_atomic(sys_rt_exec_state, EXEC_FEED_HOLD); break; // Set as true
		case CMD_SAFETY_DOOR:   bit_true_atomic(sys_rt_exec_state, EXEC_SAFETY_DOOR); break; // Set as true
		case CMD_RESET:         mc_reset(); break; // Call motion control reset routine.
		default: // Write character to buffer    
		  next_head = serial_rx_buffer_head + 1;
		  if (next_head == RX_BUFFER_SIZE) { next_head = 0; }
		
		  // Write data to buffer unless it is full.
		  if (next_head != serial_rx_buffer_tail) {
			serial_rx_buffer[serial_rx_buffer_head] = data;
			serial_rx_buffer_head = next_head;
		  }
	  }
	}
#endif //USE_RX_DMA
	if(((USART_SR(USART2) & USART_SR_TXE)  & USART_CR1(USART2)) != 0)
	{
		uint8_t tail = serial_tx_buffer_tail; // Temporary serial_tx_buffer_tail (to optimize for volatile)

		// Send a byte from the buffer	
		USART_DR(USART2) = (((uint16_t)serial_tx_buffer[tail]) & USART_DR_MASK);
		// Update tail position
		tail++;
		if (tail == TX_BUFFER_SIZE) { tail = 0; }
		serial_tx_buffer_tail = tail;
		// Turn off Data Register Empty Interrupt to stop tx-streaming if this concludes the transfer
		if (tail == serial_tx_buffer_head) { USART_CR1(USART2) &= ~USART_CR1_TXEIE; }
	}
}
#else
// Data Register Empty Interrupt handler
ISR(SERIAL_UDRE)
{
  uint8_t tail = serial_tx_buffer_tail; // Temporary serial_tx_buffer_tail (to optimize for volatile)
  
  #ifdef ENABLE_XONXOFF
    if (flow_ctrl == SEND_XOFF) { 
      UDR0 = XOFF_CHAR; 
      flow_ctrl = XOFF_SENT; 
    } else if (flow_ctrl == SEND_XON) { 
      UDR0 = XON_CHAR; 
      flow_ctrl = XON_SENT; 
    } else
  #endif
  { 
    // Send a byte from the buffer	
    UDR0 = serial_tx_buffer[tail];
  
    // Update tail position
    tail++;
    if (tail == TX_BUFFER_SIZE) { tail = 0; }
  
    serial_tx_buffer_tail = tail;
  }
  
  // Turn off Data Register Empty Interrupt to stop tx-streaming if this concludes the transfer
  if (tail == serial_tx_buffer_head) { UCSR0B &= ~(1 << UDRIE0); }
}



ISR(SERIAL_RX)
{
  uint8_t data = UDR0;
  uint8_t next_head;
  
  // Pick off realtime command characters directly from the serial stream. These characters are
  // not passed into the buffer, but these set system state flag bits for realtime execution.
  switch (data) {
    case CMD_STATUS_REPORT: bit_true_atomic(sys_rt_exec_state, EXEC_STATUS_REPORT); break; // Set as true
    case CMD_CYCLE_START:   bit_true_atomic(sys_rt_exec_state, EXEC_CYCLE_START); break; // Set as true
    case CMD_FEED_HOLD:     bit_true_atomic(sys_rt_exec_state, EXEC_FEED_HOLD); break; // Set as true
    case CMD_SAFETY_DOOR:   bit_true_atomic(sys_rt_exec_state, EXEC_SAFETY_DOOR); break; // Set as true
    case CMD_RESET:         mc_reset(); break; // Call motion control reset routine.
    default: // Write character to buffer    
      next_head = serial_rx_buffer_head + 1;
      if (next_head == RX_BUFFER_SIZE) { next_head = 0; }
    
      // Write data to buffer unless it is full.
      if (next_head != serial_rx_buffer_tail) {
        serial_rx_buffer[serial_rx_buffer_head] = data;
        serial_rx_buffer_head = next_head;    
        
        #ifdef ENABLE_XONXOFF
          if ((serial_get_rx_buffer_count() >= RX_BUFFER_FULL) && flow_ctrl == XON_SENT) {
            flow_ctrl = SEND_XOFF;
            UCSR0B |=  (1 << UDRIE0); // Force TX
          } 
        #endif
        
      }
      //TODO: else alarm on overflow?
  }
}
#endif

void serial_reset_read_buffer() 
{
  serial_rx_buffer_tail = serial_rx_buffer_head;

  #ifdef ENABLE_XONXOFF
    flow_ctrl = XON_SENT;
  #endif
}

#ifdef NUCLEO
#ifdef USE_RX_DMA
void serial_rx_dma_init(void)
{
	/* Enable clocks for DMA1. */
	rcc_periph_clock_enable(RCC_DMA1);

	/* Disable DMA stream and reset it. */
	dma_disable_stream(DMA1, DMA_STREAM5);
	dma_stream_reset(DMA1, DMA_STREAM5);

	/* DMA stream settings. */
	dma_set_transfer_mode(DMA1, DMA_STREAM5, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
	dma_set_memory_size(DMA1, DMA_STREAM5, DMA_SxCR_MSIZE_8BIT);
	dma_set_peripheral_size(DMA1, DMA_STREAM5, DMA_SxCR_PSIZE_8BIT);
	dma_enable_memory_increment_mode(DMA1, DMA_STREAM5);
	dma_disable_peripheral_increment_mode(DMA1, DMA_STREAM5);
	dma_enable_circular_mode(DMA1, DMA_STREAM5);
	dma_channel_select(DMA1, DMA_STREAM5, DMA_SxCR_CHSEL_4);
	dma_set_memory_burst(DMA1, DMA_STREAM5, DMA_SxCR_MBURST_SINGLE);
	dma_set_peripheral_burst(DMA1, DMA_STREAM5, DMA_SxCR_PBURST_SINGLE);
	dma_set_initial_target(DMA1, DMA_STREAM5, (uint8_t)0);
	dma_disable_double_buffer_mode(DMA1, DMA_STREAM5);
	dma_set_dma_flow_control(DMA1, DMA_STREAM5); //unsure about this
	dma_enable_direct_mode(DMA1, DMA_STREAM5);
	dma_set_peripheral_address(DMA1, DMA_STREAM5, (uint32_t)(USART2_BASE+0x04));
	dma_set_memory_address(DMA1, DMA_STREAM5, (uint32_t)(&serial_rx_dma_data));
	dma_set_number_of_data(DMA1, DMA_STREAM5, (uint16_t)1);

	/* Clear transfer complete interrupt flag */
	dma_clear_interrupt_flags(DMA1, DMA_STREAM5, DMA_TCIF);
	/* Enable Transfer Complete interrupt for real-time commands. */
	dma_enable_transfer_complete_interrupt(DMA1, DMA_STREAM5);

	/* Enable DMA stream. */
	dma_enable_stream(DMA1, DMA_STREAM5);

	nvic_enable_irq(NVIC_DMA1_STREAM5_IRQ);
}

__attribute__( ( long_call, section(".ramcode") ) )
void dma1_stream5_isr()
{
	/* Clear transfer complete interrupt flag */
	DMA_HIFCR(DMA1) = (DMA_TCIF << DMA_ISR_OFFSET(DMA_STREAM5));

    switch (serial_rx_dma_data)
    {
   		case CMD_STATUS_REPORT:
   			bit_true_atomic(sys_rt_exec_state, EXEC_STATUS_REPORT); // Set as true
   			break;
   		case CMD_CYCLE_START:
   			bit_true_atomic(sys_rt_exec_state, EXEC_CYCLE_START); // Set as true
			break;
   		case CMD_FEED_HOLD:
   			bit_true_atomic(sys_rt_exec_state, EXEC_FEED_HOLD); // Set as true
			break;
   		case CMD_SAFETY_DOOR:
   			bit_true_atomic(sys_rt_exec_state, EXEC_SAFETY_DOOR); // Set as true
			break;
   		case CMD_RESET:
   			mc_reset(); // Call motion control reset routine.
			break;
   		default:
   			serial_rx_buffer[serial_rx_buffer_head] = serial_rx_dma_data;

   			serial_rx_buffer_head++;
			if (serial_rx_buffer_head == RX_BUFFER_SIZE)
			{
				serial_rx_buffer_head = 0;
			}
   			break;
    }
}
#endif
#endif
