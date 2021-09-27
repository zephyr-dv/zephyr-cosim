/****************************************************************************
 * zephyr_cosim_uvm.sv
 * 
 * Package provides SystemVerilog interface to Zephyr-Cosim
 ****************************************************************************/
`include "uvm_macros.svh"
  
/**
 * Package: zephyr_cosim_uvm
 * 
 * TODO: Add package documentation
 */
package zephyr_cosim_uvm;
	import uvm_pkg::*;
	import tblink_rpc::*;
	import tblink_rpc_uvm::*;

	class zephyr_cosim_agent extends uvm_component;
		`uvm_component_utils(zephyr_cosim_agent)
		
		chandle				m_endpoint;
		
		function new(string name, uvm_component parent);
			super.new(name, parent);
		endfunction
		
		function void build_phase(uvm_phase phase);
			string zephyr_exe;
			string tblink_libpath;
			
			$display("build_phase");
			if (!$value$plusargs("zephyr-cosim-exe=%s", zephyr_exe)) begin
				$display("Error: missing +zephyr-cosim-exe option");
				$finish();
			end
			
			tblink_libpath = tblink_rpc_libpath();
			
			begin
				chandle launch_type = tblink_rpc_findLaunchType("process.socket");
				chandle launch_params = tblink_rpc_newLaunchParams();
				string errmsg;
				
				tblink_rpc_ILaunchParams_add_arg(launch_params, zephyr_exe);
				tblink_rpc_ILaunchParams_add_arg(launch_params, tblink_libpath);
			
				m_endpoint = tblink_rpc_ILaunchType_launch(
						launch_type,
						launch_params,
						errmsg);
				
				if (m_endpoint == null) begin
					$display("Failed to launch: %0s", errmsg);
				end else begin
					$display("Launch succeeded");
				end
				
				if (tblink_rpc_IEndpoint_build_complete(m_endpoint) == -1) begin
					$display("Error: build_complete failed");
					$finish();
				end
				
//				while (tblink_rpc_IEndpoint_is_build_complete(m_endpoint) == -1) begin
			end
			
		endfunction
		
		function void connect_phase(uvm_phase phase);
		endfunction
		
		task run_phase(uvm_phase phase);
		endtask
		
	endclass

endpackage


