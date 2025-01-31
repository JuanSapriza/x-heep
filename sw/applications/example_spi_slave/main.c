#include <stdint.h>
#include <stdlib.h>

#include "x-heep.h"
#include "spi_host.h"
#include "spi_slave_sdk.h"

/* By default, printfs are activated for FPGA and disabled for simulation. */
#define PRINTF_IN_FPGA 1
#define PRINTF_IN_SIM 0

#if TARGET_SIM && PRINTF_IN_SIM
#define PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#elif PRINTF_IN_FPGA && !TARGET_SIM
#define PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define DUMMY_CYCLES  32

#define DATA_LENGTH_B 200
uint8_t buffer_1[DATA_LENGTH_B] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 
    31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 
    61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 
    91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 
    121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 
    151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 
    181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200
};
uint8_t buffer_2[DATA_LENGTH_B];

static spi_host_t* host; 

int main(){

    host = spi_host1;
    spi_return_flags_e flags;

    // Initialize the SPI host to transmit data to the SPI slave
    if( spi_host_init(host)!= SPI_FLAG_SUCCESS) {
        PRINTF("Failure to initialize\n\r");
        return EXIT_FAILURE;
    }

    // Use the SPI Host SDK to write into the SPI slave.
    // In the SPI Slave SDK there are the needed SPI HOST functions to control the SPI Slave. 
    // The SPI slave per se cannot be controlled from X-HEEP's SW. 
    if( spi_slave_write(host, buffer_2, buffer_1, DATA_LENGTH_B) != SPI_FLAG_SUCCESS) {
        PRINTF("Failure to write\n\r");
        return EXIT_FAILURE;
    }

    // Check that the written values match the source ones. 
    // Also increment those values +1 to use as new data.
    for(int i=0; i < DATA_LENGTH_B; i++){
        if(buffer_1[i] != buffer_2[i]) return EXIT_FAILURE;
        buffer_2[i] = i+1;
    }
    PRINTF("WRITE OK\n\r");

    // Use the SPI Host SDK to read from the SPI slave and store the read values in buffer 1
    if( spi_slave_read(host, buffer_2, buffer_1, DATA_LENGTH_B, DUMMY_CYCLES) != SPI_FLAG_SUCCESS) {
        PRINTF("Failure to read\n\r");
        return EXIT_FAILURE;
    }

    // check that the new values still match
    for(int i=0; i < DATA_LENGTH_B; i++){
        if(buffer_1[i] != buffer_2[i]) return EXIT_FAILURE;
    }
    PRINTF("READ OK\n\r");

    return EXIT_SUCCESS;
}





