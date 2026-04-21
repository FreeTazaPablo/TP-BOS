/*
 * ata.c  —  Driver ATA PIO modo 28-bit LBA
 *
 * Usa el disco SECUNDARIO (drive 1 / hdb) para no tocar el CD de boot.
 * En QEMU se mapea con:  -drive file=disk.img,format=raw,index=1,media=disk
 *
 * Registros del controlador primario ATA (0x1F0–0x1F7):
 *   0x1F0  DATA     (16-bit)
 *   0x1F1  FEATURES / ERROR
 *   0x1F2  SECTOR COUNT
 *   0x1F3  LBA LO
 *   0x1F4  LBA MID
 *   0x1F5  LBA HI
 *   0x1F6  DRIVE / HEAD
 *   0x1F7  COMMAND / STATUS
 */

#include "ata.h"

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

/* ── Puertos ────────────────────────────────────────────────────────────── */
#define ATA_DATA        0x1F0
#define ATA_SECCOUNT    0x1F2
#define ATA_LBA_LO      0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HI      0x1F5
#define ATA_DRIVE       0x1F6
#define ATA_CMD         0x1F7
#define ATA_STATUS      0x1F7

#define ATA_CMD_READ    0x20
#define ATA_CMD_WRITE   0x30
#define ATA_CMD_FLUSH   0xE7

/* Drive 1 (secundario) en modo LBA */
#define ATA_DRIVE_SLAVE 0xF0

/* Bits de estado */
#define ATA_SR_BSY  0x80   /* Busy */
#define ATA_SR_DRQ  0x08   /* Data Request */
#define ATA_SR_ERR  0x01   /* Error */

/* ── I/O de puertos (igual que en shell.c) ──────────────────────────────── */
static inline uint8_t inb(uint16_t port) {
    uint8_t v;
    __asm__ volatile ("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}
static inline void outb(uint16_t port, uint8_t v) {
    __asm__ volatile ("outb %0, %1" :: "a"(v), "Nd"(port));
}
static inline uint16_t inw(uint16_t port) {
    uint16_t v;
    __asm__ volatile ("inw %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}
static inline void outw(uint16_t port, uint16_t v) {
    __asm__ volatile ("outw %0, %1" :: "a"(v), "Nd"(port));
}

/* ── Esperar hasta que el disco no esté ocupado ─────────────────────────── */
static void ata_wait_bsy(void) {
    while (inb(ATA_STATUS) & ATA_SR_BSY);
}

/* Esperar hasta que haya datos listos (DRQ) */
static void ata_wait_drq(void) {
    while (!(inb(ATA_STATUS) & ATA_SR_DRQ));
}

/* ── Preparar registro DRIVE/LBA para el drive slave ───────────────────── */
static void ata_select(uint32_t lba) {
    /* 0xF0 = slave | LBA mode | bits 24-27 del LBA */
    outb(ATA_DRIVE, (uint8_t)(ATA_DRIVE_SLAVE | ((lba >> 24) & 0x0F)));
}

/* ── Leer un sector ─────────────────────────────────────────────────────── */
void ata_read_sector(uint32_t lba, uint8_t *buf) {
    ata_wait_bsy();
    ata_select(lba);
    outb(ATA_SECCOUNT, 1);
    outb(ATA_LBA_LO,  (uint8_t)(lba));
    outb(ATA_LBA_MID, (uint8_t)(lba >>  8));
    outb(ATA_LBA_HI,  (uint8_t)(lba >> 16));
    outb(ATA_CMD, ATA_CMD_READ);
    ata_wait_bsy();
    ata_wait_drq();
    /* 256 palabras de 16-bit = 512 bytes */
    for (int i = 0; i < 256; i++) {
        uint16_t w = inw(ATA_DATA);
        buf[i * 2]     = (uint8_t)(w & 0xFF);
        buf[i * 2 + 1] = (uint8_t)(w >> 8);
    }
}

/* ── Escribir un sector ─────────────────────────────────────────────────── */
void ata_write_sector(uint32_t lba, const uint8_t *buf) {
    ata_wait_bsy();
    ata_select(lba);
    outb(ATA_SECCOUNT, 1);
    outb(ATA_LBA_LO,  (uint8_t)(lba));
    outb(ATA_LBA_MID, (uint8_t)(lba >>  8));
    outb(ATA_LBA_HI,  (uint8_t)(lba >> 16));
    outb(ATA_CMD, ATA_CMD_WRITE);
    ata_wait_bsy();
    ata_wait_drq();
    for (int i = 0; i < 256; i++) {
        uint16_t w = (uint16_t)buf[i * 2] | ((uint16_t)buf[i * 2 + 1] << 8);
        outw(ATA_DATA, w);
    }
    /* Flush para asegurar escritura física */
    outb(ATA_CMD, ATA_CMD_FLUSH);
    ata_wait_bsy();
}
