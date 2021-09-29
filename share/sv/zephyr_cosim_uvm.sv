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
	
	typedef class zephyr_cosim_agent;

	/**
	 * Class: zephyr_cosim_if
	 *
	 * InterfaceImpl class interfaces to tblink-rpc
	 */
	class zephyr_cosim_if extends IInterfaceImpl;
		zephyr_cosim_agent			m_agent;
		
		typedef enum {
			sys_read8 = 1,
			sys_write8,
			sys_read16,
			sys_write16,
			sys_read32,
			sys_write32
		} method_id_t;
		
		function new(zephyr_cosim_agent agent);
			m_agent = agent;
		endfunction
		
		virtual task invoke_b(input InvokeInfo ii);
			chandle ret = null;
			chandle method_t = tblink_rpc_InvokeInfo_method(ii.m_hndl);
			longint unsigned id = tblink_rpc_IMethodType_id(method_t);
			$display("Invoke %0d", id);
			
			case (id)
				sys_read8: begin
					$display("TODO: read8");
				end
				sys_write8: begin
					$display("TODO: write8");
				end
				sys_read16: begin
					$display("TODO: read16");
				end
				sys_write16: begin
					$display("TODO: write16");
				end
				sys_read32: begin
					$display("TODO: read32");
				end
				sys_write32: begin
					$display("TODO: write32");
				end
			endcase
			
			tblink_rpc_InvokeInfo_invoke_rsp(
					ii.m_hndl,
					ret);
		endtask
		
		static function chandle registerType(chandle ep);
			chandle iftype_h = tblink_rpc_IEndpoint_findInterfaceType(ep, "zephyr_cosim_if");
			
			if (iftype_h == null) begin
				chandle method_b;
				chandle iftype_b = tblink_rpc_IEndpoint_newInterfaceTypeBuilder(
						ep,
						"zephyr_cosim_if");

				// read8
				method_b = tblink_rpc_IInterfaceTypeBuilder_newMethodTypeBuilder(
						iftype_b,
						"sys_read8",
						sys_read8,
						tblink_rpc_IInterfaceTypeBuilder_mkTypeInt(iftype_b, 0, 8),
						1,
						1);
				tblink_rpc_IMethodTypeBuilder_add_param(method_b, "addr",
						tblink_rpc_IInterfaceTypeBuilder_mkTypeInt(iftype_b, 0, 64));
				void'(tblink_rpc_IInterfaceTypeBuilder_add_method(iftype_b, method_b));
				
				// write8
				method_b = tblink_rpc_IInterfaceTypeBuilder_newMethodTypeBuilder(
						iftype_b,
						"sys_write8",
						sys_write8,
						null,
						1,
						1);
				tblink_rpc_IMethodTypeBuilder_add_param(method_b, "data",
						tblink_rpc_IInterfaceTypeBuilder_mkTypeInt(iftype_b, 0, 8));
				tblink_rpc_IMethodTypeBuilder_add_param(method_b, "addr",
						tblink_rpc_IInterfaceTypeBuilder_mkTypeInt(iftype_b, 0, 64));
				void'(tblink_rpc_IInterfaceTypeBuilder_add_method(iftype_b, method_b));

				// read16
				method_b = tblink_rpc_IInterfaceTypeBuilder_newMethodTypeBuilder(
						iftype_b,
						"sys_read16",
						sys_read16,
						tblink_rpc_IInterfaceTypeBuilder_mkTypeInt(iftype_b, 0, 16),
						1,
						1);
				tblink_rpc_IMethodTypeBuilder_add_param(method_b, "addr",
						tblink_rpc_IInterfaceTypeBuilder_mkTypeInt(iftype_b, 0, 64));
				void'(tblink_rpc_IInterfaceTypeBuilder_add_method(iftype_b, method_b));
				
				// write16
				method_b = tblink_rpc_IInterfaceTypeBuilder_newMethodTypeBuilder(
						iftype_b,
						"sys_write16",
						sys_write16,
						null,
						1,
						1);
				tblink_rpc_IMethodTypeBuilder_add_param(method_b, "data",
						tblink_rpc_IInterfaceTypeBuilder_mkTypeInt(iftype_b, 0, 16));
				tblink_rpc_IMethodTypeBuilder_add_param(method_b, "addr",
						tblink_rpc_IInterfaceTypeBuilder_mkTypeInt(iftype_b, 0, 64));
				void'(tblink_rpc_IInterfaceTypeBuilder_add_method(iftype_b, method_b));
				
				// read32
				method_b = tblink_rpc_IInterfaceTypeBuilder_newMethodTypeBuilder(
						iftype_b,
						"sys_read32",
						sys_read32,
						tblink_rpc_IInterfaceTypeBuilder_mkTypeInt(iftype_b, 0, 32),
						1,
						1);
				tblink_rpc_IMethodTypeBuilder_add_param(method_b, "addr",
						tblink_rpc_IInterfaceTypeBuilder_mkTypeInt(iftype_b, 0, 64));
				void'(tblink_rpc_IInterfaceTypeBuilder_add_method(iftype_b, method_b));
				
				// write32
				method_b = tblink_rpc_IInterfaceTypeBuilder_newMethodTypeBuilder(
						iftype_b,
						"sys_write32",
						sys_write32,
						null,
						1,
						1);
				tblink_rpc_IMethodTypeBuilder_add_param(method_b, "data",
						tblink_rpc_IInterfaceTypeBuilder_mkTypeInt(iftype_b, 0, 32));
				tblink_rpc_IMethodTypeBuilder_add_param(method_b, "addr",
						tblink_rpc_IInterfaceTypeBuilder_mkTypeInt(iftype_b, 0, 64));
				void'(tblink_rpc_IInterfaceTypeBuilder_add_method(iftype_b, method_b));				

				
				iftype_h = tblink_rpc_IEndpoint_defineInterfaceType(ep, iftype_b);
			end
			
			return iftype_h;
		endfunction
		
	endclass

	class zephyr_cosim_agent extends uvm_component;
		`uvm_component_utils(zephyr_cosim_agent)
		
		chandle				m_endpoint;
		zephyr_cosim_if		m_if;
		
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
				chandle iftype_h, ifinst_h;
				string errmsg;
				
				tblink_rpc_ILaunchParams_add_arg(launch_params, zephyr_exe);
				tblink_rpc_ILaunchParams_add_arg(launch_params, tblink_libpath);
			
				m_endpoint = tblink_rpc_ILaunchType_launch(
						launch_type,
						launch_params,
						errmsg);
				
				if (m_endpoint == null) begin
					$display("Failed to launch: %0s", errmsg);
					return;
				end else begin
					$display("Launch succeeded");
				end
				
				iftype_h = zephyr_cosim_if::registerType(m_endpoint);
				m_if = new(this);
				
				ifinst_h = tblink_rpc_IEndpoint_defineInterfaceInst(
						m_endpoint,
						iftype_h,
						"cosim",
						0,
						m_if);
			
				$display("--> uvm: build_complete");
				if (tblink_rpc_IEndpoint_build_complete(m_endpoint) == -1) begin
					$display("Error: build_complete failed");
					$finish();
				end
				$display("<-- uvm: build_complete");
			end
			
		endfunction
		
		function void connect_phase(uvm_phase phase);
			$display("connect_phase");
			
			$display("--> uvm: connect_complete");
			if (tblink_rpc_IEndpoint_connect_complete(m_endpoint) == -1) begin
				$display("Error: connect_complete failed");
				$finish();
			end
			$display("<-- uvm: connect_complete");
		endfunction
		
		task run_phase(uvm_phase phase);
			phase.raise_objection(this, "Main", 1);
			$display("--> start");
			tblink_rpc_IEndpoint_start(m_endpoint);
			$display("<-- start");
			phase.drop_objection(this, "Main", 1);
		endtask
		
		task sys_read8(
			output byte unsigned		data,
			input longint unsigned		addr);
		endtask
		
		task sys_write8(
			input byte unsigned			data,
			input longint unsigned		addr);
		endtask

		task sys_read16(
			output byte unsigned		data,
			input longint unsigned		addr);
		endtask
		
		task sys_write16(
			input byte unsigned			data,
			input longint unsigned		addr);
		endtask
		
		task sys_read32(
			output byte unsigned		data,
			input longint unsigned		addr);
		endtask
		
		task sys_write32(
			input byte unsigned			data,
			input longint unsigned		addr);
		endtask
		
	endclass

endpackage


