#pragma once

#include <string.h>

/**
 * OSYSTEM:
 * bit 0..7:  operating system
 * bit 8:     endianess
 * bit 9..15: processor type
 */

#define OSYSTEM_TYPE(os, en, pr) \
	((os & 0xff) | ((en & 1) << 8) | (pr & 0x7f) << 9)
#define OSYSTEM_LE_TYPE(os, pr) \
	OSYSTEM_TYPE(OSYSTEM_OS_##os, 0, OSYSTEM_CPU_##pr)
#define OSYSTEM_BE_TYPE(os, pr) \
	OSYSTEM_TYPE(OSYSTEM_OS_##os, 1, OSYSTEM_CPU_##pr)

#define OSYSTEM_OS_WINDOWS 0
#define OSYSTEM_OS_LINUX 1
#define OSYSTEM_OS_QNX 2
// do not redifine 3 until VA2.0, it has been used for eneo (LinuxEneo) before
#define OSYSTEM_OS_DARWIN 4

#define OSYSTEM_CPU_IA32 0
#define OSYSTEM_CPU_AMD64 1
#define OSYSTEM_CPU_IA64 2

#define OSYSTEM_CPU_ARM7 8
#define OSYSTEM_CPU_ARM9 9
#define OSYSTEM_CPU_ARM10 10
#define OSYSTEM_CPU_ARM11 11

#define OSYSTEM_CPU_XSCALE_PXA25X 16
#define OSYSTEM_CPU_XSCALE_PXA26X 17
#define OSYSTEM_CPU_XSCALE_PXA27X 18
#define OSYSTEM_CPU_XSCALE_PXA3XX 19

#define OSYSTEM_OS(v)	(v & 0xff)
#define OSYSTEM_ENDIANESS(v)	((v >> 8) & 0x1)
#define OSYSTEM_CPU(v)	((v >> 9) & 0x7f)

enum hap_opsys {
	OS_WINDOWS_IA32  = OSYSTEM_LE_TYPE(WINDOWS, IA32),
	OS_LINUX_IA32    = OSYSTEM_LE_TYPE(LINUX, IA32),
	OS_QNX_IA32      = OSYSTEM_LE_TYPE(QNX, IA32),
	OS_LINUX_ENEO    = 3,	// legacy
	OS_DARWIN_IA32   = OSYSTEM_LE_TYPE(DARWIN, IA32),
	OS_WINDOWS_AMD64 = OSYSTEM_LE_TYPE(WINDOWS, AMD64),
	OS_LINUX_AMD64   = OSYSTEM_LE_TYPE(LINUX, AMD64),
	OS_DARWIN_AMD64  = OSYSTEM_LE_TYPE(DARWIN, AMD64),
	OS_INVALID       = 0xffffffff
};

static const struct hap_os_name {
	enum hap_opsys value;
	const char *name;
} hap_os_names[] = {
	{ OS_WINDOWS_IA32, "Windows/IA32" },
	{ OS_LINUX_IA32, "Linux/IA32" },
	{ OS_QNX_IA32, "QNX/IA32" },
	{ OS_LINUX_ENEO, "Linux/Eneo" },
	{ OS_DARWIN_IA32, "Darwin/IA32" },
	{ OS_WINDOWS_AMD64, "Windows/AMD64" },
	{ OS_LINUX_AMD64, "Linux/AMD64" },
	{ OS_DARWIN_AMD64, "Darwin/AMD64" },
	{ OS_INVALID, NULL }
};

static inline enum hap_opsys
hap_opsys_by_name(const char *name)
{
	unsigned int i;

	for (i = 0; hap_os_names[i].name != NULL; i++) {
		if (strcmp(name, hap_os_names[i].name) == 0)
			break;
	}

	return hap_os_names[i].value;
}

static inline const char *
hap_opsys_name(const enum hap_opsys value)
{
	unsigned int i;

	for (i = 0; hap_os_names[i].name != NULL; i++) {
		if (value == hap_os_names[i].value)
			break;
	}

	return hap_os_names[i].name;
}

/**
 * \brief returns the opsys setting for the platform this was compiled on
 */
static inline enum hap_opsys
hap_opsys_of_library(void)
{
#ifdef _WIN64
	return OS_WINDOWS_AMD64;
#elif _WIN32
	return OS_WINDOWS_IA32;
#elif QNX
	return OS_QNX_IA32;
#elif DARWIN
#ifdef __i386__
	return OS_DARWIN_IA32;
#else
	return OS_DARWIN_AMD64;
#endif /* __i386__ */
#elif LINUX
#ifdef _ARM_ERRNO_H
	return OS_LINUX_ENEO;
#elif __i386__
	return OS_LINUX_IA32;
#else
	return OS_LINUX_AMD64;
#endif /* _ARM_ERRNO_H */
#else /* _WIN64 */
#error OS defines not set properly
#endif /* _WIN64  */
}
