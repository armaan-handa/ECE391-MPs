extern void init_exception_idt();

extern void eh_divide_by_zero ();
extern void eh_debug ();
extern void eh_nmi ();
extern void eh_breakpoint ();
extern void eh_overflow ();
extern void eh_bound_range ();
extern void eh_opcode ();
extern void eh_device_not_available ();
extern void eh_double_fault ();
extern void eh_invalid_tss ();
extern void eh_segment_not_present ();
extern void eh_stack_segment_fault ();
extern void eh_protection_fault ();
extern void eh_page_fault ();
extern void eh_x87_floating_point ();
extern void eh_alignment_check ();
extern void eh_machine_check ();
extern void eh_simd_floating_point ();
extern void eh_virtualization ();
extern void eh_security ();

extern void eh_unused (); // Unusued
