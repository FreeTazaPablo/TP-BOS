#ifndef FS_PERSIST_H
#define FS_PERSIST_H

/*
 * fs_persist.h  —  Persistencia del sistema de archivos en disco ATA
 *
 * Layout en disk.img:
 *   Sector 0          : Magic header (4 bytes "TPBF" + 4 bytes checksum)
 *   Sectores 1..524   : Array FSNode[128] serializado (268288 bytes = 524 sectores)
 *
 * Uso:
 *   Al iniciar  → fs_load_from_disk()   (si no hay magic → fs_init() normal)
 *   Tras cambios → fs_save_to_disk()
 */

/* Carga el FS desde disco. Devuelve 1 si cargó datos, 0 si es disco virgen. */
int  fs_load_from_disk(void);

/* Guarda el FS completo al disco. */
void fs_save_to_disk(void);

#endif
