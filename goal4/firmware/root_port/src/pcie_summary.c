#include <zephyr/kernel.h>

#include "pcie_demo.h"

static const char *pass_fail(int passed)
{
	return passed ? "PASS" : "FAIL";
}

void pcie_print_summary(const struct pcie_test_results *results)
{
	printk("\n========== GOAL 4 TEST SUMMARY ==========\n");
	printk("Root Port (Type 1 bridge)   : %s\n", pass_fail(results->root_port));
	printk("Bus numbers programmed      : %s\n", pass_fail(results->bus_nr));
	printk("Connector (RP -> EP)        : %s\n", pass_fail(results->connector));
	printk("Endpoint enumeration        : %s\n", pass_fail(results->enumerated));
	printk("Vendor/Device ID read       : %s\n", pass_fail(results->ids));
	printk("BAR size detection (4 KB)   : %s\n", pass_fail(results->bar_size));
	printk("BAR address assignment      : %s\n", pass_fail(results->bar_assign));
	printk("Endpoint MMIO write         : %s\n", pass_fail(results->mmio_write));
	printk("Endpoint MMIO read          : %s\n", pass_fail(results->mmio_read));
	printk("MMIO round trip             : %s\n", pass_fail(results->round_trip));
	printk("INTx interrupt (endpoint)   : %s\n", pass_fail(results->intx));
	printk("INTx reached CPU (ISR/claim) : %s\n", pass_fail(results->intx_plic));
	printk("MSI (capability -> PLIC)    : %s\n", pass_fail(results->msi));
	printk("Interrupt handled by ISR    : %s\n",
	       results->isr ? "PASS" : "NOT TAKEN (polled instead)");

	int all = results->root_port && results->bus_nr && results->connector &&
		  results->enumerated && results->ids && results->bar_size &&
		  results->bar_assign && results->round_trip && results->intx &&
		  results->intx_plic && results->msi;

	printk("\nOverall Result: %s\n", all ? "SUCCESS" : "FAILURE");
	printk("== done ==\n");
}
