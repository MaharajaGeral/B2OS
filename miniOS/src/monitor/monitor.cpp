#include "monitor/monitor.h"
#include "drivers/vga.h"
#include "kernel/cpu.h"
#include "proc/scheduler.h"

namespace {

const int KINDS    = 5;
const int TEXT_MAX = 24;       // panjang teks satu baris event
const int BOX_TOP  = 3, BOX_LEFT = 40, BOX_H = 9, BOX_W = 38;
const int HIDE_AFTER = 30;     // 30 x 100 ms = 3 detik tanpa event -> kotak hilang

enum { K_THREAD = 0, K_SWITCH = 1, K_MEM = 2, K_FS = 3, K_ACCESS = 4 };
const char* const LABEL[KINDS] = { "thread", "switch", "memori", "ramfs ", "akses " };

char    last[KINDS][TEXT_MAX + 1];   // event terbaru tiap jenis
bool    dirty = false;               // ada teks yang berubah
bool    wake  = false;               // ada event yang boleh memunculkan/memperpanjang kotak
Thread* self  = nullptr;             // thread monitor sendiri
bool    ready = false;               // true setelah monitor::init selesai

// Event memori/ramfs/akses dicatat dari thread biasa dan shell (id 0). Thread latar dan monitor diabaikan.
bool relevant() {
    if (!ready) return false;
    Thread* c = sched::current();
    if (!c || c == self) return false;
    if (c->is_system && c->id != 0) return false;
    return true;
}

void put(char* d, int& n, const char* s) {
    while (*s && n < TEXT_MAX) d[n++] = *s++;
}

void put_u32(char* d, int& n, u32 v) {
    char t[10];
    int k = 0;
    if (v == 0) t[k++] = '0';
    while (v) { t[k++] = (char)('0' + v % 10); v /= 10; }
    while (k && n < TEXT_MAX) d[n++] = t[--k];
}

const char* short_state(ThreadState s) {
    switch (s) {
        case ThreadState::NEW:        return "NEW";
        case ThreadState::READY:      return "READY";
        case ThreadState::RUNNING:    return "RUN";
        case ThreadState::WAITING:    return "WAIT";
        case ThreadState::TERMINATED: return "DONE";
        default:                      return "-";
    }
}

// Simpan satu baris event. Aman dari interrupt: hanya menyalin ke buffer statis.
void commit(int kind, const char* buf, bool show) {
    u32 f = irq_save();
    int i = 0;
    for (; buf[i] && i < TEXT_MAX; i++) last[kind][i] = buf[i];
    last[kind][i] = '\0';
    dirty = true;
    if (show) wake = true;
    irq_restore(f);
}

void draw() {
    const u8 C = 0x1F;   // putih di biru
    char snap[KINDS][TEXT_MAX + 1];

    u32 f = irq_save();
    for (int k = 0; k < KINDS; k++)
        for (int i = 0; i <= TEXT_MAX; i++) snap[k][i] = last[k][i];
    irq_restore(f);

    for (int r = 0; r < BOX_H; r++) {
        for (int c = 0; c < BOX_W; c++) {
            bool er = (r == 0 || r == BOX_H - 1);
            bool ec = (c == 0 || c == BOX_W - 1);
            char ch = ' ';
            if (er && ec) ch = '+';
            else if (er)  ch = '-';
            else if (ec)  ch = '|';
            vga::overlay_put(r, c, ch, C);
        }
    }
    vga::overlay_print(1, 2, "MONITOR", C);
    for (int k = 0; k < KINDS; k++) {
        vga::overlay_print(3 + k, 2, LABEL[k], C);
        vga::overlay_print(3 + k, 8, ":", C);
        vga::overlay_print(3 + k, 10, snap[k][0] ? snap[k] : "-", C);
    }
}

// Thread latar: tiap 100 ms cek event baru, gambar, dan sembunyikan bila sepi.
void monitor_main(void*) {
    int idle = 0;
    for (;;) {
        sched::sleep_ms(100);

        bool d, w;
        u32 f = irq_save();
        d = dirty; w = wake;
        dirty = false; wake = false;
        irq_restore(f);

        if (w) {
            idle = 0;
            if (!vga::overlay_visible()) {
                vga::overlay_show(BOX_TOP, BOX_LEFT, BOX_H, BOX_W);   // gagal bila mode riwayat
            }
        } else {
            idle++;
        }

        if (vga::overlay_visible()) {
            if (d) draw();
            if (idle > HIDE_AFTER) vga::overlay_hide();
        }
    }
}

}  // namespace

namespace monitor {

void init() {
    self = sched::create_system("monitor", monitor_main, nullptr);
    ready = true;
}

void thread_state(const Thread* t, ThreadState s) {
    if (!t || t == self || t->is_system) return;   // shell, idle, dan thread latar tidak dicatat
    char buf[TEXT_MAX + 1];
    int n = 0;
    put(buf, n, "T");
    put_u32(buf, n, t->id);
    put(buf, n, " ");
    put(buf, n, t->name);
    put(buf, n, " ");
    put(buf, n, short_state(s));
    buf[n] = '\0';
    commit(K_THREAD, buf, true);
}

void context_switch(const Thread* from, const Thread* to) {
    if (!from || !to || from == self || to == self) return;
    char buf[TEXT_MAX + 1];
    int n = 0;
    put(buf, n, from->name);
    put(buf, n, " > ");
    put(buf, n, to->name);
    buf[n] = '\0';
    commit(K_SWITCH, buf, false);   // hanya memperbarui baris, tidak memunculkan kotak
}

void mem_alloc(u32 n) {
    if (!relevant()) return;
    char b[TEXT_MAX + 1]; int k = 0;
    put(b, k, "alloc "); put_u32(b, k, n); put(b, k, " B"); b[k] = '\0';
    commit(K_MEM, b, true);
}

void mem_free(u32 n) {
    if (!relevant()) return;
    char b[TEXT_MAX + 1]; int k = 0;
    put(b, k, "free "); put_u32(b, k, n); put(b, k, " B"); b[k] = '\0';
    commit(K_MEM, b, true);
}

void mem_fail(u32 n) {
    if (!relevant()) return;
    char b[TEXT_MAX + 1]; int k = 0;
    put(b, k, "GAGAL alloc "); put_u32(b, k, n); b[k] = '\0';
    commit(K_MEM, b, true);
}

void mem_bad() {
    if (!relevant()) return;
    commit(K_MEM, "free DITOLAK", true);
}

void fs_event(const char* op, const char* name) {
    if (!relevant()) return;
    char b[TEXT_MAX + 1]; int k = 0;
    put(b, k, op); put(b, k, " "); put(b, k, name); b[k] = '\0';
    commit(K_FS, b, true);
}

void access_event(const char* what) {
    if (!relevant()) return;
    commit(K_ACCESS, what, true);
}

void access_denied() {
    if (!relevant()) return;
    commit(K_ACCESS, "DITOLAK", true);
}

}  // namespace monitor
