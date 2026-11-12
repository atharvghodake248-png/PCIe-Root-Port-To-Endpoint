#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>

#include "pcie_demo.h"

static uintptr_t pcie_cfg_addr(uint8_t bus, uint8_t dev, uint8_t fn, uint16_t reg)
{
	return PCIE_ECAM_BASE | ((uintptr_t)bus << 20) | ((uintptr_t)dev << 15) |
	       ((uintptr_t)fn << 12) | (reg & 0xFFC);
}

uint32_t pcie_cfg_read32(uint8_t bus, uint8_t dev, uint8_t fn, uint16_t reg)
{
	return sys_read32(pcie_cfg_addr(bus, dev, fn, reg));
}

void pcie_cfg_write32(uint8_t bus, uint8_t dev, uint8_t fn, uint16_t reg, uint32_t value)
{
	sys_write32(value, pcie_cfg_addr(bus, dev, fn, reg));
}

void pcie_print_capabilities(uint8_t bus, uint8_t dev, uint8_t fn)
{
	uint32_t status = pcie_cfg_read32(bus, dev, fn, 0x04) >> 16;

	if (!(status & 0x10)) {
		printk("         no capability list\n");
		return;
	}

	uint32_t ptr = pcie_cfg_read32(bus, dev, fn, 0x34) & 0xFC;

	while (ptr) {
		uint32_t header = pcie_cfg_read32(bus, dev, fn, ptr);

		printk("         capability id=0x%02x at 0x%02x\n", header & 0xFF, ptr);
		ptr = (header >> 8) & 0xFC;
	}
}

bool pcie_setup_root_port(struct pcie_test_results *results)
{
	uint32_t id = pcie_cfg_read32(0, 0, 0, 0x00);

	if (id == 0xFFFFFFFF) {
		printk("00:00.0  no root port\n");
		return false;
	}

	uint32_t header_type = (pcie_cfg_read32(0, 0, 0, 0x0C) >> 16) & 0x7F;

	printk("00:00.0  vendor=0x%04x device=0x%04x header_type=0x%02x\n",
	       id & 0xFFFF, id >> 16, header_type);
	printk("         class/rev=0x%08x (bridge)\n", pcie_cfg_read32(0, 0, 0, 0x08));
	results->root_port = (header_type == 1);

	pcie_cfg_write32(0, 0, 0, 0x18, 0x00010100);
	uint32_t bus_numbers = pcie_cfg_read32(0, 0, 0, 0x18) & 0xFFFFFF;

	printk("         bus numbers pri/sec/sub = 0x%06x\n", bus_numbers);
	results->bus_nr = (bus_numbers == 0x010100);

	pcie_cfg_write32(0, 0, 0, 0x20, 0xA000A000);
	printk("         memory base/limit reg = 0x%08x\n", pcie_cfg_read32(0, 0, 0, 0x20));

	pcie_cfg_write32(0, 0, 0, 0x04, pcie_cfg_read32(0, 0, 0, 0x04) | 0x6);
	return true;
}
