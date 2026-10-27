/* Read-only: Dell calling interface class 0 select 0 (token read / nv storage read)
 * for each location given; prints status, current, min, max. Never writes. */
#include <stdio.h>
#include <stdlib.h>
#include <smbios_c/smi.h>

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        u32 loc = (u32)strtoul(argv[i], 0, 0);
        u32 args[4] = {loc, 0, 0, 0}, res[4] = {0, 0, 0, 0};
        int rc = dell_simple_ci_smi(0, 0, args, res);
        printf("0x%04x rc=%d status=0x%08x cur=0x%08x(%u) min=0x%08x max=0x%08x\n",
               loc, rc, res[0], res[1], res[1], res[2], res[3]);
    }
    return 0;
}
