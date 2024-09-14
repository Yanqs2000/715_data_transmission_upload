/**
 * Copyright (C) 2013 Guangzhou Tronlong Electronic Technology Co., Ltd. - www.tronlong.com
 *
 * @file parallel_csi_tx.v
 *
 * @brief Write test data to fifo and read out, then transfer it through parallel CSI.
 *
 * @author Tronlong <support@tronlong.com>
 *
 * @version V1.0
 *
 * @date 2023-01-13
 *
 **/
`timescale 1ns / 1ps

module parallel_csi_tx #(
	parameter			FRAME_WIDTH  	= 1024,	// Frame width
	parameter			FRAME_HEIGHT	= 512	// Frame height
)(
	input				rst_n,					// Reset signal input, active low

	// FIFO
	input				fifo_wr_clk,			// FIFO write clock 30Mhz
	input				fifo_rd_clk,			// FIFO read clock 65Mhz
	input				fifo_wr_en,				// FIFO write vaild
	input 		[7:0]	fifo_data_in,			// FIFO write data

	// CSI
	output				csi_pclk,				// Parallel CSI pixel clock
	output				csi_hsync,				// Parallel CSI hsync, active high
	output				csi_vsync,				// Parallel CSI vsync, active high
	output reg [7:0]	csi_data				// Parallel CSI data
);

// FIFO signal
reg				fifo_rd_en;						// FIFO read enable signal
wire	[7:0]	fifo_data_out;					// FIFO data out
wire			fifo_full;						// FIFO full signal
wire			fifo_almost_full;				// FIFO program full signal

// Parallel CSI signal
reg				hsync;							// Hsync, active high
reg				vsync;							// Vsync, active high
reg		[15:0]	hs_cnt;							// Count the hsync signal, mean that how many rows were sent
reg		[15:0]	vblank_cnt;						// Count the clock period that vsync needs to pull down
reg		[15:0]	line_data_cnt;					// Count how many data was sent, used to determine whether one line has been sent

// FIFO ip core
ip_fifo u_ip_fifo (
	.wr_data			(fifo_data_in),			// Write data
	.wr_en				(fifo_wr_en),			// Write vaild
	.wr_clk				(fifo_wr_clk),			// Write clock
	.wr_rst				(~rst_n),				// Write reset, active high
	.full				(fifo_full),			// Data full
	.almost_full		(fifo_almost_full),		// Data almost full
	.rd_data			(fifo_data_out),		// Read data
	.rd_en				(fifo_rd_en),			// Read vaild
	.rd_clk				(fifo_rd_clk),			// Read clock
	.rd_rst				(~rst_n)				// Read reset, active high
);

assign csi_pclk = fifo_rd_clk;

// Read data from fifo and tx by parallel csi
always @ (negedge csi_pclk or negedge rst_n) begin
	if (~rst_n) begin
		hs_cnt <= 16'h0;
		line_data_cnt <= 16'h0;
		vblank_cnt <= 16'h0;
		fifo_rd_en <= 1'b0;
		hsync <= 1'b0;
		vsync <= 1'b0;
		csi_data <= 8'h0;
	end else begin
		// Keep vsync low level 10 clk period
		if (vblank_cnt < 16'd10) begin
			vblank_cnt <= vblank_cnt + 1'b1;
			vsync <= 1'b0;
		end else begin
			csi_data <= fifo_data_out;	// Output fifo_data_out to csi_data

			// IF fifo_almost_full high and fifo is not reading, enable fifo read
			if (fifo_almost_full == 1'b1 && fifo_rd_en == 1'b0) begin
				fifo_rd_en <= 1'b1;
			end

			// Read data and calculate how many line has been sent
			if (fifo_rd_en == 1'b1) begin
				hsync <= 1'b1;
				if (line_data_cnt == FRAME_WIDTH - 1'b1) begin
					line_data_cnt <= 16'h0;					// IF one line have sent, restart count
					fifo_rd_en <= 1'b0;						// IF one line have sent, disable fifo read, wait fifo_almost_full again
					hs_cnt <= hs_cnt + 1'b1;				// IF one line have sent, hs_cnt +1, used to judge whether one frame has been sent
				end else
					line_data_cnt <= line_data_cnt + 1'b1;	// Count how many data was sent, used to judge whether one line has been sent
			end else
				hsync <= 1'b0;

			// Judge whether one frame has been sent
			if (hs_cnt == FRAME_HEIGHT) begin
				hs_cnt <= 16'h0;
				line_data_cnt <= 16'h0;
				vblank_cnt <= 16'h0;
				fifo_rd_en <= 1'b0;
				hsync <= 1'b0;
			end else
				vsync <= 1'b1;		// If one frame is not sent done, keep vsync high
		end
	end
end

assign csi_hsync = hsync;		// Output hsync
assign csi_vsync = vsync;		// Output vsync

endmodule
