/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2015 - ARM Ltd
 * Author: Marc Zyngier <marc.zyngier@arm.com>
 */

#ifndef __ARM64_KVM_HYP_H__
#define __ARM64_KVM_HYP_H__

#include <linux/compiler.h>
#include <linux/kvm_host.h>
#include <asm/alternative.h>
#include <asm/sysreg.h>

DECLARE_PER_CPU(struct kvm_cpu_context, kvm_hyp_ctxt);
DECLARE_PER_CPU(unsigned long, kvm_hyp_vector);
DECLARE_PER_CPU(struct kvm_nvhe_init_params, kvm_init_params);

/*
 * Unified accessors for registers that have a different encoding
 * between VHE and non-VHE. They must be specified without their "ELx"
 * encoding, but with the SYS_ prefix, as defined in asm/sysreg.h.
 */

#if defined(__KVM_VHE_HYPERVISOR__)

#define read_sysreg_el0(r)	read_sysreg_s(r##_EL02)
#define write_sysreg_el0(v,r)	write_sysreg_s(v, r##_EL02)
#define read_sysreg_el1(r)	read_sysreg_s(r##_EL12)
#define write_sysreg_el1(v,r)	write_sysreg_s(v, r##_EL12)
#define read_sysreg_el2(r)	read_sysreg_s(r##_EL1)
#define write_sysreg_el2(v,r)	write_sysreg_s(v, r##_EL1)

#else // !__KVM_VHE_HYPERVISOR__

#if defined(__KVM_NVHE_HYPERVISOR__)
#define VHE_ALT_KEY	ARM64_KVM_HVHE
#else
#define VHE_ALT_KEY	ARM64_HAS_VIRT_HOST_EXTN
#endif

#define read_sysreg_elx(r,nvh,vh)					\
	({								\
		u64 reg;						\
		asm volatile(ALTERNATIVE(__mrs_s("%0", r##nvh),		\
					 __mrs_s("%0", r##vh),		\
					 VHE_ALT_KEY)			\
			     : "=r" (reg));				\
		reg;							\
	})

#define write_sysreg_elx(v,r,nvh,vh)					\
	do {								\
		u64 __val = (u64)(v);					\
		asm volatile(ALTERNATIVE(__msr_s(r##nvh, "%x0"),	\
					 __msr_s(r##vh, "%x0"),		\
					 VHE_ALT_KEY)			\
					 : : "rZ" (__val));		\
	} while (0)

#define read_sysreg_el0(r)	read_sysreg_elx(r, _EL0, _EL02)
#define write_sysreg_el0(v,r)	write_sysreg_elx(v, r, _EL0, _EL02)
#define read_sysreg_el1(r)	read_sysreg_elx(r, _EL1, _EL12)
#define write_sysreg_el1(v,r)	write_sysreg_elx(v, r, _EL1, _EL12)
#define read_sysreg_el2(r)	read_sysreg_elx(r, _EL2, _EL1)
#define write_sysreg_el2(v,r)	write_sysreg_elx(v, r, _EL2, _EL1)

#endif	// __KVM_VHE_HYPERVISOR__

/*
 * Without an __arch_swab32(), we fall back to ___constant_swab32(), but the
 * static inline can allow the compiler to out-of-line this. KVM always wants
 * the macro version as it's always inlined.
 */
#define __kvm_swab32(x)	___constant_swab32(x)

int __vgic_v2_perform_cpuif_access(struct kvm_vcpu *vcpu);

u64 __gic_v3_get_lr(unsigned int lr);

void __vgic_v3_save_state(struct vgic_v3_cpu_if *cpu_if);
void __vgic_v3_restore_state(struct vgic_v3_cpu_if *cpu_if);
void __vgic_v3_activate_traps(struct vgic_v3_cpu_if *cpu_if);
void __vgic_v3_deactivate_traps(struct vgic_v3_cpu_if *cpu_if);
void __vgic_v3_save_vmcr_aprs(struct vgic_v3_cpu_if *cpu_if);
void __vgic_v3_restore_vmcr_aprs(struct vgic_v3_cpu_if *cpu_if);
int __vgic_v3_perform_cpuif_access(struct kvm_vcpu *vcpu);

#ifdef __KVM_NVHE_HYPERVISOR__
void __timer_enable_traps(struct kvm_vcpu *vcpu);
void __timer_disable_traps(struct kvm_vcpu *vcpu);
#endif

#ifdef __KVM_NVHE_HYPERVISOR__
void __sysreg_save_state_nvhe(struct kvm_cpu_context *ctxt);
void __sysreg_restore_state_nvhe(struct kvm_cpu_context *ctxt);
#else
void __vcpu_load_switch_sysregs(struct kvm_vcpu *vcpu);
void __vcpu_put_switch_sysregs(struct kvm_vcpu *vcpu);
void sysreg_save_host_state_vhe(struct kvm_cpu_context *ctxt);
void sysreg_restore_host_state_vhe(struct kvm_cpu_context *ctxt);
void sysreg_save_guest_state_vhe(struct kvm_cpu_context *ctxt);
void sysreg_restore_guest_state_vhe(struct kvm_cpu_context *ctxt);
#endif

void __debug_switch_to_guest(struct kvm_vcpu *vcpu);
void __debug_switch_to_host(struct kvm_vcpu *vcpu);

#ifdef __KVM_NVHE_HYPERVISOR__
void __debug_save_host_buffers_nvhe(struct kvm_vcpu *vcpu);
void __debug_restore_host_buffers_nvhe(struct kvm_vcpu *vcpu);
#endif

void __fpsimd_save_state(struct user_fpsimd_state *fp_regs);
void __fpsimd_restore_state(struct user_fpsimd_state *fp_regs);
void __sve_save_state(void *sve_pffr, u32 *fpsr, int save_ffr);
void __sve_restore_state(void *sve_pffr, u32 *fpsr, int restore_ffr);

u64 __guest_enter(struct kvm_vcpu *vcpu);

bool kvm_host_psci_handler(struct kvm_cpu_context *host_ctxt, u32 func_id);

#ifdef __KVM_NVHE_HYPERVISOR__
void __noreturn __hyp_do_panic(struct kvm_cpu_context *host_ctxt, u64 spsr,
			       u64 elr, u64 par);
#endif

#ifdef __KVM_NVHE_HYPERVISOR__
void __pkvm_init_switch_pgd(phys_addr_t pgd, unsigned long sp,
		void (*fn)(void));
int __pkvm_init(phys_addr_t phys, unsigned long size, unsigned long nr_cpus,
		unsigned long *per_cpu_base, u32 hyp_va_bits);
void __noreturn __host_enter(struct kvm_cpu_context *host_ctxt);
#endif

extern u64 kvm_nvhe_sym(id_aa64pfr0_el1_sys_val);
extern u64 kvm_nvhe_sym(id_aa64pfr1_el1_sys_val);
extern u64 kvm_nvhe_sym(id_aa64isar0_el1_sys_val);
extern u64 kvm_nvhe_sym(id_aa64isar1_el1_sys_val);
extern u64 kvm_nvhe_sym(id_aa64isar2_el1_sys_val);
extern u64 kvm_nvhe_sym(id_aa64mmfr0_el1_sys_val);
extern u64 kvm_nvhe_sym(id_aa64mmfr1_el1_sys_val);
extern u64 kvm_nvhe_sym(id_aa64mmfr2_el1_sys_val);
extern u64 kvm_nvhe_sym(id_aa64smfr0_el1_sys_val);

extern unsigned long kvm_nvhe_sym(__icache_flags);
extern unsigned int kvm_nvhe_sym(kvm_arm_vmid_bits);
extern unsigned int kvm_nvhe_sym(kvm_host_sve_max_vl);

#endif /* __ARM64_KVM_HYP_H__ */
