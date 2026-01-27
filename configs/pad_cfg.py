from x_heep_gen.pads.PadRing import *
from x_heep_gen.pads.Floorplan import *
from x_heep_gen.pads.Pin import *

import numpy as np

PAD_QTY                     = 92
DIE_WIDTH                   = 2000
DIE_HEIGHT                  = 2000
SPACE_FROM_CORNER_CELL      = 20
PITCH_BETWEEN_IO_DEFAULT    = 65
BONDPAD_MARGIN              = 24
IOCELL_MARGIN               = 95
CORE_MARGIN                 = 160



def config() -> PadRing:
    """
    Build and return the PadRing for the design, including pin definitions and pad mapping.
    For detailed documentation and usage instructions, please refer to docs/source/Configuration/PadConfiguration.md
    """

    fp_dim = FloorplanDimensions(   die_dimensions  = Dimension( height=DIE_WIDTH, width=DIE_HEIGHT ),
                                    bondpad_margin  = { Side.LEFT   : BONDPAD_MARGIN,
                                                        Side.BOTTOM : BONDPAD_MARGIN,
                                                        Side.RIGHT  : BONDPAD_MARGIN,
                                                        Side.TOP    : BONDPAD_MARGIN 
                                                    },
                                    iocell_margin   = { Side.LEFT   : IOCELL_MARGIN,
                                                        Side.BOTTOM : IOCELL_MARGIN,
                                                        Side.RIGHT  : IOCELL_MARGIN,
                                                        Side.TOP    : IOCELL_MARGIN 
                                                    },
                                    core_margin     = { Side.LEFT   : CORE_MARGIN,
                                                        Side.BOTTOM : CORE_MARGIN,
                                                        Side.RIGHT  : CORE_MARGIN,
                                                        Side.TOP    : CORE_MARGIN 
                                                    }
                                 )
    
    ##############################################
    # Define the library and technology's dimensions

    Cell.cell_bondpad_a     = Dimension(width=60, length=80, name="BPAD6A")
    Cell.cell_bondpad_d     = Dimension(width=60, length=80, name="BPADD")

    Cell.cell_iocell_d      = Dimension(width=60, length=80, name="PD")
    Cell.cell_iocell_clk    = Cell.cell_iocell_d
    Cell.cell_iocell_dVdd   = Dimension(width=60, length=80, name="PS1")
    Cell.cell_iocell_ioVdd  = Dimension(width=60, length=80, name="PS2")
    Cell.cell_iocell_ioPoc  = Dimension(width=60, length=80, name="PS3")
    Cell.cell_iocell_dVss   = Dimension(width=60, length=80, name="PS4")
    Cell.cell_iocell_ioVss  = Cell.cell_iocell_dVss

    Cell.cell_iocell_a      = Dimension(width=60, length=80, name="PA")
    Cell.cell_iocell_aVdd   = Dimension(width=60, length=80, name="PSA1")
    Cell.cell_iocell_aVss   = Dimension(width=60, length=80, name="PSA2")

    Cell.cell_aPrcut        = Dimension(width=30, length=80, name="PCA")
    Cell.cell_aPrcut        = Dimension(width=60, length=80, name="PCD")

    Cell.cell_aCorner       = Dimension(width=60, length=80, name="PCA")
    Cell.cell_dCorner       = Dimension(width=60, length=80, name="PCD")


    ##############################################
    # DEFINE ALL THE AVAILABLE PINS

    digital_pins = [
        (Input,     "clk",                  [1],  {"driven_manually": True} ),
        (Input,     "rst",                  [4],  {"active":"low", "driven_manually": True}),
        (Input,     "execute_from_flash",   [5],  {} ),
        (Output,    "exit_valid",           [6],  {} ),
        (Output,    "exit_value",           [7],  {"driven_manually": True} ),
        (Input,     "boot_select",          [8],  {} ),
        (Output,    "vco_counter_overflow", [9],  {"driven_manually": True} ),
        (Output,    "lc_dir",               [10], {"driven_manually": True} ),
        (Output,    "lc_xing",              [11], {"driven_manually": True} ),
        (Output,    "dsm_clk",              [14], {"driven_manually": True} ),
        (Input,     "dsm_in",               [15], {"driven_manually": True} ),
        (Output,    "spi_flash_cs_1",       [16], {} ),
        (Inout,     "spi_flash_sd_1",       [17], {} ),
        (Inout,     "spi_flash_sd_0",       [18], {} ),
        (Output,    "spi_flash_sck",        [19], {} ),
        (Output,    "spi_flash_cs_0",       [20], {} ),
        (Input,     "spi_slave_cs",         [52], {} ),
        (Input,     "spi_slave_sck",        [53], {} ),
        (Inout,     "spi_slave_miso",       [54], {} ),
        (Input,     "spi_slave_mosi",       [55], {} ),
        (Input,     "jtag_trst",            [56], {"active":"low"}),
        (Input,     "jtag_tms",             [57], {} ),
        (Input,     "jtag_tdi",             [58], {} ),
        (Input,     "jtag_tck",             [59], {} ),
        (Output,    "jtag_tdo",             [60], {} ),
        (Output,    "uart_tx",              [63], {} ),
        (Input,     "uart_rx",              [64], {} ),
    ]

    # Add all gpios at once
    for i in range(32):
        digital_pins.append(Inout(f"gpio_{i}", attributes={"priority": 0}))

    # Generate a pin dict with all these pins
    pin_dict = {}
    for pin in digital_pins:
        pin_dict.update({pin.name: pin})

    ##############################################
    # MAP PINS TO PADS
    # And assign them sides.
    # If you don't care about sides (i.e. just want to simulate and/or FPGA)
    # Just assign them all to the same side, like done here.

    mapping = {
        Side.TOP: [
            ["clk"],
            ["rst"],
            ["boot_select"],
            ["execute_from_flash"],
            ["jtag_tck"],
            ["jtag_tms"],
            ["jtag_trst"],
            ["jtag_tdi"],
            ["jtag_tdo"],
            ["uart_rx"],
            ["uart_tx"],
            ["exit_valid"],
            ["gpio_0"],
            ["gpio_1"],
            ["gpio_2"],
            ["gpio_3"],
            ["gpio_4"],
            ["gpio_5"],
            ["gpio_6"],
            ["gpio_7"],
            ["gpio_8"],
            ["gpio_9"],
            ["gpio_10"],
            ["gpio_11"],
            ["gpio_12"],
            ["gpio_13"],
            ["spi_flash_sck"],
            ["spi_flash_cs_0"],
            ["spi_flash_cs_1"],
            ["spi_flash_sd_0"],
            ["spi_flash_sd_1"],
            ["spi_flash_sd_2"],
            ["spi_flash_sd_3"],
            ["spi_sck"],
            ["spi_cs_0"],
            ["spi_cs_1"],
            ["spi_sd_0"],
            ["spi_sd_1"],
            ["spi_sd_2"],
            ["spi_sd_3"],
            ["spi_slave_sck", "gpio_14"],
            ["spi_slave_cs", "gpio_15"],
            ["spi_slave_miso", "gpio_16"],
            ["spi_slave_mosi", "gpio_17"],
            ["pdm2pcm_pdm", "gpio_18"],
            ["pdm2pcm_clk", "gpio_19"],
            ["i2s_sck", "gpio_20"],
            ["i2s_ws", "gpio_21"],
            ["i2s_sd", "gpio_22"],
            ["spi2_cs_0", "gpio_23"],
            ["spi2_cs_1", "gpio_24"],
            ["spi2_sck", "gpio_25"],
            ["spi2_sd_0", "gpio_26"],
            ["spi2_sd_1", "gpio_27"],
            ["spi2_sd_2", "gpio_28"],
            ["spi2_sd_3", "gpio_29"],
            ["i2c_scl", "gpio_31"],
            ["i2c_sda", "gpio_30"],
        ],
    }

    # Replace the strings for their correspinding Pin element from the pins list
    mapping = {
        side: [
            ([pin_dict[p] for p in item] if isinstance(item, list) else item)
            for item in groups
        ]
        for side, groups in mapping.items()
    }

    ##############################################
    # CREATE THE PAD RING

    pins = {}
    for ps in [digital_pins, analog_pins, supply_pins]:
        pins.update({name: cls(name, pads, attr) for (cls, name, pads, attr) in ps})

    ##############################################
    # ASSIGN SOME GPIOS HERE AND THERE

    pins["gpio_0"].pads = [49]
    pins["gpio_1"].pads = [9]
    pins["gpio_2"].pads = [10]
    pins["gpio_3"].pads = [11]
    pins["gpio_4"].pads = [14]
    pins["gpio_5"].pads = [15]
    pins["gpio_6"].pads = [16]

    # Assign a gpio to the last pad just to make sure that the mcu-gen does not die
    pins["gpio_7"].pads = [PAD_QTY]






    padring = PadRing(  floorplan_dimensions    = fp_dim,\
                        pin_list                = list(pins.values()),\
                        pad_list                = [None]*PAD_QTY 
                    )



    ##############################################
    # ASSIGN PADS TO SIDES

    assign_to_side( padring.pad_list[1                     : int(PAD_QTY/4)*1 +1], Side.LEFT )
    assign_to_side( padring.pad_list[int(PAD_QTY/4)*1 +1   : int(PAD_QTY/4)*2 +1], Side.BOTTOM )
    assign_to_side( padring.pad_list[int(PAD_QTY/4)*2 +1   : int(PAD_QTY/4)*3 +1], Side.RIGHT )
    assign_to_side( padring.pad_list[int(PAD_QTY/4)*3 +1   : int(PAD_QTY/4)*4 +1], Side.TOP )

    ##############################################
    # PLACE PHYSICAL COMPONENTS (pad ring cuts)
    # Layout indexes are counted per side,
    # starting from 0
    # clockwise (inverse wrt the global index

    prcuta_top      = Physical( name            ="PRCUTA_TOP",
                                layout          = Layout(bond_pad=PadDef.bp_skip, cell_pad=PadDef.aPrcut),
                                side            = Side.TOP,
                                orient          = Orientation.R0,
                                layout_index    = 15.5,
                                space           = 5 )

    prcuta_bottom   = Physical( name            = "PRCUTA_BOTTOM",
                                layout          = Layout(bond_pad=PadDef.bp_skip, cell_pad=PadDef.aPrcut),
                                side            = Side.BOTTOM,
                                orient          = Orientation.R180,
                                layout_index    = 9.5,
                                space           = 0 )

    pads.append(prcuta_top)
    pads.append(prcuta_bottom)

    ##############################################
    # PRINT A NICE DIAGRAM TO CHECK EVERYTHING IS OK

    print_pad_frame(pads[1:])
    print_pad_table(pads[1:])


    # Arbitrarily assign a fixed position to some pad
    padring.pad_list[31].iocell_center_to_ring_edge = 586

    ##############################################
    # MANUALLY SET SPACING
    for side in Side: padring.space_by_pitch(side, SPACE_FROM_CORNER_CELL, PITCH_BETWEEN_IO_DEFAULT )

    return padring
