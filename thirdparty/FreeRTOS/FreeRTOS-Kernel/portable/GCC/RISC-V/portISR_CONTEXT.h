#ifndef RISCV_portISR_CONTEXT_H
#define RISCV_portISR_CONTEXT_H
/*
 * NOTE: This header file is used in both C and ASM.
 *       You'll see the #ifdef  __ASSEMBLY__
 *
 *       This allows a cross check between C data
 *       structures and ASM structure offsets 
 *       to be shared between ASM and C.
 *
 * As a result, be careful of what and how you change this file.
 */
#ifndef __riscv_xlen
#error Missing __riscv_xlen
#endif

#if __riscv_xlen == 64
	#define portWORD_SIZE 8
	#define store_x sd
	#define load_x ld
#elif __riscv_xlen == 32
	#define store_x sw
	#define load_x lw
	#define portWORD_SIZE 4
#else
	#error Assembler did not define __riscv_xlen
#endif

/* integer registers */
#define PORT_REGISTER_SIZEOF        (__riscv_xlen / 8)
#if defined(__riscv_flen)
/* how large are floating point registers */
#define PORT_FPU_REGISTER_SIZEOF  (__riscv_flen / 8)
#endif

#if !defined(portADDITIONAL_CONTEXT_REGS)
/*
 * if the port requires additional thing saved as part
 * of the task context, must be non-zero
 *
 * NOTE: For ABI Stack alignment reasons also a multiple of 4.
 *
 *       Provide a default value that can be used in compile time
 *       non-preprocessor C expressions.
 *       
 */
#define portADDITIONAL_CONTEXT_REGS 0
#endif


/* these index must match the union below, and be visible/usable in ASM code. */
#define PORT_CONTEXT_xIDX(X)      (X) /* index into "raw" for register x? */
#define PORT_CONTEXT_mcauseIDX    (32)
#define PORT_CONTEXT_mepcIDX      (33)
#define PORT_CONTEXT_mstatusIDX   (34)
#define PORT_CONTEXT_mtvectIDX    (35)
//#define PORT_CONTEXT_floatIDX     (35)
#define PORT_CONTEXT_extraIDX(X)  (36+X)
#define PORT_CONTEXT_lastIDX      (36+portADDITIONAL_CONTEXT_REGS)

/* used in assembler, as byte offsets from the start of the context */
#define PORT_CONTEXT_xOFFSET( X )   (PORT_CONTEXT_xIDX(X)    * PORT_REGISTER_SIZEOF)
#define PORT_CONTEXT_mcauseOFFSET   (PORT_CONTEXT_mcauseIDX  * PORT_REGISTER_SIZEOF)
#define PORT_CONTEXT_mepcOFFSET     (PORT_CONTEXT_mepcIDX    * PORT_REGISTER_SIZEOF)
#define PORT_CONTEXT_mstatusOFFSET  (PORT_CONTEXT_mstatusIDX * PORT_REGISTER_SIZEOF)
#define PORT_CONTEXT_mtvectOFFSET   (PORT_CONTEXT_mtvectIDX  * PORT_REGISTER_SIZEOF)
//#define PORT_CONTEXT_floatOFFSET    (PORT_CONTEXT_floatIDX   * PORT_REGISTER_SIZEOF)
#define PORT_CONTEXT_extraOFFSET(X) (PORT_CONTEXT_mepcIDX    * PORT_REGISTER_SIZEOF)
/* total size of the structure usable in ASM. */
#define PORT_CONTEXT_SIZEOF         (PORT_CONTEXT_lastIDX   * PORT_REGISTER_SIZEOF)

/* this file is included into ASM code - so this define is seperate then FreeRTOSConfig.h */
#define configASM_RISCV_USE_FLOAT 0
/* cross check/verify with the "portMACRO.h" file */
#if configRISCV_USE_FLOAT != configASM_RISCV_USE_FLOAT
#error Inconsistant: configRISCV_USE_FLOAT != configASM_RISCV_USE_FLOAT
#endif

/* todo: StaticAsserts() to verify size/location */
#if configASM_RISCV_USE_FLOAT
#if !defined(PORT_FPU_REGISTER_SIZEOF)
#error Missing FPU register size.
#endif
#define RISCV_FLOAT_CONTEXT_SIZEOF  (PORT_FPU_REGISTER_SIZEOF * (32+4))
#define RISCV_FLOAT_freg0_rawIDX   0
#define RISCV_FLOAT_fflagsIDX     32
#define RISCV_FLOAT_frmIDX        33
#define RISCV_FLOAT_fcsrIDX       34

#define RISCV_FLOAT_fregOFFSET(x)   (((X)+RISCV_FLOAT_freg0_rawIDX) * PORT_FPU_REGISTER_SIZEOF)
#define RISCV_FLOAT_fflagsOFFSET    (32 * PORT_FPU_REGISTER_SIZEOF)
#define RISCV_FLOAT_frmOFFSET       (33 * PORT_FPU_REGISTER_SIZEOF)
#define RISCV_FLOAT_fcsrOFFSET      (34 * PORT_FPU_REGISTER_SIZEOF)
#endif

/* Floating point context is stored via a thread local pointer */
#define portRISCV_ASM_FLOAT_THREAD_LOCAL_PTR 1
/* cross check with portMACRO.h */
#if configRISCV_THREAD_LOCAL_FLOAT_PTR != portRISCV_ASM_FLOAT_THREAD_LOCAL_PTR
//#error These must match (configRISCV_THREAD_LOCAL_FLOAT_PTR != portRISCV_ASM_FLOAT_THREAD_LOCAL_PTR)
#endif


#ifndef __ASSEMBLY__
/* these are required for the union below */
#include <stdint.h> /* C99 standar types */
#include <assert.h> /* static assert */

/*
 * This is a IRQ Context - during an interrupt or exception.
 * At an interrupt, this is pushed onto the stack
 *
 * To maintain RISCV ABI stack alignment requirements(16bytes)
 * this data structure must be a MULTIPLE of 16 bytes in size.
 */
union portRISCV_CONTEXT {
    uintptr_t raw[PORT_CONTEXT_lastIDX];
    struct {
	/* for simplicity, position x[0] is not used */
	uintptr_t x[32];
	uintptr_t mcause;  /* 33rd register */
	uintptr_t mepc;    /* 34th register */
	uintptr_t mstatus; /* 35th register */
	uintptr_t mtvect;  /* 36th register */
#if portADDITIONAL_CONTEXT_REGS
#if (portADDITIONAL_CONTEXT_REGS % 4) != 0
#error Must be multiple of 4.
#endif
	uintptr_t extra[ portADDITIONAL_CONTEXT_REGS ];
#endif
    } named;
};

/* sanity check C vrs Assembler offsets */

/* did we pick the correct register size */
_Static_assert( sizeof(uintptr_t) == PORT_REGISTER_SIZEOF, "register size seems wrong" );

/* are x register offsets correct? */
_Static_assert( offsetof( union portRISCV_CONTEXT, named.x[0]) == PORT_CONTEXT_xOFFSET(0), "x0 offset is wrong" );
_Static_assert( offsetof( union portRISCV_CONTEXT, named.x[31]) == PORT_CONTEXT_xOFFSET(31), "x31 offset is wrong" );

/* named registers also? */
_Static_assert( offsetof( union portRISCV_CONTEXT, named.mepc) == PORT_CONTEXT_mepcOFFSET, "mepc offset wrong" );
_Static_assert( offsetof( union portRISCV_CONTEXT, named.mcause) == PORT_CONTEXT_mcauseOFFSET, "mcause offset wrong" );
_Static_assert( offsetof( union portRISCV_CONTEXT, named.mstatus) == PORT_CONTEXT_mstatusOFFSET, "mstatus offset wrong" );
/* we do not check the padding */
#if PORT_CONTEXT_EXTRA_REGS > 0
_Static_assert( offsetof( union portRISCV_CONTEXT, named.extra[0]) == PORT_CONTEXT_extraOFFSET(0), "exra0 offset is wrong" );
_Static_assert( offsetof( union portRISCV_CONTEXT, named.extra[portADDITIONAL_CONTEXT_REGS-1]) == PORT_CONTEXT_extraOFFSET(portADDITIONAL_CONTEXT_REGS-1), "exraN offset is wrong" );
#endif

_Static_assert( sizeof(union portRISCV_CONTEXT) == (PORT_REGISTER_SIZEOF * PORT_CONTEXT_lastIDX), "total context size is wrong");
/* The ABI states the STACK must be aligned on 16byte boundaries
 * Since this element lives on the stack, it too must be aligned in size
 * Reference: https://riscv.org/wp-content/uploads/2015/01/riscv-calling.pdf
 * Section 18.2.
 */
_Static_assert( (sizeof(union portRISCV_CONTEXT) & 0x0f) == 0, "context must be a multiple of 16bytes");


#if configASM_RISCV_USE_FLOAT
/* verify floating point offsets used in asm code */
_Static_assert( sizeof(union portRISCV_FLOAT_CONTEXT) == RISCV_FLOAT_CONTEXT_SIZEOF, "invalid float context size" );
_Static_assert( offsetof(union portRISCV_FLOAT_CONTEXT, named.freg[0] )   == RISCV_FLOAT_fregOFFSET(0) );
_Static_assert( offsetof(union portRISCV_FLOAT_CONTEXT, named.freg[31] )  == RISCV_FLOAT_fregOFFSET(31) );
_Static_assert( offsetof(union portRISCV_FLOAT_CONTEXT, named.fflags)     == RISCV_FLOAT_fflagsOFFSET );
_Static_assert( offsetof(union portRISCV_FLOAT_CONTEXT, named.frm)        == RISCV_FLOAT_frmsOFFSET );
_Static_assert( offsetof(union portRISCV_FLOAT_CONTEXT, named.fcsr)       == RISCV_FLOAT_fcsrOFFSET );
#endif
		
/* this is the common raw assembler trap handler code */
void freertos_risc_v_trap_handler(void);

void portRISCV_ASM_ASSERT(void);

#endif /* __ASSEMBLY__ */

#endif /* RISCV_portISR_CONTEXT_H */
