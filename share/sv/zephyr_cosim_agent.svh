/****************************************************************************
 * zephyr_cosim_agent.svh
 ****************************************************************************/

  
/**
 * Class: zephyr_cosim_agent
 * 
 * TODO: Add class documentation
 */
class zephyr_cosim_agent extends uvm_component;
	`uvm_component_utils(zephyr_cosim_agent)
	
	typedef class Listener;
		
	IEndpoint			m_endpoint;
	IInterfaceInst		m_ifinst;
	zephyr_cosim_if		m_if;
	uvm_sequencer		m_seqr;
	uvm_reg_adapter		m_adapter;
	uvm_phase			m_run_phase;
	Listener			m_listener;
	semaphore			m_ev_sem = new();
		
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
	
	function void irq(int num);
		m_if.irq(num);
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
			TbLink tblink = TbLink::inst();
			ILaunchType launch_type;
			ILaunchParams launch_params;
			IInterfaceType iftype;
			zephyr_cosim_services services;
			string errmsg;
			
			launch_type = tblink.findLaunchType("process.socket");
			launch_params = launch_type.newLaunchParams();
				
			launch_params.add_arg(zephyr_exe);
			launch_params.add_arg(tblink_libpath);
			
			m_endpoint = launch_type.launch(
					launch_params,
					null,
					errmsg);
			
			m_listener = new(this);
			m_endpoint.addListener(m_listener);
				
			if (m_endpoint == null) begin
				`uvm_fatal("zephyr-cosim-agent", $sformatf("Failed to launch: %0s", errmsg));
				return;
			end else begin
				`uvm_info("zephyr-cosim-agent", "Note: Launch succeeded", UVM_LOW);
			end
			
			services = new(this);
			m_endpoint.init(services);
				
			iftype = zephyr_cosim_if::registerType(m_endpoint);
			m_if = new(this);
				
			m_ifinst = m_endpoint.defineInterfaceInst(
					iftype,
					"cosim",
					0,
					m_if);
			
			$display("--> uvm: build_complete");
			if (m_endpoint.build_complete() == -1) begin
				`uvm_fatal("zephyr-cosim-agent", "build phase (1) failed");
				$finish();
			end

			while (1) begin
				int ret = m_endpoint.is_build_complete();
				if (ret == -1) begin
					`uvm_fatal("zephyr-cosim-agent", "build phase failed");
					break;
				end else if (ret == 1) begin
					`uvm_info("zephyr-cosim-agent", "build phase completed", UVM_LOW);
					break;
				end else begin
					if (m_endpoint.process_one_message() == -1)  begin
						`uvm_fatal("zephyr-cosim-agent", "process-one-message failed during build-complete");
						break;
					end
				end
			end
			$display("<-- uvm: build_complete");
		end
			
	endfunction
		
	function void connect_phase(uvm_phase phase);
		$display("connect_phase");
			
		$display("--> uvm: connect_complete");
		if (m_endpoint.connect_complete() == -1) begin
			`uvm_fatal("zephyr-cosim-agent", "connect phase (1) failed");
			$finish();
		end
			
		while (1) begin
			int ret= m_endpoint.is_connect_complete();
			if (ret == -1) begin
				`uvm_fatal("zephyr-cosim-agent", "connect phase failed");
				break;
			end else if (ret == 1) begin
				`uvm_info("zephyr-cosim-agent", "connect phase completed", UVM_LOW);
				break;
			end else begin
				if (m_endpoint.process_one_message() == -1)  begin
					`uvm_fatal("zephyr-cosim-agent", "process-one-message failed during connect-complete");
					break;
				end
			end
		end
		$display("<-- uvm: connect_complete");
	endfunction
	
	function void shutdown();
		if (m_run_phase != null) begin
			m_run_phase.drop_objection(this, "Main", 1);
		end
	endfunction
		
	task run_phase(uvm_phase phase);
		int rv = 0;
		m_run_phase = phase;
		if (m_seqr == null || m_adapter == null) begin
			`uvm_fatal("zephyr-cosim-agent", "sequencer not configured");
			return;
		end
		
		phase.raise_objection(this, "Main", 1);
	
		// Start TbLink management thread
		tblink_rpc_start();
		
		$display("--> start");
		while (rv != -1) begin
			$display("--> Waiting %0s", m_endpoint.comm_state());
			while (m_endpoint.comm_state() == IEndpoint::Waiting && rv != -1) begin
				// Should this be the '_b' variant?
				$display("--> process_one_message");
				rv = m_endpoint.process_one_message();
				$display("<-- process_one_message");
			end
			$display("<-- Waiting %0s", m_endpoint.comm_state());
			
			$display("--> Released %0s", m_endpoint.comm_state());
			while (m_endpoint.comm_state() == IEndpoint::Released) begin
				// Should this be the '_b' variant?
				$display("--> await_event");
				m_ev_sem.get(1);
				$display("<-- await_event %0s", m_endpoint.comm_state());
			end
			$display("<-- Released");
		end
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
		$display("==> Sending an IRQ");
		irq(10);
		$display("<== Sending an IRQ");
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
	
	class Listener extends IEndpointListener;
		zephyr_cosim_agent			m_agent;
		
		function new(zephyr_cosim_agent agent);
			m_agent = agent;
		endfunction
		
		function void event_f(IEndpointEvent ev);
			$display("Listener::event_f");
			m_agent.m_ev_sem.put(1);
		endfunction
		
	endclass
		
endclass
