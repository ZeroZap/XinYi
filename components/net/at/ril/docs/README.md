# RIL(Radio Interface Layer)
[![License](https://img.shields.io/badge/license-Apache%202-green.svg)](https://gitee.com/moluo-tech/ril/blob/master/LICENSE)[![docs](https://img.shields.io/badge/docs-support-green.svg)](http://moluo-tech.gitee.io/ril)

> 欢迎使用RIL，持续更新中...

## 简介

RIL是一款专门为嵌入式平台开发的无线通信模组(GSM/GPRS/CatM1/NB-Iot)管理软件。它与其它大型框架(例如Android RIL、Wince RIL)不同的是，它被设计成应用于资源受限物联网终端设备（单片机+无线模组的方案），并提供物联网通信所需的基本功能，包含网络注册、连接管理、短信收发及Socket通信。目前已支持EC21、SIM900A、 HL8518等模组，相关的应用也在持续更新中。


## 基本特性

- 包含网络注册、连接、短信收发及Socket通信管理等基本功能。
- 模组驱动使用插件化方式管理，同一个系统中能够挂载适配多个模组，并能够动态选择模组驱动程序。
- 通过简单的修改就能够适用于不同的RTOS平台。
- 内置多种情况下的异常处理机制，保证模组稳定运行。
- 内置常用组件，如带断点续传功能的HTTP文件下载、TFTP传输、MQTT等。

## 系统要求

为了使RIL能够正常运行，目标系统必须满足以下要求:

- ROM 资源：至少 13.5K Bytes(取决于挂载的模组个数及使用的组件)
- RAM 资源：至少 1.2K Bytes(取决于socket创建数量)
- RTOS及堆栈: RIL需要运行在两个任务中，每个任务的堆栈至少 256 Bytes
- 编译器：由于RIL使用了一些C99的特性(柔性数组、内联)，所以编译器需要开启对C99的支持。对于IAR，它默认是打开的，而Keil MDK需要手动增加编译选项(--c99 --gnu) 。

## 软件架构
![软件架构图](/images/SoftwareArchDiagram-zh.png)
## 开发指南

```c
#include "ril.h"
//....
//创建RIL任务

ril_init(&adt, &cfg);     //初始化RIL

ril_use_device("EC21");   //选择模组型号

ril_open();               //打开设备

ril_netconn(true);        //启动网络连接
//...
```

更多详细的使用说明，请参考：
- [快速上手](/quickStart.md)
- [案例演示](/caseShow.md)
- [OS移植文档](/OSPorting.md)
- [模组适配文档](/modulePorting.md)

## 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request
4.  如果你在使用过程中有发现任何问题或者重大BUG，记得在评论区留言，作者将不胜感激！

