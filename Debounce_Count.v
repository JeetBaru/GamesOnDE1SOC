module Debounce_Count(pb0, pb1, pb2, led,clock);
input pb0, pb1, pb2;
input clock;
output reg [7:0] led;

reg [7:0] out;

wire deb1;
wire deb2;
wire deb3;

debouncer deb_int1(pb0, clock, deb1);

debouncer deb_int2(pb1, clock, deb2);

debouncer deb_int3(pb2, clock, deb3);

always @ (posedge deb1 or posedge  deb2 or posedge deb3) begin

if (deb3 == 1)begin
	led <= ~out;
end

else if (deb2 == 1 )begin
	out <= 0;
end

else if (deb1 == 1)begin
	out <= out + 1;
end
end

endmodule

module debouncer (
	input 		noisy,
	input 		clk,
	output reg 	debounced
);

	reg [7:0] shiftreg;      //shift register used to wait for stable input
	
	always @ (posedge clk) 
	begin
		shiftreg[7:0] <= {shiftreg[6:0],noisy}; //shift in the current sampled value of the noisy input
		if (shiftreg[7:0] == 8'b00000000) begin
			debounced <= 1'b1; //if all 0s, then button is being pressed
		end else if (shiftreg[7:0] == 8'b11111111) begin
			debounced <= 1'b0; //if all 1s, then button is not being pressed
		end else begin
			debounced <= debounced;
		end
	end

endmodule