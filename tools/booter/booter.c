// Cargador mínimo de DOL (sin libogc). Vive en MEM2 (0x93000000) para no pisarse al copiar
// el DOL nuevo en MEM1. El juego deja el DOL en 0x92000000 y salta aquí con las interrupciones
// desactivadas. Copia las secciones, borra el BSS, sincroniza cachés y salta al punto de entrada.
typedef unsigned int u32;
typedef unsigned char u8;

#define DOL_ADDR ((const u8*)0x92000000)

typedef struct {
    u32 textOff[7], dataOff[11];
    u32 textAddr[7], dataAddr[11];
    u32 textSize[7], dataSize[11];
    u32 bssAddr, bssSize;
    u32 entry;
} DolHeader;

static void copy(u8* dst, const u8* src, u32 n) {
    while (n--) *dst++ = *src++;
}

static void zero(u8* dst, u32 n) {
    while (n--) *dst++ = 0;
}

static void syncRange(u32 addr, u32 size) {
    u32 start = addr & ~31u, end = addr + size;
    for (u32 a = start; a < end; a += 32) __asm__ volatile("dcbst 0,%0; sync; icbi 0,%0" ::"r"(a));
    __asm__ volatile("sync; isync");
}

void booter_main(void) {
    const DolHeader* h = (const DolHeader*)DOL_ADDR;
    for (int i = 0; i < 7; i++) {
        if (!h->textSize[i]) continue;
        copy((u8*)h->textAddr[i], DOL_ADDR + h->textOff[i], h->textSize[i]);
        syncRange(h->textAddr[i], h->textSize[i]);
    }
    for (int i = 0; i < 11; i++) {
        if (!h->dataSize[i]) continue;
        copy((u8*)h->dataAddr[i], DOL_ADDR + h->dataOff[i], h->dataSize[i]);
        syncRange(h->dataAddr[i], h->dataSize[i]);
    }
    if (h->bssSize) {
        zero((u8*)h->bssAddr, h->bssSize);
        syncRange(h->bssAddr, h->bssSize);
    }
    void (*entry)(void) = (void (*)(void))h->entry;
    entry();
}
