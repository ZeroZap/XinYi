# 函数文档模板

## Doxygen 格式
```c
/**
 * @brief   函数简要描述
 * @details 详细描述（可选）
 *
 * @param[in]     input_param   输入参数描述
 * @param[out]    output_param  输出参数描述
 * @param[in,out] inout_param   输入输出参数描述
 *
 * @return  返回值描述
 * @retval  0       成功
 * @retval  -1      失败：参数无效
 * @retval  -2      失败：内存不足
 *
 * @note    注意事项
 * @warning 警告信息
 * @see     相关函数
 *
 * @code
 * // 使用示例
 * int result = function_name(input, &output);
 * if (result != 0) {
 *     // 错误处理
 * }
 * @endcode
 *
 * @author  作者名
 * @date    2026-02-25
 * @version 1.0
 */
int function_name(int input_param, int *output_param);