/****************************************************************************
 * zephyr_cosim_if.svh
 ****************************************************************************/
  
typedef class zephyr_cosim_agent;

/**
 * Class: zephyr_cosim_if
 *
 * InterfaceImpl class interfaces to tblink-rpc
 */
class zephyr_cosim_if extends IInterfaceImpl;
	zephyr_cosim_agent			m_agent;
	IMethodType					m_sys_irq_t;
		
	typedef enum {
		sys_read8 = 1,
		sys_write8,
		sys_read16,
		sys_write16,
		sys_read32,
		sys_write32,
		sys_irq
	} method_id_t;
		
	function new(zephyr_cosim_agent agent);
		IInterfaceType iftype;
		
		m_agent = agent;
		iftype = registerType(m_agent.m_endpoint);
		m_sys_irq_t = iftype.findMethod("sys_irq");
	endfunction
	
	function void irq(int num);
		IParamValVec params = m_agent.m_ifinst.mkValVec();
		params.push_back(m_agent.m_ifinst.mkValIntU(num, 8));
		void'(m_agent.m_ifinst.invoke_nb(
				m_sys_irq_t,
				params));
	endfunction
		
	virtual task invoke_b(
		output IParamVal			retval,
		input IInterfaceInst		ifinst,
		input IMethodType			method,
		input IParamValVec			params);
		IParamValInt addr_p;
		IParamValInt data_p;
		retval = null;
			
		$display("--> invoke_b");
			
		case (method.id())
			sys_read8: begin
				byte unsigned data;
				$cast(addr_p, params.at(0));
					
				m_agent.sys_read8(data, addr_p.val_u());
				retval = ifinst.mkValIntU(data, 8);
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
				retval = ifinst.mkValIntU(data, 16);
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
				retval = ifinst.mkValIntU(data, 32);
			end
			sys_write32: begin
				$cast(data_p, params.at(0));
				$cast(addr_p, params.at(1));
				m_agent.sys_write32(
						data_p.val_u(),
						addr_p.val_u());
			end
		endcase
			
		$display("<-- invoke_b");
	endtask
		
	static function IInterfaceType registerType(IEndpoint ep);
		IInterfaceType iftype = ep.findInterfaceType("zephyr_cosim_if");
			
		if (iftype == null) begin
			IMethodTypeBuilder method_b;
			IInterfaceTypeBuilder iftype_b = ep.newInterfaceTypeBuilder(
					"zephyr_cosim_if");

			// read8
			method_b = iftype_b.newMethodTypeBuilder(
					"sys_read8",
					sys_read8,
					iftype_b.mkTypeInt(0, 8),
					1,
					1);
			method_b.add_param(
					"addr",
					iftype_b.mkTypeInt(0, 64));
			void'(iftype_b.add_method(method_b));
				
			// write8
			method_b = iftype_b.newMethodTypeBuilder(
					"sys_write8",
					sys_write8,
					null,
					1,
					1);
			method_b.add_param(
					"data",
					iftype_b.mkTypeInt(0, 8));
			method_b.add_param(
					"addr",
					iftype_b.mkTypeInt(0, 64));
			void'(iftype_b.add_method(method_b));

			// read16
			method_b = iftype_b.newMethodTypeBuilder(
					"sys_read16",
					sys_read16,
					iftype_b.mkTypeInt(0, 16),
					1,
					1);
			method_b.add_param(
					"addr",
					iftype_b.mkTypeInt(0, 64));
			void'(iftype_b.add_method(method_b));
				
			// write16
			method_b = iftype_b.newMethodTypeBuilder(
					"sys_write16",
					sys_write16,
					null,
					1,
					1);
			method_b.add_param(
					"data",
					iftype_b.mkTypeInt(0, 16));
			method_b.add_param(
					"addr",
					iftype_b.mkTypeInt(0, 64));
			void'(iftype_b.add_method(method_b));
				
			// read32
			method_b = iftype_b.newMethodTypeBuilder(
					"sys_read32",
					sys_read32,
					iftype_b.mkTypeInt(0, 32),
					1,
					1);
			method_b.add_param(
					"addr",
					iftype_b.mkTypeInt(0, 64));
			void'(iftype_b.add_method(method_b));
				
			// write32
			method_b = iftype_b.newMethodTypeBuilder(
					"sys_write32",
					sys_write32,
					null,
					1,
					1);
			method_b.add_param(
					"data",
					iftype_b.mkTypeInt(0, 32));
			method_b.add_param(
					"addr",
					iftype_b.mkTypeInt(0, 64));
			void'(iftype_b.add_method(method_b));
			
			// IRQ
			method_b = iftype_b.newMethodTypeBuilder(
					"sys_irq",
					sys_irq,
					null,
					0, // is_export
					0);
			method_b.add_param(
					"num",
					iftype_b.mkTypeInt(0, 8));
			void'(iftype_b.add_method(method_b));

				
			iftype = ep.defineInterfaceType(iftype_b);
		end
			
		return iftype;
	endfunction
		
endclass


