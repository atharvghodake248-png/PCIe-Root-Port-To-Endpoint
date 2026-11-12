#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/irq.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/devicetree.h>

#include "pcie_demo.h"

#if defined(CONFIG_DYNAMIC_INTERRUPTS) && defined(CONFIG_2ND_LEVEL_INTERRUPTS)
#define ISR_AVAILABLE 1
#define UART_IRQN DT_IRQN(DT_CHOSEN(zephyr_console))
#define PCIE_ZIRQ (UART_IRQN + (((int)PCIE_INTX_IRQ - (int)irq_from_level_2(UART_IRQN)) << \
				CONFIG_1ST_LEVEL_INTERRUPT_BITS))
static volatile int isr_count;

static void pcie_isr(const void *arg)
{
	ARG_UNUSED(arg);
	isr_count++;
	irq_disable(PCIE_ZIRQ);
}
#else
#define ISR_AVAILABLE 0
#endif

static void plic_enable(int source)
{
	sys_write32(1, PCIE_PLIC_BASE + 4 * source);
	sys_write32(sys_read32(PCIE_PLIC_BASE + 0x2000) | (1U << source),
		    PCIE_PLIC_BASE + 0x2000);
	sys_write32(0, PCIE_PLIC_BASE + 0x200000);
}

static void test_intx(uint8_t dev, struct pcie_test_results *results)
{
	plic_enable(PCIE_INTX_IRQ);
#if ISR_AVAILABLE
	int irq = irq_connect_dynamic(PCIE_ZIRQ, 1, pcie_isr, NULL, 0);

	irq_enable(PCIE_ZIRQ);
	printk("         ISR installed on irq 0x%x\n", irq);
#endif
	sys_write32(1, PCIE_BAR0_ADDR + PCIE_REG_DOORBELL);

#if ISR_AVAILABLE
	for (volatile uint32_t i = 0; i < 2000000 && !isr_count; i++) {
	}
#endif
	uint32_t status_asserted = (pcie_cfg_read32(1, dev, 0, 0x04) >> 19) & 1;
	uint32_t claim;
	int via_isr = 0;
	int isr_runs = 0;

#if ISR_AVAILABLE
	via_isr = (isr_count > 0);
	isr_runs = isr_count;
#endif
	if (via_isr) {
		claim = 0;
		printk("         INTx asserted: endpoint status.int=%u, ISR ran %d time(s) "
		       "(PLIC claim done by Zephyr's driver)\n", status_asserted, isr_runs);
	} else {
		claim = sys_read32(PCIE_PLIC_BASE + 0x200004);
		printk("         INTx asserted: endpoint status.int=%u plic_claim=%u (expected %u) via polling\n",
		       status_asserted, claim, PCIE_INTX_IRQ);
	}

	sys_write32(0, PCIE_BAR0_ADDR + PCIE_REG_DOORBELL);
	if (!via_isr && claim) {
		sys_write32(claim, PCIE_PLIC_BASE + 0x200004);
	}
	uint32_t status_cleared = (pcie_cfg_read32(1, dev, 0, 0x04) >> 19) & 1;

	printk("         INTx cleared:  endpoint status.int=%u\n", status_cleared);
	results->intx = (status_asserted == 1 && status_cleared == 0);
	results->intx_plic = via_isr ? (isr_runs >= 1) : (claim == PCIE_INTX_IRQ);
	results->isr = via_isr;
}

static void test_msi(uint8_t dev, struct pcie_test_results *results)
{
	plic_enable(PCIE_MSI_IRQ);
	pcie_cfg_write32(1, dev, 0, 0x44, PCIE_MSI_ADDR);
	pcie_cfg_write32(1, dev, 0, 0x48, PCIE_MSI_DATA);
	pcie_cfg_write32(1, dev, 0, 0x40, pcie_cfg_read32(1, dev, 0, 0x40) | (1U << 16));
	printk("         MSI cap programmed: addr=0x%08x data=0x%x ctrl=0x%08x\n",
	       pcie_cfg_read32(1, dev, 0, 0x44), pcie_cfg_read32(1, dev, 0, 0x48),
	       pcie_cfg_read32(1, dev, 0, 0x40));

	sys_write32(1, PCIE_BAR0_ADDR + PCIE_REG_DOORBELL);
	uint32_t data = sys_read32(PCIE_MSI_ADDR);
	uint32_t claim = sys_read32(PCIE_PLIC_BASE + 0x200004);

	printk("         MSI received: data=0x%x plic_claim=%u (expected %u)\n",
	       data, claim, PCIE_MSI_IRQ);

	sys_write32(0, PCIE_MSI_ADDR);
	if (claim) {
		sys_write32(claim, PCIE_PLIC_BASE + 0x200004);
	}
	pcie_cfg_write32(1, dev, 0, 0x40,
			 pcie_cfg_read32(1, dev, 0, 0x40) & ~(1U << 16));
	results->msi = (data == PCIE_MSI_DATA && claim == PCIE_MSI_IRQ);
}

void pcie_run_interrupt_tests(uint8_t dev, struct pcie_test_results *results)
{
	test_intx(dev, results);
	test_msi(dev, results);
}
