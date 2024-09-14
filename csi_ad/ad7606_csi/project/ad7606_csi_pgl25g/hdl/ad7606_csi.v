/**
 * Copyright (C) 2013 Guangzhou Tronlong Electronic Technology Co., Ltd. - www.tronlong.com
 *
 * @file ad7606_csi.v
 *
 * @brief CSI sends data collected by AD7606.
 *
 * @author Tronlong <support@tronlong.com>
 *
 * @version V1.0
 *
 * @date 2023-04-18
 *
 **/
`timescale 1ns / 1ps

module ad7606_csi #(
	parameter		FPGA_CLOCK_FREQ		= 40,		// FPGA clock frequency input for sys_clk. [MHz]
	parameter		ADC_SAMPLING_RATE	= 200,		// Set the ADC sampling rate. [kSPS]
	parameter		FRAME_WIDTH			= 512,		// Frame width
	parameter		FRAME_HEIGHT		= 512		// Frame height
)(
	input			sys_clk,						// system clock 24Mhz

	// AD7606 interface
	output			adc_reset,						// ADC RESET signal, active high
	output			adc_convst_a,					// ADC Conversion Start Input A
	output			adc_convst_b,					// ADC Conversion Start Input B
	output [2:0]	adc_os,							// ADC Oversampling Mode Pins
	output			adc_range,						// ADC Analog Input Range Selection
	input			adc_busy,						// ADC BUSY signal
	output			adc_cs_n,						// ADC Chip Select signal
	output			adc_rd_n,						// ADC Parallel Data Read Control Input signal
	input [15:0]	adc_data,						// ADC parallel data bus

	// CSI interface
	output			csi_pclk,						// Parallel CSI pixel clock
	output			csi_hsync,						// Parallel CSI hsync, active high
	output			csi_vsync,						// Parallel CSI vsync, active high
	output [7:0]	csi_data						// Parallel CSI data
);

// Clock wire
wire			adc_refclk;							// ADC clock
wire			fifo_wr_clk;						// CSI FIFO write clock
wire			fifo_rd_clk;						// CSI FIFO read clock

wire			rst_n;								// Reset signal input, active low

// CSI signal
reg				fifo_wr_en;							// CSI FIFO write enable
reg [7:0]		fifo_data_in;						// CSI FIFO write data
reg [7:0]		adc_data_cnt;

// ADC signal
wire			adc_read_done;						// ADC read data completion flag
wire [15:0]		adc_ch1_data_out;					// ADC channel V1 data
wire [15:0]		adc_ch2_data_out;					// ADC channel V2 data
wire [15:0]		adc_ch3_data_out;					// ADC channel V3 data
wire [15:0]		adc_ch4_data_out;					// ADC channel V4 data
wire [15:0]		adc_ch5_data_out;					// ADC channel V5 data
wire [15:0]		adc_ch6_data_out;					// ADC channel V6 data
wire [15:0]		adc_ch7_data_out;					// ADC channel V7 data
wire [15:0]		adc_ch8_data_out;					// ADC channel V8 data

ip_clk u_ip_clk (
	.clkin1       (sys_clk),						// Input system clock 24MHz
	.clkout0      (adc_refclk),						// Output clock 40MHz
	.clkout1      (fifo_wr_clk),					// Output clock 30MHz
	.clkout2      (fifo_rd_clk),					// Output clock 60MHz
	.pll_lock	  (rst_n)    						// output, active high
);

ad7606 #(
	.FPGA_CLOCK_FREQ		(FPGA_CLOCK_FREQ),		// FPGA clock frequency input for sys_clk. [MHz]
	.ADC_SAMPLING_RATE		(ADC_SAMPLING_RATE)		// Set the ADC sampling rate. [kSPS]
) u_ad7606 (
	.sys_clk				(adc_refclk),   		// Clock input
	.rst_n					(rst_n),    			// Reset signal input, active low
	.adc_reset				(adc_reset),    		// ADC RESET signal, active high
	.adc_convst_a			(adc_convst_a), 		// ADC Conversion Start Input A
	.adc_convst_b			(adc_convst_b), 		// ADC Conversion Start Input B
	.adc_os					(adc_os),       		// ADC Oversampling Mode Pins
	.adc_range				(adc_range),    		// ADC Analog Input Range Selection
	.adc_busy				(adc_busy),     		// ADC BUSY signal
	.adc_cs_n				(adc_cs_n),     		// ADC Chip Select signal
	.adc_rd_n				(adc_rd_n),     		// ADC Parallel Data Read Control Input signal
	.adc_data				(adc_data),    			// ADC parallel data bus
	.adc_ch1_data_out		(adc_ch1_data_out),		// ADC channel V1 data
	.adc_ch2_data_out		(adc_ch2_data_out),		// ADC channel V2 data
	.adc_ch3_data_out		(adc_ch3_data_out),		// ADC channel V3 data
	.adc_ch4_data_out		(adc_ch4_data_out),		// ADC channel V4 data
	.adc_ch5_data_out		(adc_ch5_data_out),		// ADC channel V5 data
	.adc_ch6_data_out		(adc_ch6_data_out),		// ADC channel V6 data
	.adc_ch7_data_out		(adc_ch7_data_out),		// ADC channel V7 data
	.adc_ch8_data_out		(adc_ch8_data_out),		// ADC channel V8 data
	.adc_read_done			(adc_read_done)			// ADC read data completion flag
);

always @ (posedge fifo_wr_clk or negedge rst_n) begin
	if (!rst_n) begin
		fifo_data_in <= 8'h0;
		adc_data_cnt <= 8'h0;
		fifo_wr_en <= 1'b0;
	end else begin
		if (fifo_wr_en | adc_read_done) begin
			if (adc_data_cnt <= 8'd15) begin
				fifo_wr_en <= 1'b1;
				adc_data_cnt <= adc_data_cnt + 1'b1;
			end else
				fifo_wr_en <= 1'b0;

			case (adc_data_cnt)
				8'd0 : begin fifo_data_in <= adc_ch1_data_out[7:0]; end
				8'd1 : begin fifo_data_in <= adc_ch1_data_out[15:8];end
				8'd2 : begin fifo_data_in <= adc_ch2_data_out[7:0]; end
				8'd3 : begin fifo_data_in <= adc_ch2_data_out[15:8];end
				8'd4 : begin fifo_data_in <= adc_ch3_data_out[7:0]; end
				8'd5 : begin fifo_data_in <= adc_ch3_data_out[15:8];end
				8'd6 : begin fifo_data_in <= adc_ch4_data_out[7:0]; end
				8'd7 : begin fifo_data_in <= adc_ch4_data_out[15:8];end
				8'd8 : begin fifo_data_in <= adc_ch5_data_out[7:0]; end
				8'd9 : begin fifo_data_in <= adc_ch5_data_out[15:8];end
				8'd10: begin fifo_data_in <= adc_ch6_data_out[7:0]; end
				8'd11: begin fifo_data_in <= adc_ch6_data_out[15:8];end
				8'd12: begin fifo_data_in <= adc_ch7_data_out[7:0]; end
				8'd13: begin fifo_data_in <= adc_ch7_data_out[15:8];end
				8'd14: begin fifo_data_in <= adc_ch8_data_out[7:0]; end
				8'd15: begin fifo_data_in <= adc_ch8_data_out[15:8];end
				default: fifo_data_in <= 8'h0;
			endcase
		end else
			adc_data_cnt <= 8'h0;
	end
end

parallel_csi_tx #(
	.FRAME_WIDTH 	(FRAME_WIDTH),		// Frame width
	.FRAME_HEIGHT	(FRAME_HEIGHT)  	// Frame height
) u_parallel_csi_tx (
	.rst_n			(rst_n),			// Reset signal input, active low

	// FIFO interface
	.fifo_wr_clk	(fifo_wr_clk),		// FIFO write clock 30Mhz
	.fifo_rd_clk	(fifo_rd_clk),		// FIFO read clock 65Mhz
	.fifo_wr_en		(fifo_wr_en),		// FIFO write vaild
	.fifo_data_in	(fifo_data_in),		// FIFO write data

	// CSI interface
	.csi_pclk		(csi_pclk),			// Parallel CSI pixel clock
	.csi_hsync		(csi_hsync),		// Parallel CSI hsync, active high
	.csi_vsync		(csi_vsync),		// Parallel CSI vsync, active high
	.csi_data		(csi_data)			// Parallel CSI data
);

endmodule
