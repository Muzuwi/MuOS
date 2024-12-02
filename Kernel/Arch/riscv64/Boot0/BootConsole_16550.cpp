#include <Arch/riscv64/Boot0/Boot0.hpp>
#include <Arch/riscv64/Boot0/BootConsole.hpp>
#include <Arch/riscv64/Boot0/DeviceTree.hpp>
#include <Arch/riscv64/Boot0/Memory.hpp>
#include <SystemTypes.hpp>

enum class Register {
	Data = 0,
	IrqEn = 1,
	IrqId = 2,
	LineControl = 3,
	ModemControl = 4,
	LineStatus = 5,
	ModemStatus = 6,
	Scratch = 7
};

static void* s_uart_base { nullptr };

static inline uint32 read_reg(Register reg) {
	if(!s_uart_base) {
		return {};
	}
	return *((volatile uint32*)((uint8*)s_uart_base + static_cast<uintptr_t>(reg) * 4));
}

static inline void write_reg(Register reg, uint32 value) {
	if(!s_uart_base) {
		return;
	}
	*((volatile uint32*)((uint8*)s_uart_base + static_cast<uintptr_t>(reg) * 4)) = value;
}

void bootcon::init(FdtHeader const* fdt) {
	FdtNodeHandle uart { nullptr };
	fdt_visit_each_node(
	        fdt,
	        [](FdtHeader const* fdt, FdtNodeHandle nhandle, void* priv) -> bool {
		        const auto compatible = fdt_node_get_named_prop(fdt, nhandle, "compatible");
		        if(!compatible) {
			        return true;
		        }
		        auto const* compatstr = fdt_prop_read_string(fdt, compatible);
		        if(mem::strcmp(compatstr, "snps,dw-apb-uart") == 0) {
			        *static_cast<FdtNodeHandle*>(priv) = nhandle;
			        return false;
		        }
		        return true;
	        },
	        &uart);
	if(!uart) {
		//  Failed to init bootcon
		return;
	}

	const auto reg = fdt_node_get_named_prop(fdt, uart, "reg");
	if(!reg) {
		return;
	}
	uint64 addr;
	if(!fdt_prop_read_u64(fdt, reg, &addr)) {
		return;
	}
	s_uart_base = reinterpret_cast<void*>(addr);

	write_reg(Register::IrqEn, 0x0);
	write_reg(Register::IrqEn, 0x2);
	write_reg(Register::IrqId, 0x01);
}

void bootcon::putch(char ch) {
	while(!(read_reg(Register::LineStatus) & (1u << 5u))) {
		//  busy loop until txhr is empty
	}
	write_reg(Register::Data, ch);
}

void bootcon::remap() {
	if(!s_uart_base) {
		return;
	}

	addrmap((void*)s_uart_base, (void*)s_uart_base, DEFAULT_FLAGS);
}
