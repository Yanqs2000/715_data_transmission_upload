/**
 * Copyright (C) 2013 Guangzhou Tronlong Electronic Technology Co., Ltd. - www.tronlong.com
 *
 * @file ad7606.v
 *
 * @brief AD7606 data acquisition module.
 *
 * @author Tronlong <support@tronlong.com>
 *
 * @version V1.0
 *
 * @date 2022-05-12
 *
 **/
`timescale 1ns / 1ps

module ad7606 #(
	parameter		FPGA_CLOCK_FREQ		= 100,  // FPGA clock frequency input for sys_clk. [MHz]
	parameter		ADC_SAMPLING_RATE	= 20    // Set the ADC sampling rate. [kSPS]
)(
	// Module clock and reset signal
	input			sys_clk,					// Clock input
	input			rst_n,						// Reset signal input, active low

	// AD7606 control signal
	output			adc_reset,					// ADC RESET signal, active high
	output			adc_convst_a,				// ADC Conversion Start Input A
	output			adc_convst_b,				// ADC Conversion Start Input B
	output	[2:0]	adc_os,						// ADC Oversampling Mode Pins
	output			adc_range,					// ADC Analog Input Range Selection

	// AD7606 parallel data interface
	input			adc_busy,           		// ADC BUSY signal
	output			adc_cs_n,					// ADC Chip Select signal
	output			adc_rd_n,					// ADC Parallel Data Read Control Input signal
	input	[15:0]	adc_data,					// ADC parallel data bus

	// AD7606 channal data
	output 	[15:0]	adc_ch1_data_out,			// AD7606 channel V1 data
	output 	[15:0]	adc_ch2_data_out,			// AD7606 channel V2 data
	output 	[15:0]	adc_ch3_data_out,			// AD7606 channel V3 data
	output 	[15:0]	adc_ch4_data_out,			// AD7606 channel V4 data
	output 	[15:0]	adc_ch5_data_out,			// AD7606 channel V5 data
	output 	[15:0]	adc_ch6_data_out,			// AD7606 channel V6 data
	output 	[15:0]	adc_ch7_data_out,			// AD7606 channel V7 data
	output 	[15:0]	adc_ch8_data_out,			// AD7606 channel V8 data

	// Indicate 8 channel data read complete
	output 			adc_read_done
);

wire 	adc_convst;
wire 	adc_wr_n;
wire 	adc_convst_en;

assign 	adc_convst_a = adc_convst;				// Convert start signal applied to CONVST A and CONVST B simultaneously.
assign 	adc_convst_b = adc_convst;				// Convert start signal applied to CONVST A and CONVST B simultaneously.
assign 	adc_range = 1'b0;           			// adc_range = 0, analog input range is +/-5V; adc_range = 1, analog input range is +/-10V.
assign 	adc_os = 3'b000;            			// Set the oversampling ratio to 0; NO OS.
assign 	adc_reset = ~rst_n;						// Reset AD7606
assign 	adc_wr_n = 1'b1;            			// adc_wr = 1, but AD7606 does not require additional write configuration
assign 	adc_convst_en = 1'b1;       			// adc convst en, always enable for ad7606

parallel_adc_capture #(
	.FPGA_CLOCK_FREQ  	(FPGA_CLOCK_FREQ),		// FPGA clock frequency input for sys_clk. [MHz]
	.ADC_SAMPLING_RATE	(ADC_SAMPLING_RATE)		// Set the ADC sampling rate. [kSPS]
) u_parallel_adc_capture (
	.sys_clk          	(sys_clk),         		// Clock input
	.rst_n            	(rst_n),          		// Reset signal input, active low
	.adc_convst   		(adc_convst),      		// ADC Conversion Start
	.adc_busy     		(adc_busy),        		// ADC BUSY signal
	.adc_cs_n     		(adc_cs_n),        		// ADC Chip Select signal
	.adc_rd_n     		(adc_rd_n),        		// ADC Parallel Data Read Control Input signal
	.adc_wr_n     		(adc_wr_n),        		// ADC Parallel Data Write Control Input signal
	.adc_data  			(adc_data),        		// ADC parallel data bus
	.adc_ch1_data_out 	(adc_ch1_data_out),		// ADC channel V1 data
	.adc_ch2_data_out 	(adc_ch2_data_out),		// ADC channel V2 data
	.adc_ch3_data_out 	(adc_ch3_data_out),		// ADC channel V3 data
	.adc_ch4_data_out 	(adc_ch4_data_out),		// ADC channel V4 data
	.adc_ch5_data_out 	(adc_ch5_data_out),		// ADC channel V5 data
	.adc_ch6_data_out 	(adc_ch6_data_out),		// ADC channel V6 data
	.adc_ch7_data_out 	(adc_ch7_data_out),		// ADC channel V7 data
	.adc_ch8_data_out 	(adc_ch8_data_out),		// ADC channel V8 data
	.adc_convst_en    	(adc_convst_en),   		// ADC Conversion enable
	.adc_read_done    	(adc_read_done)    		// ADC read data completion flag
);

endmodule
