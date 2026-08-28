#include "unity.h"

#include <stdio.h>
#include <string.h>

#include "xy_fs.h"

static xy_fs_t g_fs;
static char g_data[64];
static size_t g_size;
static int g_init_result;
static int g_deinit_result;
static int g_close_result;

void xy_log_char(char ch)
{
    (void)ch;
}

static int fake_init(void)
{
    return g_init_result;
}

static int fake_deinit(void)
{
    return g_deinit_result;
}

static int fake_open(xy_fs_file_t *file, const char *path, uint8_t mode)
{
    (void)mode;
    if (strcmp(path, "data.bin") != 0) {
        return XY_FS_NOT_FOUND;
    }
    file->priv = g_data;
    return XY_FS_OK;
}

static int fake_close(xy_fs_file_t *file)
{
    (void)file;
    return g_close_result;
}

static int fake_read(xy_fs_file_t *file, void *buf, size_t len)
{
    size_t available = g_size - file->pos;
    size_t count = len < available ? len : available;

    memcpy(buf, &g_data[file->pos], count);
    return (int)count;
}

static int fake_write(xy_fs_file_t *file, const void *buf, size_t len)
{
    if (file->pos + len > sizeof(g_data)) {
        return XY_FS_FULL;
    }
    memcpy(&g_data[file->pos], buf, len);
    if (file->pos + len > g_size) {
        g_size = file->pos + len;
    }
    return (int)len;
}

static int fake_stat(const char *path, xy_fs_stat_t *stat)
{
    if (strcmp(path, "data.bin") != 0) {
        return XY_FS_NOT_FOUND;
    }
    memset(stat, 0, sizeof(*stat));
    stat->size = (uint32_t)g_size;
    return XY_FS_OK;
}

static int fake_remove(const char *path)
{
    return strcmp(path, "data.bin") == 0 ? XY_FS_OK : XY_FS_NOT_FOUND;
}

static int fake_rename(const char *old_path, const char *new_path)
{
    return strcmp(old_path, "data.bin") == 0 && strcmp(new_path, "renamed.bin") == 0
               ? XY_FS_OK
               : XY_FS_NOT_FOUND;
}

static const xy_fs_ops_t g_ops = {
    .init = fake_init,
    .deinit = fake_deinit,
    .open = fake_open,
    .close = fake_close,
    .read = fake_read,
    .write = fake_write,
    .stat = fake_stat,
    .remove = fake_remove,
    .rename = fake_rename,
};

void setUp(void)
{
    g_init_result = XY_FS_OK;
    g_deinit_result = XY_FS_OK;
    g_close_result = XY_FS_OK;
}

void tearDown(void)
{
}

static void test_fs_lifecycle_and_invalid_parameters(void)
{
    xy_fs_t unregistered = {0};

    TEST_ASSERT_EQUAL_INT(XY_FS_INVALID_PARAM, xy_fs_register(NULL, "mem", &g_ops));
    TEST_ASSERT_EQUAL_INT(XY_FS_INVALID_PARAM, xy_fs_register(&g_fs, NULL, &g_ops));
    TEST_ASSERT_EQUAL_INT(XY_FS_INVALID_PARAM, xy_fs_mount(NULL, "0:"));
    TEST_ASSERT_EQUAL_INT(XY_FS_INVALID_PARAM, xy_fs_mount(&unregistered, "0:"));
    TEST_ASSERT_NULL(xy_fs_open(NULL, XY_FS_MODE_READ));
    TEST_ASSERT_NULL(xy_fs_open("", XY_FS_MODE_READ));

    memset(&g_fs, 0, sizeof(g_fs));
    TEST_ASSERT_EQUAL_INT(XY_FS_OK, xy_fs_register(&g_fs, "mem", &g_ops));

    g_init_result = XY_FS_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_FS_ERROR, xy_fs_mount(&g_fs, "0:"));
    TEST_ASSERT_FALSE(g_fs.mounted);

    g_init_result = XY_FS_OK;
    TEST_ASSERT_EQUAL_INT(XY_FS_OK, xy_fs_mount(&g_fs, "0:"));
    TEST_ASSERT_TRUE(g_fs.mounted);
    TEST_ASSERT_EQUAL_INT(XY_FS_ERROR, xy_fs_mount(&g_fs, "0:"));

    g_deinit_result = XY_FS_ERROR;
    TEST_ASSERT_EQUAL_INT(XY_FS_ERROR, xy_fs_unmount(&g_fs));
    TEST_ASSERT_TRUE(g_fs.mounted);
    g_deinit_result = XY_FS_OK;
    TEST_ASSERT_EQUAL_INT(XY_FS_OK, xy_fs_unmount(&g_fs));
    TEST_ASSERT_FALSE(g_fs.mounted);
}

static void test_fs_io_helpers_and_error_contracts(void)
{
    xy_fs_file_t *file;
    char out[8] = {0};
    size_t actual = 99U;
    uint32_t size = 99U;

    TEST_ASSERT_EQUAL_INT(XY_FS_OK, xy_fs_mount(&g_fs, "0:"));
    memcpy(g_data, "hello", 5U);
    g_size = 5U;

    file = xy_fs_open("0:/data.bin", XY_FS_MODE_READ | XY_FS_MODE_WRITE);
    TEST_ASSERT_NOT_NULL(file);
    TEST_ASSERT_EQUAL_INT(5, xy_fs_read(file, out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("hello", out);
    TEST_ASSERT_EQUAL_INT(5, xy_fs_tell(file));
    TEST_ASSERT_EQUAL_INT(XY_FS_INVALID_PARAM, xy_fs_seek(file, -1L, SEEK_SET));
    TEST_ASSERT_EQUAL_INT(5, xy_fs_tell(file));
    TEST_ASSERT_EQUAL_INT(XY_FS_INVALID_PARAM, xy_fs_seek(file, 0L, 99));
    TEST_ASSERT_EQUAL_INT(XY_FS_OK, xy_fs_seek(file, 0L, SEEK_SET));
    TEST_ASSERT_EQUAL_INT(2, xy_fs_write(file, "XY", 2U));
    TEST_ASSERT_EQUAL_INT(XY_FS_OK, xy_fs_close(file));

    TEST_ASSERT_EQUAL_INT(XY_FS_OK, xy_fs_size("0:/data.bin", &size));
    TEST_ASSERT_EQUAL_UINT32(5U, size);
    TEST_ASSERT_TRUE(xy_fs_exists("0:/data.bin"));
    TEST_ASSERT_EQUAL_INT(XY_FS_OK, xy_fs_remove("0:/data.bin"));
    TEST_ASSERT_EQUAL_INT(XY_FS_OK, xy_fs_rename("0:/data.bin", "0:/renamed.bin"));
    TEST_ASSERT_EQUAL_INT(XY_FS_NOT_SUPPORTED,
                          xy_fs_rename("0:/data.bin", "1:/renamed.bin"));

    memset(out, 0, sizeof(out));
    TEST_ASSERT_EQUAL_INT(XY_FS_OK,
                          xy_fs_read_file("0:/data.bin", out, sizeof(out), &actual));
    TEST_ASSERT_EQUAL_UINT(5U, actual);
    TEST_ASSERT_EQUAL_MEMORY("XYllo", out, 5U);
    TEST_ASSERT_EQUAL_INT(XY_FS_OK, xy_fs_write_file("0:/data.bin", "abc", 3U));

    g_close_result = XY_FS_ERROR;
    actual = 77U;
    TEST_ASSERT_EQUAL_INT(XY_FS_ERROR,
                          xy_fs_read_file("0:/data.bin", out, sizeof(out), &actual));
    TEST_ASSERT_EQUAL_UINT(77U, actual);
    TEST_ASSERT_EQUAL_INT(XY_FS_ERROR, xy_fs_write_file("0:/data.bin", "z", 1U));

    g_close_result = XY_FS_OK;
    TEST_ASSERT_EQUAL_INT(XY_FS_OK, xy_fs_unmount(&g_fs));
    TEST_ASSERT_EQUAL_INT(XY_FS_NOT_MOUNTED, xy_fs_size("0:/data.bin", &size));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_fs_lifecycle_and_invalid_parameters);
    RUN_TEST(test_fs_io_helpers_and_error_contracts);
    return UNITY_END();
}
