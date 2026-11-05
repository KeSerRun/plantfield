# 植物战场 (Plant Battlegrounds)

酒馆战棋式的植物自走棋。C++17 + raylib + Dear ImGui，Windows 原生。

## 玩法循环

园丁选择 → **招募**（拖拽购买 / 上场 / 铲除 / 调整站位，三连融合，浇水刷新，保鲜锁定，升级草坪，园丁技能）
→ **战斗**（逐事件回放：撞击位移、双方同时掉血、破盾与毒杀特效）
→ 结算扣血 → 下一回合。一方血量归零即结束。

- 34 种植物、7 个种族、6 级草坪、6 位园丁；内容全部数据驱动，加植物不改代码
- 关键字：荆棘（嘲讽，必须被优先攻击）、护盾（免疫一次伤害）、毒刺、速生、再生，以及播种 / 凋零 / 光合
- 三连奖励固定给出**高一级草坪**的植物
- 战斗结算确定性：同种子完全可复现（`-c <种子>` 走控制台模式，可批量回归）

## 构建

前置：**VS 2022 Build Tools**（C++ 桌面工作负载）+ CMake ≥ 3.20。编辑器用 VS Code 即可，不需要 VS IDE。

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug      # 或 Release
```

> **不要换成 Ninja。** 中文区域设置下 MSVC 的 `/showIncludes` 前缀（`注意: 包含文件:`）会被错误编码写进 Ninja 的依赖规则，导致输出刷屏且头文件依赖跟踪失效。用 VS 生成器。

首次配置会通过 FetchContent 拉取 raylib 5.5、Dear ImGui 1.92.7、rlImGui。

## 运行

```powershell
.\build\Debug\plant_battlegrounds.exe
```

程序按 `assets/` 相对当前目录查找图片，所以在仓库根目录运行，或把 exe 与 `assets/` 放在一起。

命令行参数：

| 参数 | 说明 |
| --- | --- |
| `-c` | 控制台模式（无窗口，可配合脚本做回归） |
| `-v` | 控制台模式打印战斗全过程 |
| `-a <n>` | 先让 AI 代打 n 回合，再交还操作 |
| `-g <n>` | 指定园丁序号 |
| `-shot <路径>` | 启动后自动截图到指定文件 |
| `<数字>` | 随机种子 |

## 操作

| 场景 | 操作 |
| --- | --- |
| 选园丁 | 点击卡片开始对局 |
| 招募 | 拖拽：商店→手牌 购买 ｜ 手牌→草坪 上场 ｜ 草坪→商店 铲除 ｜ 草坪内拖动 调整站位 |
| 招募 | 草坪区右上角 升级 ｜ 左上角圆框点击放技能 ｜ 商店区右上角 浇水 / 保鲜 |
| 战斗 | 空格 跳到结果 ｜ S 切换播放速度 |
| 通用 | F1 日志 ｜ R 重开 ｜ ESC 退出 |

## 目录结构

```
src/core/    确定性战斗结算、事件队列、效果/光环、卡面文本
src/data/    34 种植物与 6 位园丁的数值定义
src/game/    花园经济与操作、AI、招募/战斗状态机
src/ui/      raylib 棋盘与 ImGui 叠层、拖拽交互、战斗回放、提示浮层
src/platform/ Windows 控制台隔离（避免 windows.h 与 raylib 冲突）
tools/       美术批量生成脚本（ComfyUI）、免安装包打包脚本
assets/      生成好的植物 / 园丁插图
```

## 美术

插图由本地 ComfyUI（Z-Image Turbo）批量生成，脚本在 `tools/`：

```powershell
.\tools\gen_plant_art.ps1      # 34 张植物图，240x320
.\tools\gen_gardener_art.ps1   # 6 张园丁图，320x320
```

脚本需要 ComfyUI 在 `127.0.0.1:8188` 运行。注意脚本内必须是纯 ASCII：PowerShell 5.1 会把无 BOM 的 UTF-8 当 GBK 读。

## 打包分享

```powershell
.\tools\package.ps1
```

产出的 `dist/plant-battlegrounds.zip` 是**免安装绿色版**：Release + 静态链接 MSVC 运行库，只依赖 Windows 系统 DLL，同学解压双击即玩。

## 设计文档

机制取舍、技术选型与开发路线见 [DEV.md](DEV.md)。
