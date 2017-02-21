// Copyright (c) 2013 Qualcomm Atheros, Inc.  All rights reserved.
// $ATH_LICENSE_HW_HDR_C$
//
// DO NOT EDIT!  This file is automatically generated
//               These definitions are tied to a particular hardware layout


#ifndef _RX_SERVICE_H_
#define _RX_SERVICE_H_
#if !defined(__ASSEMBLER__)
#endif

// ################ START SUMMARY #################
//
//	Dword	Fields
//	0	service[15:0], reserved_0[31:16]
//
// ################ END SUMMARY #################

#define NUM_OF_DWORDS_RX_SERVICE 1

struct rx_service {
    volatile uint32_t service                         : 16, //[15:0]
                      reserved_0                      : 16; //[31:16]
};

/*

service
			
			Contents of received SERVICE field <legal all>

reserved_0
			
			Reserved: Generation should set the consumer shall
			ignore <legal 0>
*/


/* Description		RX_SERVICE_0_SERVICE
			
			Contents of received SERVICE field <legal all>
*/
#define RX_SERVICE_0_SERVICE_OFFSET                                  0x00000000
#define RX_SERVICE_0_SERVICE_LSB                                     0
#define RX_SERVICE_0_SERVICE_MASK                                    0x0000ffff

/* Description		RX_SERVICE_0_RESERVED_0
			
			Reserved: Generation should set the consumer shall
			ignore <legal 0>
*/
#define RX_SERVICE_0_RESERVED_0_OFFSET                               0x00000000
#define RX_SERVICE_0_RESERVED_0_LSB                                  16
#define RX_SERVICE_0_RESERVED_0_MASK                                 0xffff0000


#endif // _RX_SERVICE_H_