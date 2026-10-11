#define FORBIDDEN_SYMBOL_ALLOW_ALL
#include <setjmp.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "ivanoff/emu/cpu.h"
static jmp_buf g_jmp;
static char g_msg[512];
namespace Common {} void error(const char *s, ...) { va_list va; va_start(va, s); vsnprintf(g_msg, sizeof(g_msg), s, va); va_end(va); longjmp(g_jmp, 1); }
void warning(const char *s, ...) {}
void debugN(const char *s, ...) {}
void debug(const char *s, ...) {}
using namespace Ivanoff::Emu;
int main() {
	char line[4096];
	while (fgets(line, sizeof(line), stdin)) {
		unsigned regs[8], fl; char code[64];
		if (sscanf(line, "%x %x %x %x %x %x %x %x %x %63s", &regs[0],&regs[1],&regs[2],&regs[3],&regs[4],&regs[5],&regs[6],&regs[7],&fl,code) != 10) continue;
		Memory mem;
		mem.map(0x100000, 0x10000);
		for (unsigned i = 0; i < 0x10000; i++) mem.w8(0x100000 + i, (i * 7 + 3) & 0xff);
		mem.map(0x200000, 0x1000);
		int n = strlen(code) / 2;
		for (int i = 0; i < n; i++) { unsigned b; sscanf(code + 2 * i, "%2x", &b); mem.w8(0x200000 + i, b); }
		CPU cpu(mem);
		for (int i = 0; i < 8; i++) cpu.r[i] = regs[i];
		cpu.cf = fl & 1; cpu.pf = fl & 4; cpu.af = fl & 0x10; cpu.zf = fl & 0x40; cpu.sf = fl & 0x80; cpu.df = fl & 0x400; cpu.of = fl & 0x800;
		cpu.eip = 0x200000;
		if (setjmp(g_jmp)) { printf("ERR %s\n", g_msg); fflush(stdout); continue; }
		cpu.stepOnce();
		unsigned f = (cpu.cf?1:0)|(cpu.pf?4:0)|(cpu.af?0x10:0)|(cpu.zf?0x40:0)|(cpu.sf?0x80:0)|(cpu.df?0x400:0)|(cpu.of?0x800:0);
		printf("OK");
		for (int i = 0; i < 8; i++) printf(" %08x", cpu.r[i]);
		printf(" %08x %08x", f, cpu.eip);
		for (unsigned i = 0; i < 0x10000; i++) { unsigned v = mem.r8(0x100000 + i); if (v != ((i * 7 + 3) & 0xff)) printf(" %x=%02x", 0x100000 + i, v); }
		printf("\n"); fflush(stdout);
	}
}
#include "common/mutex.h"
class OSystem; OSystem *g_system = nullptr;
namespace Common { Mutex::Mutex() {} Mutex::~Mutex() {} bool Mutex::lock() { return true; } bool Mutex::unlock() { return true; } }
#include "common/ustr.h"
#include "common/util.h"
namespace Common { String U32String::encode(CodePage) const { return String(); } bool isDigit(int c) { return c>='0'&&c<='9'; } bool isPrint(int c) { return c>=32&&c<127; } bool isSpace(int c) { return c==' '||c=='\t'||c=='\n'||c=='\r'; } }
