/*
 * fs_persist.c  —  Persistencia del FS en disco ATA secundario
 *
 * Layout en disk.img:
 *   Sector 0        : 512 bytes — Magic "TPBF" (4 bytes) + reserved (508 bytes)
 *   Sectores 1..524 : FSNode[128] raw — 268288 bytes, 524 sectores de 512
 *
 * La serialización es trivial: el array nodes[] tiene tamaño fijo en
 * tiempo de compilación, así que escribimos/leemos los bytes directamente.
 */

#include "fs.h"
#include "ata.h"
#include "fs_persist.h"

typedef unsigned char  uint8_t;
typedef unsigned int   uint32_t;

/* ── Constantes ────────────────────────────────────────────────────────── */
#define FS_MAGIC_0  'T'
#define FS_MAGIC_1  'P'
#define FS_MAGIC_2  'B'
#define FS_MAGIC_3  'F'

#define FS_DATA_START_LBA   1          /* primer sector de datos */

/* Tamaño total del array en bytes y sectores */
#define FS_ARRAY_BYTES   (FS_MAX_NODES * (int)sizeof(FSNode))
#define FS_ARRAY_SECTORS ((FS_ARRAY_BYTES + 511) / 512)

/* ── Sector buffer temporal (512 bytes en stack) ────────────────────────── */

/* Copia len bytes de src a dst sin libc */
static void p_memcpy(uint8_t *dst, const uint8_t *src, int len) {
    while (len--) *dst++ = *src++;
}
static void p_memset(uint8_t *dst, uint8_t v, int len) {
    while (len--) *dst++ = v;
}

/* ── Guardar ────────────────────────────────────────────────────────────── */
void fs_save_to_disk(void) {
    uint8_t sector[512];

    /* 1. Escribir magic en sector 0 */
    p_memset(sector, 0, 512);
    sector[0] = FS_MAGIC_0;
    sector[1] = FS_MAGIC_1;
    sector[2] = FS_MAGIC_2;
    sector[3] = FS_MAGIC_3;
    ata_write_sector(0, sector);

    /* 2. Serializar FSNode[] sector a sector */
    const uint8_t *src = (const uint8_t *)fs_get(0); /* puntero al inicio del array */

    for (int s = 0; s < FS_ARRAY_SECTORS; s++) {
        int offset = s * 512;
        int remaining = FS_ARRAY_BYTES - offset;
        int chunk = (remaining >= 512) ? 512 : remaining;

        p_memset(sector, 0, 512);
        p_memcpy(sector, src + offset, chunk);
        ata_write_sector(FS_DATA_START_LBA + s, sector);
    }
}

/* ── Cargar ─────────────────────────────────────────────────────────────── */
int fs_load_from_disk(void) {
    uint8_t sector[512];

    /* 1. Leer sector 0 y verificar magic */
    ata_read_sector(0, sector);

    if (sector[0] != FS_MAGIC_0 ||
        sector[1] != FS_MAGIC_1 ||
        sector[2] != FS_MAGIC_2 ||
        sector[3] != FS_MAGIC_3) {
        /* Disco virgen o no formateado → arrancar con FS vacío */
        fs_init();
        return 0;
    }

    /* 2. Leer los sectores de datos directo al array interno */
    uint8_t *dst = (uint8_t *)fs_get(0);

    for (int s = 0; s < FS_ARRAY_SECTORS; s++) {
        ata_read_sector(FS_DATA_START_LBA + s, sector);
        int offset    = s * 512;
        int remaining = FS_ARRAY_BYTES - offset;
        int chunk     = (remaining >= 512) ? 512 : remaining;
        p_memcpy(dst + offset, sector, chunk);
    }

    return 1;   /* FS cargado desde disco */
}
