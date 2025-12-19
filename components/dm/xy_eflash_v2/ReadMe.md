## extend
- flash read/write more than 8 bytes
  - no more than 8 bytes

## 额外想法
- 不同的数据长度，直接做成一个等长的log
  - addr 成了 唯一 ID



## 使用限制

- 最小编程单元为 32bit 及以下
  - 如果超过 32bit 的，需要底层适配
- eeprom 无法支持 32位写入原子操作，则要先写低16位后写高16位

