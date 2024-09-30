/**
 * Copyright (C) 2013 Guangzhou Tronlong Electronic Technology Co., Ltd. - www.tronlong.com
 *
 * @file parallel_adc_capture.v
 *
 * @brief parallel adc data acquisition module for 8ch 16bit parallel adc data capture.
 *
 * @author Tronlong <support@tronlong.com>
 *
 * @version V1.0
 *
 * @date 2022-05-12
 *
 **/
`timescale 1ns / 1ps

module parallel_adc_capture #(
	parameter			FPGA_CLOCK_FREQ		= 100,	// FPGA clock frequency input for sys_clk. [MHz]
	parameter			ADC_SAMPLING_RATE	= 20  	// Set the ADC sampling rate. [kSPS]
)(
	// Module clock and reset signal
	input				sys_clk,          			// Clock input
	input				rst_n, 						// Reset signal input, active low

	// ADC control signal
	output				adc_convst,    				// ADC Conversion Start Output

	// ADC parallel data interface
	input				adc_busy,           		// ADC BUSY signal
	output				adc_cs_n,					// ADC Chip Select signal
	output				adc_rd_n,					// ADC Parallel Data Read Control Input signal
	input 				adc_wr_n,					// ADC parallel data write control input signal
	input		[15:0]	adc_data,					// ADC parallel data bus

	input				adc_convst_en,      		// ADC Conversion enable

	// ADC channal data
	output reg	[15:0]	adc_ch1_data_out,			// ADC channel V1 data
	output reg	[15:0]	adc_ch2_data_out,			// ADC channel V2 data
	output reg	[15:0]	adc_ch3_data_out,			// ADC channel V3 data
	output reg	[15:0]	adc_ch4_data_out,			// ADC channel V4 data
	output reg	[15:0]	adc_ch5_data_out,			// ADC channel V5 data
	output reg	[15:0]	adc_ch6_data_out,			// ADC channel V6 data
	output reg	[15:0]	adc_ch7_data_out,			// ADC channel V7 data
	output reg	[15:0]	adc_ch8_data_out,			// ADC channel V8 data

	// Indicate 8 channel data read complete
	output reg			adc_read_done
);

// Set this vaule to count the sampling rate for adc to convst
localparam	[31:0]	ADC_CYCLE_CNT 		= FPGA_CLOCK_FREQ * 1000000 / (ADC_SAMPLING_RATE * 1000);
// Set this vaule to count the clk rate for clk_adc_par which is 50 times of the sampling rate
localparam	[31:0]	ADC_PAR_CLK_CNT		= ADC_CYCLE_CNT / 50;

reg		[31:0]	cycle_cnt;					// Cycle time, used to count time for adc_convst sigbal
reg 			start_read_data;			// Indicate that start to read adc data
reg				clk_convst;					// Sampling rate to adc_convst_a/b
reg				clk_adc_par;				// Clock for Read ADC data
reg		[3:0]	adc_channel_read;			// Used to indicate the number of ADC channels that have been read

// Before ADC configuration, change the cs and wr pin levels at the same state
// otherwise, change the cs and rd pin levels at the same state
assign adc_cs_n = adc_convst_en ? adc_rd_n : adc_wr_n;
assign adc_rd_n = (start_read_data && (adc_channel_read < 4'd8)) ? clk_adc_par : 1'b1;	// Control adc_rd_n signal to read adc data
assign adc_convst = adc_convst_en ? clk_convst : 1'b0; 								// The converted start signal is applied to convst a, convst B, convst C and convst D

// Update the ADC timing counters count
always @ (posedge sys_clk or negedge rst_n) begin
	if (!rst_n) begin
		cycle_cnt <= ADC_CYCLE_CNT - 1'b1;
		clk_convst <= 1'b1;
		clk_adc_par <= 1'b0;
	end else if (adc_convst_en) begin
		// Update the cycle_cnt
		cycle_cnt <= cycle_cnt - 1'b1;

		// When cycle_cnt count to 0, restart cycle_cnt
		if (cycle_cnt == 32'h0)
			cycle_cnt <= ADC_CYCLE_CNT - 1'b1;

		// Generate a clk_adc_par signal for adc_cs_n/adc_rd_n to read adc data
		if (cycle_cnt % (ADC_PAR_CLK_CNT / 2) == 32'h0)
			clk_adc_par <= ~clk_adc_par;

		// Generate ADC sampling clock
		if ((cycle_cnt == ADC_CYCLE_CNT / 2) || (cycle_cnt == 32'h0))
			clk_convst <= ~clk_convst;
	end
end

// Read ADC data and separate the data of each channel
always @ (posedge clk_adc_par or negedge rst_n)begin
	if (!rst_n)begin
		adc_channel_read <= 4'd0;
		start_read_data <= 1'b0;
		adc_read_done <= 1'b0;
		adc_ch1_data_out <= 16'b0;
		adc_ch2_data_out <= 16'b0;
		adc_ch3_data_out <= 16'b0;
		adc_ch4_data_out <= 16'b0;
		adc_ch5_data_out <= 16'b0;
		adc_ch6_data_out <= 16'b0;
		adc_ch7_data_out <= 16'b0;
		adc_ch8_data_out <= 16'b0;
	end else begin
		// Start to read adc data when ADC is not busy
		if (!adc_busy && !adc_read_done)
			start_read_data <= 1'b1;

		// Start to read adc data
		if (start_read_data) begin
			adc_channel_read <= adc_channel_read + 1'b1;

			// Verify CSI function
			case (adc_channel_read)
				4'd0: begin adc_ch1_data_out <= adc_data;end
				4'd1: begin adc_ch2_data_out <= adc_data;end
				4'd2: begin adc_ch3_data_out <= adc_data;end
				4'd3: begin adc_ch4_data_out <= adc_data;end
				4'd4: begin adc_ch5_data_out <= adc_data;end
				4'd5: begin adc_ch6_data_out <= adc_data;end
				4'd6: begin adc_ch7_data_out <= adc_data;end
				4'd7: begin adc_ch8_data_out <= adc_data;end
			endcase
		end

		// When 8 channels are collected, set adc_read_done to 1
		if (adc_channel_read > 4'd7) begin
			start_read_data <= 1'b0;
			adc_channel_read <= 4'd0;
			adc_read_done <= 1'b1;
		end

		// When adc is busy, set adc_read_done to 0, to restart read data;
		if (adc_busy)
			adc_read_done <= 1'b0;
	end
end

endmodule
