# PCIe Root Port to Endpoint on Renode

A PCIe Root Port and Endpoint simulation on two emulated Andes AE350 machines inside one Renode session.
Both machines boot Zephyr RTOS. The Root Port firmware discovers the Endpoint using hand-written MMIO
(no Zephyr PCIe driver API), configures its BAR, does an MMIO round trip and handles INTx and MSI interrupts.

## Architecture

| Part | Where | What it is |
|---|---|---|
| Root Port machine `rp` | `goal4/renode/platforms/pcie_root_port.repl` | Type 1 Root Port, BAR window, MSI window, Root Port firmware |
| Endpoint machine `ep` | `goal4/renode/platforms/pcie_endpoint.repl` | Type 0 Endpoint (BAR0, capability list with MSI), Endpoint firmware |
| PCIe peripherals and connector | `goal4/renode/peripherals/PciePeripherals.cs` | Renode models for the Root Port, Endpoint, BAR/MSI windows, and connector |
| Demo script | `goal4/renode/run.resc` | Creates the connector, both machines, and loads both firmware images |

### Address map

| Component | Address / value |
|---|---|
| Root Port (Type 1 bridge, ECAM) | `0x90000000` |
| Root Port BAR window | `0xA0000000` |
| MSI window | `0xB0000000` |
| Endpoint BAR0 | `0xA0000000`, 4 KB |
| Root Port IDs | vendor `0x1234`, device `0x0001` |
| Endpoint IDs | vendor `0x1234`, device `0x0002` |

## Repository layout

```
goal3/                       Single-machine stub Root Port (raw MMIO enumeration)
goal4/
  firmware/
    root_port/                Root Port Zephyr application
    endpoint/                 Endpoint Zephyr application
  renode/
    platforms/
      pcie_root_port.repl     Root Port machine platform
      pcie_endpoint.repl      Endpoint machine platform
    peripherals/
      PciePeripherals.cs      Root Port, Endpoint, BAR/MSI windows, connector
    reference/
      andes_ae350_n25.repl    Reference copy of the Renode base platform
    run.resc                  Demo script
```

Firmware binaries (`.elf`) are not committed. Build them as described below.

## Base platform file

`pcie_root_port.repl` and `pcie_endpoint.repl` start with:

```
using "platforms/cpus/andes_ae350_n25.repl"
```

That file is **part of the Renode installation**, not of this repository:
`<Renode install>/platforms/cpus/andes_ae350_n25.repl`
(for example `D:\Renode\renode\platforms\cpus\andes_ae350_n25.repl`).
A copy is kept in `goal4/renode/reference/` for reading only.

## Requirements

- Renode 1.17.0
- Zephyr 4.5.0-rc3 with Zephyr SDK 1.0.1 (RISC-V toolchain), built under WSL Ubuntu 24.04
- Board: `adp_xc7k/ae350`

## Build the firmware (Ubuntu / WSL)

The Zephyr workspace is assumed to be at `~/zephyrproject` and this repository at `/mnt/d/1_Project`.

```bash
source ~/zephyrproject/.venv/bin/activate
cd ~/zephyrproject

# Root Port firmware
west build -b adp_xc7k/ae350 /mnt/d/1_Project/goal4/firmware/root_port \
  -d /mnt/d/1_Project/build/goal4_rp -p always

# Endpoint firmware
west build -b adp_xc7k/ae350 /mnt/d/1_Project/goal4/firmware/endpoint \
  -d /mnt/d/1_Project/build/goal4_ep -p always
```

## Run (Windows PowerShell)

The Renode script and firmware paths are relative to the repository root. Start Renode from that
directory so the script can find `goal4/renode/` and the generated `build/` images.

```powershell
Set-Location D:\1_Project
& D:\Renode\renode\renode.exe goal4\renode\run.resc
```

Then type `start` in the Renode monitor. Two UART windows open: `ep:sysbus.uart1` and `rp:sysbus.uart1`.

## Expected output

`rp` window:

- Root Port `00:00.0`, `header_type=0x01`, `class/rev=0x06040001 (bridge)`, bus numbers `0x010100`
- Endpoint `01:00.0`, `vendor=0x1234 device=0x0002`, `capability id=0x05 at 0x40`
- `BAR0 mask=0xfffff000 size=0x1000 bytes`, `BAR0 now=0xa0000000`
- `BAR0 scratch wrote=0x12345678 read=0x12345678 magic=0xe0e0e0e0 OK`
- INTx: ISR ran 1 time; MSI: `data=0x42 plic_claim=21 (expected 21)`
- `GOAL 4 TEST SUMMARY` with all lines `PASS` and `Overall Result: SUCCESS`

`ep` window: `Endpoint machine (ep): Zephyr booted`, then the Endpoint firmware reports what the Root Port
configured (BAR0, status, scratch value, MSI enable).

The Renode log shows `[CONNECTOR] ... forwarded to Endpoint` and `INTx assert/deassert forwarded to Root Port side`
lines, which is independent evidence of the traffic crossing the connector.

## Scope notes

- The Endpoint firmware only observes. All PCIe transactions are driven by the Root Port firmware.
- The BAR window address (`0xA0000000`) is fixed in the `.repl`.
- The Root Port and Endpoint use their own register models, not Renode's `PCIeBasePeripheral` / `IPCIeRouter`.
- The connector also checks the bridge memory window and Memory Space Enable, and blocks MSI when Bus Master Enable is off.

## Known warnings

- `Writing to the Machine Cache Control CSR (0x7ca) ... not supported`: Renode does not implement this Andes CSR.
- `rp/plic` / `ep/plic: Unhandled read/write at offset 0x2000000...`: the base platform's PLIC covers a 64 MB
  range, and the Zephyr Andes PLIC driver touches registers outside the modelled area. Boot, enumeration and
  interrupt tests are not affected.
