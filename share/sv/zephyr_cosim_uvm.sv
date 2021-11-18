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

	`include "zephyr_cosim_if.svh"
	`include "zephyr_cosim_services.svh"
	`include "zephyr_cosim_agent.svh"

endpackage


