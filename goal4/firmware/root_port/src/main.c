#include <zephyr/kernel.h>

#include "pcie_demo.h"

int main(void)
{
	struct pcie_test_results results = {0};

	printk("== Raw-MMIO PCIe enumeration (Type 1 Root Port + Endpoint) ==\n");

	if (!pcie_setup_root_port(&results)) {
		return 0;
	}

	pcie_scan_endpoints(&results);
	pcie_print_summary(&results);
	return 0;
}
