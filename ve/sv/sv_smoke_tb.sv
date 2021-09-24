/****************************************************************************
 * sv_smoke_tb.sv
 ****************************************************************************/

/**
 * Module: sv_smoke_tb
 * 
 * TODO: Add module documentation
 */
module sv_smoke_tb(input clock);
	import tblink_rpc::*;
	
	initial begin
		automatic string zephyr_exe;
		
		if (!$value$plusargs("zephyr-cosim-exe=%s", zephyr_exe)) begin
			$display("Error: missing +zephyr-cosim-exe option");
			$finish();
		end
	
		$display("zephyr_exe=%0s", zephyr_exe);

		$display("libpath: %0s", tblink_rpc_libpath());		
	end


endmodule


