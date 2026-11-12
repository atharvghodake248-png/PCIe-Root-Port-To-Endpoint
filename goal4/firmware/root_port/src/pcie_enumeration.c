#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>

#include "pcie_demo.h"

static void test_endpoint(uint8_t dev, uint32_t endpoint_id,
			  struct pcie_test_results *results)
{
	printk("01:%02x.0  vendor=0x%04x device=0x%04x\n",
	       dev, endpoint_id & 0xFFFF, endpoint_id >> 16);
	printk("         class/rev=0x%08x\n", pcie_cfg_read32(1, dev, 0, 0x08));
	pcie_print_capabilities(1, dev, 0);

	results->connector = 1;
	results->enumerated = 1;
	results->ids = ((endpoint_id & 0xFFFF) == 0x1234 && (endpoint_id >> 16) == 0x0002);

	uint32_t original_bar = pcie_cfg_read32(1, dev, 0, 0x10);

	pcie_cfg_write32(1, dev, 0, 0x10, 0xFFFFFFFF);
	uint32_t bar_mask = pcie_cfg_read32(1, dev, 0, 0x10);

	pcie_cfg_write32(1, dev, 0, 0x10, original_bar);
	uint32_t bar_size = ~(bar_mask & ~0xFUL) + 1;

	printk("         BAR0 mask=0x%08x size=0x%x bytes\n", bar_mask, bar_size);
	results->bar_size = (bar_size == 0x1000);

	pcie_cfg_write32(1, dev, 0, 0x10, PCIE_BAR0_ADDR);
	pcie_cfg_write32(1, dev, 0, 0x04, pcie_cfg_read32(1, dev, 0, 0x04) | 0x6);
	uint32_t assigned_bar = pcie_cfg_read32(1, dev, 0, 0x10);

	printk("         BAR0 now=0x%08x CMD=0x%08x\n", assigned_bar,
	       pcie_cfg_read32(1, dev, 0, 0x04));
	results->bar_assign = (assigned_bar == PCIE_BAR0_ADDR);

	sys_write32(0x12345678, PCIE_BAR0_ADDR + PCIE_REG_SCRATCH);
	uint32_t scratch = sys_read32(PCIE_BAR0_ADDR + PCIE_REG_SCRATCH);
	uint32_t magic = sys_read32(PCIE_BAR0_ADDR + PCIE_REG_MAGIC);

	results->mmio_write = (scratch == 0x12345678);
	results->mmio_read = (magic == 0xE0E0E0E0);
	results->round_trip = results->mmio_write && results->mmio_read;
	printk("         BAR0 scratch wrote=0x12345678 read=0x%08x magic=0x%08x %s\n",
	       scratch, magic, results->round_trip ? "OK" : "FAIL");

	pcie_run_interrupt_tests(dev, results);
}

void pcie_scan_endpoints(struct pcie_test_results *results)
{
	for (uint8_t dev = 0; dev < 4; dev++) {
		uint32_t endpoint_id = pcie_cfg_read32(1, dev, 0, 0x00);

		if (endpoint_id == 0xFFFFFFFF) {
			printk("01:%02x.0  no device\n", dev);
			continue;
		}

		test_endpoint(dev, endpoint_id, results);
	}
}
