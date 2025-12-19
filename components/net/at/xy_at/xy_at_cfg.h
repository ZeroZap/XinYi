/**
 * @file xy_at_cfg.h
 * @brief XY AT Framework Configuration
 * @version 1.0.0
 * @date 2025-10-27
 */

#ifndef _XY_AT_CFG_H_
#define _XY_AT_CFG_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Feature Selection ==================== */

/**
 * @brief Enable AT client
 * @note Set to 1 to enable AT client functionality
 */
#ifndef XY_AT_USING_CLIENT
#define XY_AT_USING_CLIENT 1
#endif

/**
 * @brief Enable AT server
 * @note Set to 1 to enable AT server functionality
 */
#ifndef XY_AT_USING_SERVER
#define XY_AT_USING_SERVER 1
#endif

/**
 * @brief Enable debug output
 * @note Set to 1 to enable debug logging
 */
#ifndef XY_AT_DEBUG
#define XY_AT_DEBUG 0
#endif

/**
 * @brief Print raw AT commands
 * @note Set to 1 to print raw commands for debugging
 */
#ifndef XY_AT_PRINT_RAW_CMD
#define XY_AT_PRINT_RAW_CMD 0
#endif

/* ==================== Client Configuration ==================== */

#ifdef XY_AT_USING_CLIENT

/**
 * @brief Maximum number of AT clients
 */
#ifndef XY_AT_CLIENT_NUM_MAX
#define XY_AT_CLIENT_NUM_MAX 1
#endif

/**
 * @brief Maximum AT command length
 */
#ifndef XY_AT_CMD_MAX_LEN
#define XY_AT_CMD_MAX_LEN 256
#endif

/**
 * @brief Maximum response buffer length
 */
#ifndef XY_AT_RESP_MAX_LEN
#define XY_AT_RESP_MAX_LEN 1024
#endif

/**
 * @brief Maximum receive line length
 */
#ifndef XY_AT_RECV_LINE_MAX_LEN
#define XY_AT_RECV_LINE_MAX_LEN 256
#endif

/**
 * @brief Default command timeout (milliseconds)
 */
#ifndef XY_AT_DEFAULT_TIMEOUT
#define XY_AT_DEFAULT_TIMEOUT 5000
#endif

/**
 * @brief Maximum retry count
 */
#ifndef XY_AT_MAX_RETRY
#define XY_AT_MAX_RETRY 3
#endif

/**
 * @brief Maximum URC handler table size
 */
#ifndef XY_AT_URC_TABLE_MAX
#define XY_AT_URC_TABLE_MAX 16
#endif

/**
 * @brief Client parser thread stack size
 */
#ifndef XY_AT_CLIENT_THREAD_STACK_SIZE
#define XY_AT_CLIENT_THREAD_STACK_SIZE 1024
#endif

/**
 * @brief Client parser thread priority
 */
#ifndef XY_AT_CLIENT_THREAD_PRIORITY
#define XY_AT_CLIENT_THREAD_PRIORITY 24 /* XY_OS_PRIORITY_NORMAL */
#endif

#endif /* XY_AT_USING_CLIENT */

/* ==================== Server Configuration ==================== */

#ifdef XY_AT_USING_SERVER

/**
 * @brief Server receive buffer size
 */
#ifndef XY_AT_SERVER_RECV_BUF_SIZE
#define XY_AT_SERVER_RECV_BUF_SIZE 256
#endif

/**
 * @brief Server send buffer size
 */
#ifndef XY_AT_SERVER_SEND_BUF_SIZE
#define XY_AT_SERVER_SEND_BUF_SIZE 512
#endif

/**
 * @brief Maximum command name length
 */
#ifndef XY_AT_CMD_NAME_MAX_LEN
#define XY_AT_CMD_NAME_MAX_LEN 16
#endif

/**
 * @brief Maximum command table size
 */
#ifndef XY_AT_CMD_TABLE_MAX
#define XY_AT_CMD_TABLE_MAX 32
#endif

/**
 * @brief Server parser thread stack size
 */
#ifndef XY_AT_SERVER_THREAD_STACK_SIZE
#define XY_AT_SERVER_THREAD_STACK_SIZE 1024
#endif

/**
 * @brief Server parser thread priority
 */
#ifndef XY_AT_SERVER_THREAD_PRIORITY
#define XY_AT_SERVER_THREAD_PRIORITY 24 /* XY_OS_PRIORITY_NORMAL */
#endif

/**
 * @brief Enable echo mode by default
 */
#ifndef XY_AT_SERVER_ECHO_MODE
#define XY_AT_SERVER_ECHO_MODE 1
#endif

#endif /* XY_AT_USING_SERVER */

/* ==================== Response End Markers ==================== */

/**
 * @brief OK response string
 */
#ifndef XY_AT_RESP_OK_STR
#define XY_AT_RESP_OK_STR "OK"
#endif

/**
 * @brief ERROR response string
 */
#ifndef XY_AT_RESP_ERROR_STR
#define XY_AT_RESP_ERROR_STR "ERROR"
#endif

/**
 * @brief Line ending for commands
 */
#ifndef XY_AT_CMD_LINE_END
#define XY_AT_CMD_LINE_END "\r\n"
#endif

/**
 * @brief Command prefix
 */
#ifndef XY_AT_CMD_PREFIX
#define XY_AT_CMD_PREFIX "AT"
#endif

/* ==================== Memory Configuration ==================== */

/**
 * @brief Use dynamic memory allocation
 * @note If set to 0, static buffers will be used
 */
#ifndef XY_AT_USING_DYNAMIC_MEMORY
#define XY_AT_USING_DYNAMIC_MEMORY 1
#endif

/* ==================== Validation ==================== */

#if defined(XY_AT_USING_CLIENT) && (XY_AT_CLIENT_NUM_MAX < 1)
#error "XY_AT_CLIENT_NUM_MAX must be at least 1"
#endif

#if defined(XY_AT_USING_SERVER) && (XY_AT_CMD_TABLE_MAX < 1)
#error "XY_AT_CMD_TABLE_MAX must be at least 1"
#endif

#if !defined(XY_AT_USING_CLIENT) && !defined(XY_AT_USING_SERVER)
#error \
    "At least one of XY_AT_USING_CLIENT or XY_AT_USING_SERVER must be enabled"
#endif

#ifdef __cplusplus
}
#endif

#endif /* _XY_AT_CFG_H_ */
