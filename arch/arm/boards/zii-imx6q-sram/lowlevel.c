#include <debug_ll.h>
#include <common.h>
#include <linux/sizes.h>
#include <mach/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/memory.h>

extern char __dtb_imx6q_zii_sram_start[];

ENTRY_FUNCTION(start_imx6q_sabresd_sram, r0, r1, r2)
{
	void *fdt;

	arm_setup_stack(0x00900000 + SZ_256K - 8);

	imx6_cpu_lowlevel_init();

	fdt = __dtb_imx6q_zii_sram_start - get_runtime_offset();

	barebox_arm_entry(0x00900000, SZ_256K, fdt);
}

void barebox_arm_reset_vector(void) __attribute__((alias ("__start_imx6q_sabresd_sram")));
