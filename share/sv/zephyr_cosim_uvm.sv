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
			chandle ifinst = tblink_rpc_InvokeInfo_ifinst(ii.m_hndl);
			longint unsigned id = tblink_rpc_IMethodType_id(method_t);
			IParamValVector params = ii.params();
			IParamValInt addr_p;
			IParamValInt data_p;
			
			case (id)
				sys_read8: begin
					byte unsigned data;
					$cast(addr_p, params.at(0));
					
					m_agent.sys_read8(data, addr_p.val_u());
					ret = tblink_rpc_IInterfaceInst_mkValIntU(
							ifinst, 
							data,
							8);
				end
				sys_write8: begin
					$cast(data_p, params.at(0));
					$cast(addr_p, params.at(1));
					
					m_agent.sys_write8(
							data_p.val_u(),
							addr_p.val_u());
				end
				sys_read16: begin
					shortint unsigned data;
					$cast(addr_p, params.at(0));
					
					m_agent.sys_read16(data, addr_p.val_u());
					ret = tblink_rpc_IInterfaceInst_mkValIntU(
							ifinst, 
							data,
							16);
				end
				sys_write16: begin
					$cast(data_p, params.at(0));
					$cast(addr_p, params.at(1));
					
					m_agent.sys_write16(
							data_p.val_u(),
							addr_p.val_u());
				end
				sys_read32: begin
					int unsigned data;
					$cast(addr_p, params.at(0));
					
					m_agent.sys_read32(data, addr_p.val_u());
					ret = tblink_rpc_IInterfaceInst_mkValIntU(
							ifinst, 
							data,
							32);
				end
				sys_write32: begin
					$cast(data_p, params.at(0));
					$cast(addr_p, params.at(1));
					m_agent.sys_write32(
							data_p.val_u(),
							addr_p.val_u());
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
		uvm_sequencer		m_seqr;
		uvm_reg_adapter		m_adapter;
		
		function new(string name, uvm_component parent);
			super.new(name, parent);
		endfunction
	
		/**
		 * Function: set_sequencer
		 * 
		 * Configures the sequencer and register adapter
		 * used by this agent
		 * 
		 * Parameters:
		 * - uvm_sequencer seqr 
		 * - uvm_reg_adapter reg_adapter 
		 * 
		 * Returns:
		 *   void
		 */
		function void set_sequencer(
			uvm_sequencer			seqr,
			uvm_reg_adapter			reg_adapter);
			m_seqr = seqr;
			m_adapter = reg_adapter;
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
			if (m_seqr == null || m_adapter == null) begin
				`uvm_fatal("zephyr-cosim-agent", "sequencer not configured");
				return;
			end
			
			phase.raise_objection(this, "Main", 1);
			$display("--> start");
			tblink_rpc_IEndpoint_start(m_endpoint);
			$display("<-- start");
//			phase.drop_objection(this, "Main", 1);
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
			output shortint unsigned	data,
			input longint unsigned		addr);
		endtask
		
		task sys_write16(
			input shortint unsigned		data,
			input longint unsigned		addr);
		endtask
		
		task sys_read32(
			output int unsigned			data,
			input longint unsigned		addr);
			uvm_reg_bus_op rw_access;
			uvm_sequence_item bus_req;
			uvm_sequence_base seq = new("default_parent_seq");
			$display("Agent sys_write32: 'h%08h 'h%08h", data, addr);

			rw_access.kind    = UVM_READ;
			rw_access.addr    = addr;
			rw_access.n_bits  = 32;
			rw_access.byte_en = 'hF;

			bus_req = m_adapter.reg2bus(rw_access);
			
			$display("[%0t] --> Seqr Access", $time);
			seq.set_sequencer(m_seqr);
			seq.start_item(bus_req);
			seq.finish_item(bus_req);
			$display("[%0t] <-- Seqr Access", $time);			
			
			 m_adapter.bus2reg(bus_req, rw_access);
			
			data = rw_access.data;
		endtask
		
		task sys_write32(
			input int unsigned			data,
			input longint unsigned		addr);
			uvm_reg_bus_op rw_access;
			uvm_sequence_item bus_req;
			uvm_sequence_base seq = new("default_parent_seq");
			$display("Agent sys_write32: 'h%08h 'h%08h", data, addr);

			rw_access.kind    = UVM_WRITE;
			rw_access.addr    = addr;
			rw_access.data    = data;
			rw_access.n_bits  = 32;
			rw_access.byte_en = 'hF;

			bus_req = m_adapter.reg2bus(rw_access);
			
			$display("[%0t] --> Seqr Access", $time);
			seq.set_sequencer(m_seqr);
			seq.start_item(bus_req);
			seq.finish_item(bus_req);
			$display("[%0t] <-- Seqr Access", $time);
		endtask
		
	endclass

endpackage


