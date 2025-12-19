#include "components/drivers/uart/win32_uart.h"
#include "components/net/xy_at/at.h"
#include <stdio.h>
#include <windows.h>
#include <conio.h>

// 全局变量声明
static WIN32_UART_Handle g_uart_handle;
static at_hdlr_t g_at_handler;
static int g_debug_level = 1; // 调试级别：0=关闭，1=基本，2=详细

// 调试信息打印函数
static void debug_print(int level, const char *format, ...)
{
    if (level <= g_debug_level) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}

// URC处理函数
static void urc_handler(const char *prefix, const char *params)
{
    debug_print(1, "[URC] %s: %s\n", prefix, params);
}

// AT命令响应回调函数
static void at_cmd_callback(AT_Result_t result, const char *resp)
{
    switch (result) {
    case AT_RESULT_OK:
        debug_print(1, "[AT] Success: %s\n", resp);
        break;
    case AT_RESULT_ERROR:
        debug_print(1, "[AT] Error: %s\n", resp);
        break;
    case AT_RESULT_TIMEOUT:
        debug_print(1, "[AT] Timeout\n");
        break;
    default:
        debug_print(1, "[AT] Unknown result\n");
        break;
    }
}

// 按键处理函数
static int process_keyboard(void)
{
    if (_kbhit()) {
        int ch = _getch();
        switch (ch) {
        case 27: // ESC
            return 0;
        case 'd': // 切换调试级别
            g_debug_level = (g_debug_level + 1) % 3;
            debug_print(0, "Debug level changed to %d\n", g_debug_level);
            break;
        case 'q': // 查询信号质量
            at_send(&g_at_handler, "AT+CSQ", "+CSQ:", 1000, at_cmd_callback);
            break;
        case 'r': // 查询网络注册状态
            at_send(&g_at_handler, "AT+CREG?", "+CREG:", 1000, at_cmd_callback);
            break;
        }
    }
    return 1;
}

int main(void)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleOutputCP(CP_UTF8); // 设置控制台UTF8编码

    printf("4G模块AT命令测试程序\n");
    printf("按 'ESC' 退出\n");
    printf("按 'd' 切换调试级别\n");
    printf("按 'q' 查询信号质量\n");
    printf("按 'r' 查询网络注册状态\n");

    // 初始化AT处理器
    at_init(&g_at_handler, NULL);

    // 注册URC处理器
    at_add_urc_hdlr(&g_at_handler, "+CREG", urc_handler);
    at_add_urc_hdlr(&g_at_handler, "+CGREG", urc_handler);
    at_add_urc_hdlr(&g_at_handler, "+CSQ", urc_handler);

    // 初始化串口
    debug_print(1, "正在初始化串口 COM3...\n");
    if (!WIN32_UART_Init(&g_uart_handle, &g_at_handler, "\\\\.\\COM3")) {
        printf("串口初始化失败!\n");
        return -1;
    }

    // 发送初始化命令序列
    debug_print(1, "正在初始化4G模块...\n");
    at_send(&g_at_handler, "AT", "OK", 1000, at_cmd_callback);
    at_send(&g_at_handler, "ATE0", "OK", 1000, at_cmd_callback);
    at_send(&g_at_handler, "AT+CMEE=2", "OK", 1000, at_cmd_callback);

    // 主循环
    while (process_keyboard()) {
        at_process(&g_at_handler);
        Sleep(10);
    }

    // 清理资源
    WIN32_UART_DeInit(&g_uart_handle);
    debug_print(0, "程序已退出\n");

    return 0;
}