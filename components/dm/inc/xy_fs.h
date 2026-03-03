/**
 * @file xy_fs.h
 * @brief Lightweight File System Abstraction Layer
 * @version 1.0.0
 * @date 2026-03-01 自主任务
 */

#ifndef XY_FS_H
#define XY_FS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 错误码
 */
#define XY_FS_OK              0
#define XY_FS_ERROR           (-1)
#define XY_FS_INVALID_PARAM   (-2)
#define XY_FS_NOT_FOUND       (-3)
#define XY_FS_EXISTS          (-4)
#define XY_FS_FULL            (-5)
#define XY_FS_NO_MEM          (-6)
#define XY_FS_NOT_READY       (-7)

/**
 * @brief 文件模式
 */
#define XY_FS_READ            0x01
#define XY_FS_WRITE           0x02
#define XY_FS_APPEND          0x04
#define XY_FS_CREATE          0x08

/**
 * @brief 文件类型
 */
typedef enum {
    XY_FS_TYPE_FILE = 0,
    XY_FS_TYPE_DIR,
} xy_fs_type_t;

/**
 * @brief 文件信息
 */
typedef struct {
    char name[32];
    xy_fs_type_t type;
    uint32_t size;
    uint32_t mtime;
} xy_fs_stat_t;

/**
 * @brief 文件句柄
 */
typedef struct {
    void *priv;
    uint32_t pos;
    uint32_t size;
    uint8_t mode;
    bool is_open;
} xy_file_t;

/**
 * @brief 文件系统操作接口
 */
typedef struct {
    int (*init)(void);
    int (*deinit)(void);
    int (*format)(void);
    int (*open)(xy_file_t *file, const char *path, uint8_t mode);
    int (*close)(xy_file_t *file);
    int (*read)(xy_file_t *file, void *buf, uint32_t len, uint32_t *nread);
    int (*write)(xy_file_t *file, const void *buf, uint32_t len, uint32_t *nwritten);
    int (*seek)(xy_file_t *file, uint32_t pos);
    int (*stat)(const char *path, xy_fs_stat_t *stat);
    int (*remove)(const char *path);
    int (*rename)(const char *oldpath, const char *newpath);
} xy_fs_ops_t;

/**
 * @brief 文件系统句柄
 */
typedef struct {
    const char *name;
    const xy_fs_ops_t *ops;
    char mount_point[8];
    bool mounted;
} xy_fs_t;

/**
 * @brief 注册文件系统
 */
int xy_fs_register(xy_fs_t *fs, const char *name, const xy_fs_ops_t *ops);

/**
 * @brief 挂载文件系统
 */
int xy_fs_mount(xy_fs_t *fs, const char *mount_point);

/**
 * @brief 卸载文件系统
 */
int xy_fs_unmount(xy_fs_t *fs);

/**
 * @brief 打开文件
 */
int xy_fopen(xy_file_t *file, const char *path, uint8_t mode);

/**
 * @brief 关闭文件
 */
int xy_fclose(xy_file_t *file);

/**
 * @brief 读取文件
 */
int xy_fread(xy_file_t *file, void *buf, uint32_t len, uint32_t *nread);

/**
 * @brief 写入文件
 */
int xy_fwrite(xy_file_t *file, const void *buf, uint32_t len, uint32_t *nwritten);

/**
 * @brief 定位文件指针
 */
int xy_fseek(xy_file_t *file, uint32_t pos);

/**
 * @brief 获取文件大小
 */
int xy_fsize(xy_file_t *file, uint32_t *size);

/**
 * @brief 读取一行
 */
int xy_fgets(xy_file_t *file, char *buf, uint32_t len);

/**
 * @brief 写入一行
 */
int xy_fputs(xy_file_t *file, const char *str);

/**
 * @brief 检查文件是否存在
 */
int xy_fs_exists(const char *path);

/**
 * @brief 删除文件
 */
int xy_fs_remove(const char *path);

/**
 * @brief 重命名文件
 */
int xy_fs_rename(const char *oldpath, const char *newpath);

#ifdef __cplusplus
}
#endif

#endif
