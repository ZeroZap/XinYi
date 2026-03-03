/**
 * @file xy_fs.c
 * @brief File System Abstraction Layer Implementation
 * @version 1.0.0
 * @date 2026-03-01 自主任务
 */

#include "xy_fs.h"
#include "xy_log.h"
#include <string.h>
#include <stdlib.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

#define XY_FS_MAX_DRIVES    4

static xy_fs_t *g_fs_drives[XY_FS_MAX_DRIVES] = {0};

int xy_fs_register(xy_fs_t *fs, const char *name, const xy_fs_ops_t *ops)
{
    if (!fs || !name || !ops) {
        return XY_FS_INVALID_PARAM;
    }
    
    /* 查找空闲槽位 */
    for (int i = 0; i < XY_FS_MAX_DRIVES; i++) {
        if (g_fs_drives[i] == NULL) {
            fs->name = name;
            fs->ops = ops;
            fs->mounted = false;
            g_fs_drives[i] = fs;
            
            xy_log_i("FS registered: %s (slot=%d)\n", name, i);
            return XY_FS_OK;
        }
    }
    
    return XY_FS_FULL;
}

int xy_fs_mount(xy_fs_t *fs, const char *mount_point)
{
    if (!fs || !mount_point) {
        return XY_FS_INVALID_PARAM;
    }
    
    if (fs->mounted) {
        return XY_FS_ERROR;
    }
    
    strncpy(fs->mount_point, mount_point, sizeof(fs->mount_point) - 1);
    
    if (fs->ops->init) {
        int ret = fs->ops->init();
        if (ret != XY_FS_OK) {
            return ret;
        }
    }
    
    fs->mounted = true;
    xy_log_i("FS mounted: %s at %s\n", fs->name, mount_point);
    return XY_FS_OK;
}

int xy_fs_unmount(xy_fs_t *fs)
{
    if (!fs || !fs->mounted) {
        return XY_FS_INVALID_PARAM;
    }
    
    if (fs->ops->deinit) {
        fs->ops->deinit();
    }
    
    fs->mounted = false;
    xy_log_i("FS unmounted: %s\n", fs->name);
    return XY_FS_OK;
}

/**
 * @brief 解析路径，找到对应文件系统
 */
static xy_fs_t* xy_fs_find_drive(const char *path, const char **rel_path)
{
    if (!path || !rel_path) {
        return NULL;
    }
    
    /* 简化实现：假设路径格式为 "0:/path/to/file" */
    if (path[1] == ':') {
        int drive_num = path[0] - '0';
        if (drive_num >= 0 && drive_num < XY_FS_MAX_DRIVES && 
            g_fs_drives[drive_num] && g_fs_drives[drive_num]->mounted) {
            *rel_path = &path[2];  /* 跳过 "0:" */
            if (*rel_path[0] == '/') {
                (*rel_path)++;  /* 跳过前导 / */
            }
            return g_fs_drives[drive_num];
        }
    }
    
    return NULL;
}

int xy_fopen(xy_file_t *file, const char *path, uint8_t mode)
{
    xy_fs_t *fs;
    const char *rel_path;
    
    if (!file || !path) {
        return XY_FS_INVALID_PARAM;
    }
    
    memset(file, 0, sizeof(*file));
    
    fs = xy_fs_find_drive(path, &rel_path);
    if (!fs || !fs->ops->open) {
        return XY_FS_NOT_READY;
    }
    
    int ret = fs->ops->open(file, rel_path, mode);
    if (ret == XY_FS_OK) {
        file->mode = mode;
        file->is_open = true;
    }
    
    return ret;
}

int xy_fclose(xy_file_t *file)
{
    if (!file || !file->is_open) {
        return XY_FS_INVALID_PARAM;
    }
    
    /* TODO: 调用底层 close */
    
    file->is_open = false;
    file->pos = 0;
    file->size = 0;
    
    return XY_FS_OK;
}

int xy_fread(xy_file_t *file, void *buf, uint32_t len, uint32_t *nread)
{
    if (!file || !file->is_open || !buf) {
        return XY_FS_INVALID_PARAM;
    }
    
    /* TODO: 调用底层 read */
    
    return XY_FS_OK;
}

int xy_fwrite(xy_file_t *file, const void *buf, uint32_t len, uint32_t *nwritten)
{
    if (!file || !file->is_open || !buf) {
        return XY_FS_INVALID_PARAM;
    }
    
    /* TODO: 调用底层 write */
    
    return XY_FS_OK;
}

int xy_fseek(xy_file_t *file, uint32_t pos)
{
    if (!file || !file->is_open) {
        return XY_FS_INVALID_PARAM;
    }
    
    file->pos = pos;
    return XY_FS_OK;
}

int xy_fsize(xy_file_t *file, uint32_t *size)
{
    if (!file || !size) {
        return XY_FS_INVALID_PARAM;
    }
    
    *size = file->size;
    return XY_FS_OK;
}

int xy_fgets(xy_file_t *file, char *buf, uint32_t len)
{
    uint32_t i = 0;
    char c;
    
    if (!file || !buf || len == 0) {
        return XY_FS_INVALID_PARAM;
    }
    
    while (i < len - 1) {
        uint32_t nread;
        int ret = xy_fread(file, &c, 1, &nread);
        if (ret != XY_FS_OK || nread == 0) {
            break;
        }
        
        buf[i++] = c;
        
        if (c == '\n') {
            break;
        }
    }
    
    buf[i] = '\0';
    return i > 0 ? i : XY_FS_ERROR;
}

int xy_fputs(xy_file_t *file, const char *str)
{
    if (!file || !str) {
        return XY_FS_INVALID_PARAM;
    }
    
    uint32_t len = strlen(str);
    uint32_t nwritten;
    
    return xy_fwrite(file, str, len, &nwritten);
}

int xy_fs_exists(const char *path)
{
    xy_fs_t *fs;
    const char *rel_path;
    xy_fs_stat_t stat;
    
    fs = xy_fs_find_drive(path, &rel_path);
    if (!fs || !fs->ops->stat) {
        return 0;
    }
    
    return fs->ops->stat(rel_path, &stat) == XY_FS_OK ? 1 : 0;
}

int xy_fs_remove(const char *path)
{
    xy_fs_t *fs;
    const char *rel_path;
    
    fs = xy_fs_find_drive(path, &rel_path);
    if (!fs || !fs->ops->remove) {
        return XY_FS_NOT_READY;
    }
    
    return fs->ops->remove(rel_path);
}

int xy_fs_rename(const char *oldpath, const char *newpath)
{
    xy_fs_t *fs;
    const char *old_rel, *new_rel;
    
    fs = xy_fs_find_drive(oldpath, &old_rel);
    if (!fs || !fs->ops->rename) {
        return XY_FS_NOT_READY;
    }
    
    /* 简化：假设新旧路径在同一驱动器 */
    new_rel = newpath;
    if (newpath[1] == ':') {
        new_rel = &newpath[2];
        if (new_rel[0] == '/') new_rel++;
    }
    
    return fs->ops->rename(old_rel, new_rel);
}
