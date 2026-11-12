#ifndef PCIE_DEMO_H
#define PCIE_DEMO_H

#include <stdbool.h>
#include <stdint.h>

#define PCIE_ECAM_BASE 0x90000000UL
#define PCIE_BAR0_ADDR 0xA0000000UL
#define PCIE_MSI_ADDR  0xB0000000UL
#define PCIE_MSI_DATA  0x42

#define PCIE_PLIC_BASE 0xE4000000UL
#define PCIE_INTX_IRQ  20
#define PCIE_MSI_IRQ   21

#define PCIE_REG_SCRATCH  0x00
#define PCIE_REG_MAGIC    0x04
#define PCIE_REG_DOORBELL 0x08

struct pcie_test_results {
	int root_port;
	int bus_nr;
	int connector;
	int enumerated;
	int ids;
	int bar_size;
	int bar_assign;
	int mmio_write;
	int mmio_read;
	int round_trip;
	int intx;
	int intx_plic;
	int isr;
	int msi;
};

uint32_t pcie_cfg_read32(uint8_t bus, uint8_t dev, uint8_t fn, uint16_t reg);
void pcie_cfg_write32(uint8_t bus, uint8_t dev, uint8_t fn, uint16_t reg, uint32_t value);
void pcie_print_capabilities(uint8_t bus, uint8_t dev, uint8_t fn);

bool pcie_setup_root_port(struct pcie_test_results *results);
void pcie_scan_endpoints(struct pcie_test_results *results);
void pcie_run_interrupt_tests(uint8_t dev, struct pcie_test_results *results);
void pcie_print_summary(const struct pcie_test_results *results);

#endif
