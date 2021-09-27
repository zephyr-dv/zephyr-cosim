/****************************************************************************
 * sv_smoke_tb.sv
 ****************************************************************************/

/**
 * Module: sv_smoke_tb
 * 
 * TODO: Add module documentation
 */
module sv_smoke_tb(input clock);
	import uvm_pkg::*;
	import tblink_rpc::*;
	import zephyr_cosim_uvm::*;
	
	initial begin
		automatic string zephyr_exe;
		automatic zephyr_cosim_agent cosim_agent;
		
		if (!$value$plusargs("zephyr-cosim-exe=%s", zephyr_exe)) begin
			$display("Error: missing +zephyr-cosim-exe option");
			$finish();
		end
		
		cosim_agent = new("agent", null);
	
		$display("zephyr_exe=%0s", zephyr_exe);

		$display("libpath: %0s", tblink_rpc_libpath());		
		
		run_test();
	end


endmodule


