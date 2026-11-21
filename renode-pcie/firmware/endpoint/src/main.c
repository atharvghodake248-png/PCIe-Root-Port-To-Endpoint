#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <stdint.h>

#define EP_BASE      0xA0000000UL

/* ---- Endpoint identity: change these to verify (only this file) ---- */
#define EP_VENDOR    0x1234
#define EP_DEVICE    0x0002
#define EP_MAGIC     0xE0E0E0E0
#define EP_SCRATCH_INIT 0x12345678

#define REG_SCRATCH  0x00
#define REG_MAGIC    0x04
#define REG_STATUS   0x0C   /* bit0 BAR0 set, bit1 MemSpace, bit2 BusMaster, bit3 MSI on, bit4 INTx */
#define REG_BAR0     0x10
#define REG_HOSTWR   0x14   /* number of BAR writes made by the host */
#define REG_IDENT    0x20   /* [15:0] vendor, [31:16] device (first write = device becomes visible) */
#define REG_MAGIC_SET 0x24

static uint32_t rd(uint32_t off)
{
        return sys_read32(EP_BASE + off);
}

static void wr32(uint32_t off, uint32_t val)
{
        sys_write32(val, EP_BASE + off);
}

int main(void)
{
        printk("== Endpoint machine (ep): Zephyr booted ==\n");

        wr32(REG_MAGIC_SET, EP_MAGIC);
        wr32(REG_IDENT, ((uint32_t)EP_DEVICE << 16) | EP_VENDOR);
        printk("EP fw: identity set vendor=0x%04x device=0x%04x (read back 0x%08x)\n",
               EP_VENDOR, EP_DEVICE, rd(REG_IDENT));
        printk("EP fw: endpoint magic = 0x%08x\n", rd(REG_MAGIC));

        wr32(REG_SCRATCH, EP_SCRATCH_INIT);
        printk("EP fw: wrote scratch = 0x%08x, read back 0x%08x\n",
               EP_SCRATCH_INIT, rd(REG_SCRATCH));
        printk("EP fw: waiting for the host (Root Port machine)...\n");

        uint32_t last_status = 0xFFFFFFFF, last_bar = 0xFFFFFFFF;
        uint32_t last_scratch = 0xFFFFFFFF, last_wr = 0xFFFFFFFF;

        /* plain polling loop: the machine timer is not usable in Renode's AE350 model */
        while (1) {
                uint32_t st = rd(REG_STATUS);
                uint32_t bar = rd(REG_BAR0);
                uint32_t scr = rd(REG_SCRATCH);
                uint32_t wr = rd(REG_HOSTWR);

                if (bar != last_bar) {
                        printk("EP fw: host programmed BAR0 = 0x%08x\n", bar);
                        last_bar = bar;
                }
                if (st != last_status) {
                        printk("EP fw: status=0x%02x  bar0_set=%u mem_space=%u bus_master=%u msi=%u intx=%u\n",
                               st, st & 1, (st >> 1) & 1, (st >> 2) & 1, (st >> 3) & 1, (st >> 4) & 1);
                        last_status = st;
                }
                if (scr != last_scratch) {
                        printk("EP fw: scratch register = 0x%08x\n", scr);
                        last_scratch = scr;
                }
                if (wr != last_wr) {
                        printk("EP fw: host has made %u BAR write(s)\n", wr);
                        last_wr = wr;
                }
        }
        return 0;
}