/*
 * ps4-which-southbridge - report a PS4's southbridge from userland.
 *
 * This is pure userland: it resolves sysctlbyname from libkernel via the
 * dynlib syscalls (594/591) and calls sysctlbyname("hw.sce_subsys_subid").
 * It never runs kernel code and uses no kernel symbol offsets, so it is
 * safe (no panic risk) on any firmware.
 *
 * The result is shown as a system notification and written to the klog.
 *
 * The output is a raw shellcode payload for Payload Guest (Al-Azif) or any
 * other binloader. See README.md for building.
 */

typedef unsigned int u32;
typedef unsigned long usize;

/*
 * PS4 runs a FreeBSD-derived kernel: syscall number in rax, args in
 * rdi/rsi/rdx/r10/r8/r9, result in rax (rcx/r11 clobbered).
 */
static long sys6(long n, long a, long b, long c, long d, long e, long f)
{
    long ret;
    register long r10 __asm__("r10") = d;
    register long r8 __asm__("r8") = e;
    register long r9 __asm__("r9") = f;
    __asm__ __volatile__("syscall"
                         : "=a"(ret)
                         : "a"(n), "D"(a), "S"(b), "d"(c), "r"(r10), "r"(r8), "r"(r9)
                         : "rcx", "r11", "memory");
    return ret;
}

#define SYS_write 4
#define SYS_dynlib_load_prx 594
#define SYS_dynlib_dlsym 591

static int xwrite(int fd, const void *buf, usize n)
{
    return (int)sys6(SYS_write, fd, (long)buf, n, 0, 0, 0);
}

static long load_prx(const char *path, int *handle)
{
    return sys6(SYS_dynlib_load_prx, (long)path, 0, (long)handle, 0, 0, 0);
}

static long dlsym_h(int handle, const char *name, void **addr)
{
    return sys6(SYS_dynlib_dlsym, handle, (long)name, (long)addr, 0, 0, 0);
}

typedef int (*t_sysctlbyname)(const char *, void *, usize *, const void *, usize);
typedef int (*t_notify)(int, const char *);

static const char *sb_name(u32 v)
{
    switch (v & 0xFFFFFFu) {
    case 0x10100: return "Aeolia A0";
    case 0x10200: return "Aeolia A1";
    case 0x10300: return "Aeolia A2";
    case 0x20100: return "Belize A0";
    case 0x20200: return "Belize B0";
    case 0x30100: return "Baikal A0";
    case 0x30200: return "Baikal B0";
    case 0x30201: return "Baikal B1";
    case 0x40100: return "Belize2 A0";
    default: return "Unknown";
    }
}

static char *put(char *d, const char *s)
{
    while (*s)
        *d++ = *s++;
    *d = 0;
    return d;
}

static char *puthex(char *d, u32 v)
{
    static const char h[] = "0123456789abcdef";
    char tmp[8];
    int n = 0;
    do {
        tmp[n++] = h[v & 0xf];
        v >>= 4;
    } while (v);
    while (n)
        *d++ = tmp[--n];
    *d = 0;
    return d;
}

/*
 * dlopen()/dlsym() cannot be used for libkernel: the loader's dlopen bails
 * out when dynlib_get_info_ex fails on it. Call the dynlib syscalls directly.
 */
static void resolve_libkernel(const char *name, void **out)
{
    int h = 0;
    if (load_prx("libkernel.sprx", &h) == 0 && h)
        dlsym_h(h, name, out);
    if (!*out)
        dlsym_h(0x2001, name, out);
}

static void notify(const char *msg)
{
    int h = 0;
    t_notify fn = (t_notify)0;

    if (load_prx("/system/common/lib/libSceSysUtil.sprx", &h) != 0 || !h)
        load_prx("libSceSysUtil.sprx", &h);
    if (h)
        dlsym_h(h, "sceSysUtilSendSystemNotificationWithText", (void **)&fn);
    if (fn)
        fn(222, msg);
}

int main(void)
{
    t_sysctlbyname sysctlbyname = (t_sysctlbyname)0;
    u32 id = 0;
    usize len = sizeof(id);
    char msg[96];
    char *p;

    resolve_libkernel("sysctlbyname", (void **)&sysctlbyname);

    p = put(msg, "PS4 Southbridge: ");
    if (!sysctlbyname) {
        p = put(p, "sysctlbyname unavailable");
    } else if (sysctlbyname("hw.sce_subsys_subid", &id, &len, 0, 0) != 0) {
        p = put(p, "sysctl failed");
    } else {
        p = put(p, sb_name(id));
        p = put(p, " (0x");
        p = puthex(p, id);
        p = put(p, ")");
    }

    xwrite(1, msg, p - msg);
    xwrite(1, "\n", 1);

    notify(msg);
    return 0;
}
