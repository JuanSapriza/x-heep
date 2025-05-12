// Copyright 2024 EPFL and Politecnico di Torino
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: example_dlc_spi.c
// Author: Juan Sapriza
// Date: 12/05/2025
// Description: Example application to test the digital Level Crossing (dLC) IP
//              along with the SPI Host and slave, and DMA Hardware Fifo Mode.

#include <stdio.h>
#include <stdlib.h>

#include "dma.h"
#include "core_v_mini_mcu.h"
#include "x-heep.h"
#include "csr.h"
#include "dlc.h"
#include "rv_plic.h"
#include "test_ecg.h"

#include "spi_host_regs.h"
#include "spi_host.h"
#include "spi_slave_sdk.h"
#include "gpio.h"
#include "hart.h"
#include "timer_sdk.h"

#define PRINTF_IN_SIM 0
#define PRINTF_IN_FPGA 1

#if TARGET_SIM && PRINTF_IN_SIM
        #define PRINTF(fmt, ...)    printf(fmt, ## __VA_ARGS__)
#elif PRINTF_IN_FPGA && !TARGET_SIM
    #define PRINTF(fmt, ...)    printf(fmt, ## __VA_ARGS__)
#else
    #define PRINTF(...)
#endif

#define DLC_START_ADDRESS PERIPHERAL_START_ADDRESS + 0x00080000



#if TARGET_SIM
#define SYNC_LOGIC(x) ~x
#else
#define SYNC_LOGIC(x) x
#endif

#define GPIO_LD5_R  11
#define GPIO_LD5_B  12
#define GPIO_LD5_G  13

#define DUMMY_CYCLES  255 // The maximum number of dummy cycles, just to make things slower

#define FULL_ECG 1

#if FULL_ECG
    #define DATA_LENGTH_B   sizeof(SOURCE_DATA) 
#else
    #define DATA_LENGTH_B   256
#endif

#define DATA_CHUNK_W    1
#define DATA_CHUNK_B    1
#define CHUNKS_NW       (DATA_LENGTH_B/(DATA_CHUNK_W*4)) + ((DATA_LENGTH_B%(DATA_CHUNK_W*4))!=0)
#define CHUNKS_NB       (DATA_LENGTH_B/DATA_CHUNK_B)


#define SOURCE_DATA ecg_data
spi_host_t* spi_device = spi_host1;
uint8_t src_slot = DMA_TRIG_SLOT_SPI_RX;

dma_target_t tgt_src;
dma_target_t tgt_dst;
dma_trans_t trans;


void __attribute__((aligned(4), interrupt)) handler_irq_timer(void) {
    timer_arm_stop();
    timer_irq_clear();
    return;   
}

int main() {
    CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
    CSR_SET_BITS(CSR_REG_MIE, (1 << 11) | (1 << 19));

    // dLC results buffer
    int16_t dlc_results[500];
  
/*############################################################
####### SET THE DIGITAL LC POINTERS #######################*/

    // dLC programming registers
    uint32_t* dlvl_log_level_width    = DLC_START_ADDRESS + DLC_DLVL_LOG_LEVEL_WIDTH_REG_OFFSET;
    uint32_t* dlvl_n_bits             = DLC_START_ADDRESS + DLC_DLVL_N_BITS_REG_OFFSET;
    uint32_t* dlvl_format             = DLC_START_ADDRESS + DLC_DLVL_FORMAT_REG_OFFSET;
    uint32_t* dlvl_mask               = DLC_START_ADDRESS + DLC_DLVL_MASK_REG_OFFSET;
    uint32_t* dt_mask                 = DLC_START_ADDRESS + DLC_DT_MASK_REG_OFFSET;
    uint32_t* dlc_rnw                 = DLC_START_ADDRESS + DLC_READNOTWRITE_REG_OFFSET;
    
/*############################################################
####### SET THE DIGITAL LC PARAMETERS ######################*/

    // dLC programming
    // dlvl_format: if set to '1' the result data for delta-levels are in two's complement format
    //              if set to '0' the result data for delta-levels are in sign and modulo format
    *dlvl_format = LC_PARAMS_DATA_IN_TWOS_COMPLEMENT;
    // dlvl_log_level_width: log2 of the delta-levels width
    *dlvl_log_level_width = LC_PARAMS_LC_LEVEL_WIDTH_BY_BITS;
    // dlvl_n_bits: number of bits for the delta-levels field
    //              if dlvl_format is set to '1' the number of bits for the delta-levels is dlvl_n_bits
    //              if dlvl_format is set to '0' the number of bits for the delta-levels is dlvl_n_bits - 1 to account for the sign bit 
    *dlvl_n_bits = (LC_PARAMS_DATA_IN_TWOS_COMPLEMENT) ? LC_PARAMS_LC_ACQUISITION_WORD_SIZE_OF_AMPLITUDE:
                        LC_PARAMS_LC_ACQUISITION_WORD_SIZE_OF_AMPLITUDE - 1;
    // dlvl_mask: mask for the delta-levels field (it has as many bits set to 1 as the number of bits for the delta-levels field)
    *dlvl_mask = (1 << (*dlvl_n_bits)) - 1;
    // dt_mask: mask for the delta-time field (it has as many bits set to 1 as the number of bits for the delta-time field)
    *dt_mask = (1 << (LC_PARAMS_LC_ACQUISITION_WORD_SIZE_OF_TIME)) - 1; 
    // dlc_rnw: if set to '1' the dLC decrements DMA downcounter each time it reads data from the HW_READ_FIFO
    //          if set to '0' the dLC decrements DMA downcounter each time it write data to the HW_WRITE_FIFO
    *dlc_rnw = 1;

    PRINTF("Set the dLC to: \n\r2sComp:\t%d\n\rLVLw:\t%d bits\n\r",*dlvl_format, *dlvl_log_level_width );

/*############################################################
####### CONFIGURE GPIOS TO AS LEDs #####################*/

    uint8_t synq;

    // Configure the pynq's internal LEDs to show which is the slave
    // and which the master.  
    gpio_cfg_t pin_cfg = {
    .pin = GPIO_LD5_R,
    .mode = GpioModeOutPushPull,
    .en_input_sampling = true,
    .en_intr = false,
    };
    gpio_config(pin_cfg);
    pin_cfg.pin     = GPIO_LD5_B;
    gpio_config(pin_cfg);
	pin_cfg.pin     = GPIO_LD5_G;
    gpio_config(pin_cfg);
    // Start all LEDs off. 
    gpio_write(GPIO_LD5_R, false);
    gpio_write(GPIO_LD5_B, false);
    gpio_write(GPIO_LD5_G, false);


    PRINTF("Configured LEDs\n\r");

/*############################################################
####### CONFIGURE THE DMA #################################*/

    // Set the source target (where data is taken from) to the Rx fifo of the SPI
    tgt_src.ptr = (uint8_t *) (uint32_t *)((uintptr_t)spi_device + SPI_HOST_RXDATA_REG_OFFSET);
    // Select the appropriate slot (depending on which spi_device is being used)
    tgt_src.trig = src_slot;   
    // Because the data is always taken from the same register, there should be no increment 
    tgt_src.inc_d1_du = 0;
    // We will copy data in chunks of 16-bits, the width of the ECG data used
    tgt_src.type = DMA_DATA_TYPE_WORD;
    
    // After passing through the dLC, the data will be stored in a separate buffer.
    tgt_dst.ptr = (uint8_t *) dlc_results;
    // These data we will store in different places in memory, so the increment should be 1 data unit (du) 
    tgt_dst.inc_d1_du = 1;
    // We have nothing to mark the pace for the acquisition, so the slot will be simply the memory grants
    tgt_dst.trig = DMA_TRIG_MEMORY;
    // We will still copy in chuncks of 16-bits for debugging
    tgt_dst.type = DMA_DATA_TYPE_HALF_WORD;

    // Set the transaction
    trans.src        = &tgt_src;
    trans.dst        = &tgt_dst;
    // Specify that we will use the HW FIFO mode: all data read will be forwarded to the 
    // stream peripheral that is connected to the hw fifo. 
    trans.mode       = DMA_TRANS_MODE_HW_FIFO;
    // Set that this will be a 1-Dimensional data transfer
    trans.dim        = DMA_DIM_CONF_1D;
    
    // Set the size of the transaction. This is the maximum amount of data that should be written.
    // We will set it to a low value just to monitor the behavior. 
    trans.size_d1_du = DATA_LENGTH_B/DMA_DATA_TYPE_2_SIZE(tgt_src.type);
    
    // Specify that we will have the CPU checking the status of the DMA constantly 
    trans.end        = DMA_TRANS_END_INTR;
    //@ToDo: Set this as a circular transfer with double buffering.  

    // Init the DMA (NULL because we will use the internal dma #0)
    dma_init(NULL);

    // Do some sanity checks to make sure that the entered values are valid
    dma_config_flags_t res;
    res = dma_validate_transaction(&trans, DMA_ENABLE_REALIGN, DMA_PERFORM_CHECKS_INTEGRITY);
    if( res != DMA_CONFIG_OK ){
        PRINTF("Error: dma_validate_transaction: %d\n",res );
        return EXIT_FAILURE;
    }
    // Load the values into the DMA registers.
    res = dma_load_transaction(&trans);
    if( res != DMA_CONFIG_OK ) {
        PRINTF("Error: dma_load_transaction: %d\n", res);
        return EXIT_FAILURE;
    }
    
    PRINTF("Cofigured DMA\n\r");

/*############################################################
####### LAUNCH THE DMA #####################################*/
    
    // Launch the DMA transaction. As the DMA will be waiting at the SPI slot, no transaction will be done yet
    if(dma_launch(&trans) != DMA_CONFIG_OK){
        PRINTF("Error: dma_launch\n");
        return EXIT_FAILURE;
    }

    // PRINTF("Launched DMA\n\r");


    #if !TARGET_SIM
    // Enable the timer interrupts to go to sleep between packets. 
    enable_timer_interrupt();
    // Wait for a while just for the lols
    timer_wait_us(1000000);
    #endif

    

/*############################################################
####### CONFIGURE THE SPI TRANSFER ##########################*/

    // Initilize the SPI host IP
    if( spi_host_init(spi_device, 0)!= SPI_FLAG_SUCCESS) return EXIT_FAILURE;

    PRINTF("Will start requesting data through SPI\n\r");
    gpio_write(GPIO_LD5_R, true);

    if( FULL_ECG ){
        spi_slave_request_read(spi_device, SOURCE_DATA,  DATA_LENGTH_B, DUMMY_CYCLES );
    } else{
        for( uint16_t i=0; i<DATA_LENGTH_B; i+=4){
            spi_slave_request_read(spi_device,&SOURCE_DATA[i],  4, DUMMY_CYCLES );
            // spi_wait_for_rx_watermark(spi_device);
            // The DMA will take care of taking the data from the SPI host to the dLC and then to memory
            // buffer_read_to[i] = spi_copy_byte(spi_device, i%4 );
            gpio_toggle(GPIO_LD5_G);
            
            // We will simulate a 20 Hz sampling rate by delaying each SPI data request. This is transparent to the 
            // DMA, which is only waiting for data to be available at the SPI host Rx fifo.
            #if !TARGET_SIM
            timer_wait_us(50000);
            #endif
        }
    }
    
    // Celebrate in a fairly lame way
    PRINTF("Requested data!\n\r");
    gpio_write(GPIO_LD5_G,  true);
    gpio_write(GPIO_LD5_R,  false);


/*############################################################
####### WAIT FOR THE DMA TO FINISH ########################*/
    while(!dma_is_ready(0)) {       
        CSR_CLEAR_BITS(CSR_REG_MSTATUS, 0x8);
        if ( dma_is_ready(0) == 0 ) {
                wait_for_interrupt();
            }
            CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
    }
        
    // Celebrate in a fairly lame way
    PRINTF("DMA done!\n\r");
    gpio_write(GPIO_LD5_G,  false);
    gpio_write(GPIO_LD5_B,  true);    

/*############################################################
####### CHECK THE RESULTS ###################################*/    

    // Checking  the results
    PRINTF("\n\rRES\t| dLC\t| Golden");
    uint16_t errors = 0;
    for (int i = 0; i < LC_STATS_CROSSINGS; i++)
    {
        if(dlc_results[i] != lc_data_for_storage_data[i])
        {
            printf("\n\rX %d\t| %d\t| %d", i, dlc_results[i], lc_data_for_storage_data[i]);
            errors++;
        }
    }
    if( errors ){
        PRINTF("\n\r=====================\n ERRORS: %d\n", errors);
        return EXIT_FAILURE;
    } else {
        PRINTF("\n\r ALL GOOD!\n");
        return EXIT_SUCCESS;
    }


}
