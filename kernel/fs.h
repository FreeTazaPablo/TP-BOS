#ifndef FS_H
#define FS_H

#include "vga.h"

#define FS_MAX_NAME      32
#define FS_MAX_NODES    128
#define FS_MAX_CONTENT 2048   /* bytes por archivo */
#define FS_PATH_MAX     256

typedef enum { NODE_FILE, NODE_DIR } NodeType;

typedef struct FSNode {
    int       used;
    NodeType  type;
    char      name[FS_MAX_NAME];
    int       parent;          /* índice del nodo padre (-1 = raíz) */
    char      content[FS_MAX_CONTENT];
    int       content_len;
} FSNode;

void fs_init(void);

/* Devuelve índice del nodo o -1 si no existe */
int  fs_find(int parent, const char *name);

/* Crea un nodo; devuelve índice o -1 si falla */
int  fs_create(int parent, const char *name, NodeType type);

/* Elimina un nodo (y sus hijos si es dir) */
int  fs_remove(int idx);

/* Escribe contenido en un archivo */
int  fs_write(int idx, const char *data);

/* Lee contenido de un archivo (devuelve puntero interno) */
const char *fs_read(int idx);

/* Devuelve el índice raíz */
int  fs_root(void);

/* Itera hijos de un directorio; *iter empieza en 0 */
int  fs_next_child(int parent, int *iter);

/* Construye la ruta completa de un nodo en buf */
void fs_path(int idx, char *buf, int bufsz);

/* Obtiene el FSNode por índice */
FSNode *fs_get(int idx);

/* Mueve un nodo a otro padre */
int fs_move(int idx, int new_parent);

/* Renombra un nodo */
int fs_rename(int idx, const char *new_name);

#endif
