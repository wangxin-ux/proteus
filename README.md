# Proteus 仿真项目资源库

集中管理 Proteus 仿真工程、单片机源码、预编译程序和项目使用说明。

资源网站：[Proteus Lab](https://proteus.xinxinyuntu.top)

## 项目目录

| 项目 | 主要器件 | 项目分支 | 网站详情 |
| --- | --- | --- | --- |
| 51 单片机万年历 | AT89C51 / DS1302 / LCD1602 | [51-mcu-perpetual-calendar](https://github.com/wangxin-ux/proteus/tree/51-mcu-perpetual-calendar) | [查看题目与内容](https://proteus.xinxinyuntu.top/resource/1) |
| STM32 单片机密码锁 | STM32F103C6 / LCD1602 / 4×4 矩阵键盘 | [stm32-password-lock](https://github.com/wangxin-ux/proteus/tree/stm32-password-lock) | [查看题目与内容](https://proteus.xinxinyuntu.top/resource/2) |

## 仓库管理方式

- `main`：项目总目录、仓库说明。
- 每个项目使用一个独立分支，分支根目录包含该项目的全部文件及 README。
- 新增项目时，新建对应项目分支，并在本页添加入口。
- 网站每条资源的 GitHub 链接指向对应项目分支。

## 下载单个项目

进入上表中的项目分支，选择 **Code → Download ZIP**；或只克隆所需分支：

```bash
git clone --single-branch --branch 51-mcu-perpetual-calendar https://github.com/wangxin-ux/proteus.git
```

各项目的运行步骤、文件说明、来源和验证范围，见对应分支的 README 与使用说明。
