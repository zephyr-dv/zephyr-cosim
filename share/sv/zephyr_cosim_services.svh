/****************************************************************************
 * zephyr_cosim_services.svh
 ****************************************************************************/

  
/**
 * Class: zephyr_cosim_services
 * 
 * TODO: Add class documentation
 */
class zephyr_cosim_services extends SVEndpointServices;
	zephyr_cosim_agent			m_agent;
	bit							m_run_until_event;

	function new(zephyr_cosim_agent agent);
		m_agent = agent;
	endfunction
	
	virtual function void shutdown();
		$display("--> services::shutdown");
		m_run_until_event = 0;
		m_agent.shutdown();
		$display("<-- services::shutdown");
	endfunction

	virtual function void run_until_event();
		$display("--> services::run_until_event");
		m_run_until_event = 1;
		$display("<-- services::run_until_event");
	endfunction
		
	virtual function void hit_event();
		$display("--> services::hit_event");
		m_run_until_event = 0;
		$display("<-- services::hit_event");
	endfunction
		
	virtual function void idle();
		$display("--> services::idle");
		while (!m_run_until_event) begin
			if (m_ep.process_one_message() == -1) begin
				break;
			end
		end
		$display("<-- services::idle");
	endfunction

endclass


