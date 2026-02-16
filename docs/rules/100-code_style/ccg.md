## 为什么要编码规范


## 编码规范的基本约定

## 命名
### 函数命名
-  小写+下划线
- 内部/系统级函数前面要加下划线开头

### 数据结构命名
C 语言常见数据结构有：结构体，枚举，联合体等
- 小写+下划线

例如
```c
typedef enum {
    error_none = 0,
    error_timeout = 1
}error_t;


typedef struct {
    char *name;
    uint age;
}member_t;

```


## 注释
- 使用英文
- 除非句子太长换行，否则结尾要有句号
- doxygent 的 note 要放在其他注释的下方
- doxygen 的 @} 要单独一行，且 追加 end of 备注
- 类，结构体，枚举，联合体等成员旁边注释格式为 /**< comment */