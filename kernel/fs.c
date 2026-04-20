#include "fs.h"

static FSNode nodes[FS_MAX_NODES];
static int    root_idx = 0;

/* ── utilidades string mínimas (sin libc) ────────────────────────────────── */
static int k_strlen(const char *s) {
    int n = 0; while (s[n]) n++; return n;
}
static void k_strcpy(char *dst, const char *src) {
    while ((*dst++ = *src++));
}
static int k_strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}
static void k_memset(void *p, int v, int n) {
    unsigned char *b = p; while (n--) *b++ = v;
}

/* ── API pública ─────────────────────────────────────────────────────────── */

void fs_init(void) {
    k_memset(nodes, 0, sizeof(nodes));
    /* Nodo 0 = raíz "#" */
    nodes[0].used       = 1;
    nodes[0].type       = NODE_DIR;
    nodes[0].parent     = -1;
    k_strcpy(nodes[0].name, "#");
    root_idx = 0;
}

int fs_root(void) { return root_idx; }

FSNode *fs_get(int idx) {
    if (idx < 0 || idx >= FS_MAX_NODES) return 0;
    return &nodes[idx];
}

int fs_find(int parent, const char *name) {
    for (int i = 0; i < FS_MAX_NODES; i++) {
        if (nodes[i].used && nodes[i].parent == parent &&
            k_strcmp(nodes[i].name, name) == 0)
            return i;
    }
    return -1;
}

int fs_create(int parent, const char *name, NodeType type) {
    if (fs_find(parent, name) != -1) return -1;   /* ya existe */
    for (int i = 1; i < FS_MAX_NODES; i++) {
        if (!nodes[i].used) {
            k_memset(&nodes[i], 0, sizeof(FSNode));
            nodes[i].used   = 1;
            nodes[i].type   = type;
            nodes[i].parent = parent;
            k_strcpy(nodes[i].name, name);
            return i;
        }
    }
    return -1;   /* lleno */
}

static void fs_remove_recursive(int idx) {
    if (idx < 0 || idx >= FS_MAX_NODES || !nodes[idx].used) return;
    /* Eliminar hijos primero */
    for (int i = 1; i < FS_MAX_NODES; i++) {
        if (nodes[i].used && nodes[i].parent == idx)
            fs_remove_recursive(i);
    }
    k_memset(&nodes[idx], 0, sizeof(FSNode));
}

int fs_remove(int idx) {
    if (idx <= 0 || idx >= FS_MAX_NODES || !nodes[idx].used) return -1;
    fs_remove_recursive(idx);
    return 0;
}

int fs_write(int idx, const char *data) {
    if (idx < 0 || idx >= FS_MAX_NODES || !nodes[idx].used) return -1;
    if (nodes[idx].type != NODE_FILE) return -1;
    int len = k_strlen(data);
    if (len >= FS_MAX_CONTENT) len = FS_MAX_CONTENT - 1;
    k_memset(nodes[idx].content, 0, FS_MAX_CONTENT);
    for (int i = 0; i < len; i++) nodes[idx].content[i] = data[i];
    nodes[idx].content_len = len;
    return 0;
}

const char *fs_read(int idx) {
    if (idx < 0 || idx >= FS_MAX_NODES || !nodes[idx].used) return 0;
    if (nodes[idx].type != NODE_FILE) return 0;
    return nodes[idx].content;
}

int fs_next_child(int parent, int *iter) {
    for (int i = *iter; i < FS_MAX_NODES; i++) {
        if (nodes[i].used && nodes[i].parent == parent) {
            *iter = i + 1;
            return i;
        }
    }
    return -1;
}

void fs_path(int idx, char *buf, int bufsz) {
    if (idx < 0 || bufsz <= 0) return;
    if (idx == root_idx) {
        buf[0] = '#'; buf[1] = '\0';
        return;
    }
    /* Construir path de forma inversa */
    char tmp[FS_PATH_MAX];
    int  pos = 0;
    int  cur = idx;
    while (cur != root_idx && cur != -1) {
        const char *n = nodes[cur].name;
        int nl = k_strlen(n);
        /* Separador ":" */
        if (pos > 0) tmp[pos++] = ':';
        /* Copiar al revés */
        for (int i = nl - 1; i >= 0; i--) {
            if (pos < FS_PATH_MAX - 1) tmp[pos++] = n[i];
        }
        cur = nodes[cur].parent;
    }
    /* Raíz */
    if (pos < FS_PATH_MAX - 1) tmp[pos++] = '#';
    tmp[pos] = '\0';
    /* Invertir */
    int len = pos;
    for (int i = 0; i < len / 2; i++) {
        char t = tmp[i]; tmp[i] = tmp[len-1-i]; tmp[len-1-i] = t;
    }
    int copy = len < bufsz - 1 ? len : bufsz - 1;
    for (int i = 0; i < copy; i++) buf[i] = tmp[i];
    buf[copy] = '\0';
}

int fs_move(int idx, int new_parent) {
    if (idx <= 0 || idx >= FS_MAX_NODES || !nodes[idx].used) return -1;
    if (new_parent < 0 || new_parent >= FS_MAX_NODES || !nodes[new_parent].used) return -1;
    if (nodes[new_parent].type != NODE_DIR) return -1;
    nodes[idx].parent = new_parent;
    return 0;
}

int fs_rename(int idx, const char *new_name) {
    if (idx <= 0 || idx >= FS_MAX_NODES || !nodes[idx].used) return -1;
    k_strcpy(nodes[idx].name, new_name);
    return 0;
}
