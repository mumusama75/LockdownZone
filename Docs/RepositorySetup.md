# 仓库复现与资源边界

本次同步当前 Source、Config、Scripts 和已整理的项目资产，未修改玩法。美术资源库与可执行试玩包不是同一交付。

## 本地构建

需要 Unreal Engine 5.8、Visual Studio C++ / Windows SDK。检查脚本中的引擎路径与项目路径；部分脚本仍使用本机 `C:/UEProjects/LockdownZoneRepo` junction。编译目标为 LockdownZoneEditor Win64 Development。

默认第一关由 C++ 运行时布置；独立第二关是 Content/Chapter2Greybox/Maps/L_GarageEscape.umap。输入与关卡参数见 Config。

## 未随本次上传的内容

- Binaries、Intermediate、Saved、DerivedDataCache、调试缓存与打包目录。
- Antigravity 独立布景、Content/ArtTrials 和 Docs/ArtTrials。
- 未整理的 GASP / UEFN_Mannequin、样例 Blueprint、Audio、Misc 导入资源；不将其当作项目自制 CC0 内容一起发布。

要复现本地动画外观，应通过 Epic 获取 Game Animation Sample 并使用 `Scripts/import_gasp.py` 的资源清单迁移所需内容；先调整源项目路径。`Scripts/configure_gasp.py` / `audit_gasp.py` 用于配置与核对。项目自己的运动配置可能引用这些未随附资源。尚未进行“全新电脑只克隆此仓库”的重建测试，不能保证省略依赖后仍与截图一致。

普通感染者 Zombie7 已附整理后的 UE 资源与 CC BY 4.0 署名。自有 Gameplay 声音 / 配置、第二关和动作测试地图一并同步。

## 验证范围

2026.09.25 的展示来自本机已配置完整资源的开发版本。V3 PDF、截图与视频不等于可直接分发的独立游戏包。自动检查与首次玩家试玩分开看待；完整有声演示和干净环境打包仍待补。
