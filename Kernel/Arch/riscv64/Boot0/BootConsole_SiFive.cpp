#include <Arch/riscv64/Boot0/Boot0.hpp>
#include <Arch/riscv64/Boot0/BootConsole.hpp>
#include <Arch/riscv64/Boot0/DeviceTree.hpp>
#include <Arch/riscv64/Boot0/Memory.hpp>

enum class Register {
	TXDATA = 0x0,
	RXDATA = 0x04,
	TXCTRL = 0x08,
	RXCTRL = 0x0C,
	IE = 0x10,
	IP = 0x14,
	DIV = 0x18
};

static void* s_sifive_uart_base { nullptr };

static inline uint32 read_reg(Register reg) {
	if(!s_sifive_uart_base) {
		return {};
	}
	return *((volatile uint32*)((uint8*)s_sifive_uart_base + static_cast<uintptr_t>(reg)));
}

static inline void write_reg(Register reg, uint32 value) {
	if(!s_sifive_uart_base) {
		return;
	}
	*((volatile uint32*)((uint8*)s_sifive_uart_base + static_cast<uintptr_t>(reg))) = value;
}

void bootcon::init(FdtHeader const* fdt) {
	//  Find the first UART with SiFive compatible
	FdtNodeHandle uart { nullptr };
	fdt_visit_each_node(
	        fdt,
	        [](FdtHeader const* fdt, FdtNodeHandle nhandle, void* priv) -> bool {
		        const auto compatible = fdt_node_get_named_prop(fdt, nhandle, "compatible");
		        if(!compatible) {
			        return true;
		        }
		        auto const* compatstr = fdt_prop_read_string(fdt, compatible);
		        if(mem::strcmp(compatstr, "sifive,uart0") == 0) {
			        *static_cast<FdtNodeHandle*>(priv) = nhandle;
			        return false;
		        }
		        return true;
	        },
	        &uart);
	if(!uart) {
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
	s_sifive_uart_base = reinterpret_cast<void*>(addr);

	//  We got an UART - initialize it now
	write_reg(Register::TXCTRL, 0x1);//  txen = 1, one stop bit
	write_reg(Register::RXCTRL, 0x0);//  rxen = 0

	//  For simplicity, don't initialize baud rate and hope the previous
	//  stage already initialized it for us.
}

void bootcon::putch(char ch) {
	while(read_reg(Register::TXDATA) & (1u << 31u)) {
		//  busy loop until full condition is cleared
	}
	write_reg(Register::TXDATA, ch);
}

void bootcon::remap() {
	if(!s_sifive_uart_base) {
		return;
	}

	addrmap((void*)s_sifive_uart_base, (void*)s_sifive_uart_base, DEFAULT_FLAGS);
}