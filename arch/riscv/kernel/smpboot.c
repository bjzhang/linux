// SPDX-License-Identifier: GPL-2.0-only
/*
 * SMP initialisation and IPI support
 * Based on arch/arm64/kernel/smp.c
 *
 * Copyright (C) 2012 ARM Ltd.
 * Copyright (C) 2015 Regents of the University of California
 * Copyright (C) 2017 SiFive
 */

#include <linux/arch_topology.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/kernel_stat.h>
#include <linux/notifier.h>
#include <linux/cpu.h>
#include <linux/percpu.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/irq.h>
#include <linux/of.h>
#include <linux/sched/task_stack.h>
#include <linux/sched/mm.h>
#include <asm/cpu_ops.h>
#include <asm/irq.h>
#include <asm/mmu_context.h>
#include <asm/tlbflush.h>
#include <asm/sections.h>
#include <asm/sbi.h>
#include <asm/smp.h>

#include "head.h"

static DECLARE_COMPLETION(cpu_running);

void __init smp_prepare_boot_cpu(void)
{
	init_cpu_topology();
}

void __init smp_prepare_cpus(unsigned int max_cpus)
{
	int cpuid;
	int ret;

	/* This covers non-smp usecase mandated by "nosmp" option */
	if (max_cpus == 0)
		return;

	for_each_possible_cpu(cpuid) {
		if (cpuid == smp_processor_id())
			continue;
		if (cpu_ops[cpuid]->cpu_prepare) {
			ret = cpu_ops[cpuid]->cpu_prepare(cpuid);
			if (ret)
				continue;
		}
		set_cpu_present(cpuid, true);
	}
}

void __init setup_smp(void)
{
	struct device_node *dn;
	int hart;
	bool found_boot_cpu = false;
	int cpuid = 1;

	cpu_set_ops(0);

	for_each_of_cpu_node(dn) {
		hart = riscv_of_processor_hartid(dn);
		if (hart < 0)
			continue;

		if (hart == cpuid_to_hartid_map(0)) {
			BUG_ON(found_boot_cpu);
			found_boot_cpu = 1;
			continue;
		}
		if (cpuid >= NR_CPUS) {
			pr_warn("Invalid cpuid [%d] for hartid [%d]\n",
				cpuid, hart);
			break;
		}

		cpuid_to_hartid_map(cpuid) = hart;
		cpuid++;
	}

	BUG_ON(!found_boot_cpu);

	if (cpuid > nr_cpu_ids)
		pr_warn("Total number of cpus [%d] is greater than nr_cpus option value [%d]\n",
			cpuid, nr_cpu_ids);

	for (cpuid = 1; cpuid < nr_cpu_ids; cpuid++) {
		if (cpuid_to_hartid_map(cpuid) != INVALID_HARTID) {
			cpu_set_ops(cpuid);
			set_cpu_possible(cpuid, true);
		}
	}
}

static int start_secondary_cpu(int cpu, struct task_struct *tidle)
{
	pr_info("%s\n", __func__);
	if (cpu_ops[cpu]->cpu_start)
		return cpu_ops[cpu]->cpu_start(cpu, tidle);

	pr_info("%s\n", __func__);
	return -EOPNOTSUPP;
}

static void *rtt_image_vaddr;
static void *rtt_start_vaddr;
void copy_rtt(void)
{
#define RTT_IMAGE_ADDR	(0x90100000)
#define RTT_SIZE	(0x100000)
#define RTT_START_ADDR	(0x90200000)
	if (!rtt_image_vaddr) {
		rtt_image_vaddr = memremap(RTT_IMAGE_ADDR, RTT_SIZE, MEMREMAP_WB);
		pr_info("Allocated reserved memory, rt-thread image vaddr: 0x%0llX, paddr: 0x%0llX\n", (u64)rtt_image_vaddr, (u64)RTT_IMAGE_ADDR);
		rtt_start_vaddr = memremap(RTT_START_ADDR, RTT_SIZE, MEMREMAP_WB);
		pr_info("Allocated reserved memory, rt-thread start vaddr: 0x%0llX, paddr: 0x%0llX\n", (u64)rtt_start_vaddr, (u64)RTT_START_ADDR);
	}
	memcpy(rtt_start_vaddr,  rtt_image_vaddr, RTT_SIZE);
	pr_info("Copy rt-thread done\n");
}

int __cpu_up(unsigned int cpu, struct task_struct *tidle)
{
	int ret = 0;
	tidle->thread_info.cpu = cpu;

	copy_rtt();
	ret = start_secondary_cpu(cpu, tidle);
	if (!ret) {
		pr_info("CPU%u: online\n", cpu);
		wait_for_completion_timeout(&cpu_running,
					    msecs_to_jiffies(1000));

		if (!cpu_online(cpu)) {
			pr_crit("CPU%u: failed to come online\n", cpu);
			ret = -EIO;
		}
	} else {
		pr_crit("CPU%u: failed to start\n", cpu);
	}

	return ret;
}

void __init smp_cpus_done(unsigned int max_cpus)
{
}

/*
 * C entry point for a secondary processor.
 */
asmlinkage __visible void smp_callin(void)
{
	struct mm_struct *mm = &init_mm;
	unsigned int curr_cpuid = smp_processor_id();

	riscv_clear_ipi();

	/* All kernel threads share the same mm context.  */
	mmgrab(mm);
	current->active_mm = mm;

	notify_cpu_starting(curr_cpuid);
	update_siblings_masks(curr_cpuid);
	set_cpu_online(curr_cpuid, 1);

	/*
	 * Remote TLB flushes are ignored while the CPU is offline, so emit
	 * a local TLB flush right now just in case.
	 */
	local_flush_tlb_all();
	complete(&cpu_running);
	/*
	 * Disable preemption before enabling interrupts, so we don't try to
	 * schedule a CPU that hasn't actually started yet.
	 */
	preempt_disable();
	local_irq_enable();
	cpu_startup_entry(CPUHP_AP_ONLINE_IDLE);
}
