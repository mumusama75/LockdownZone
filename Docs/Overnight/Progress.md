# 封锁区·零号大厦 整夜开发进度记录

- 开始时间：2026-09-23 04:31 (UTC+10)
- 基线 Commit：`7cb58b9af664599741c4258e26f87174e46a74f0`
- 当前工作分支：`main`
- 引擎版本：Unreal Engine 5.8.2 (`C:\Program Files\Epic Games\UE_5.8`)
- 运行工作目录：`C:\UEProjects\LockdownZoneRepo` (Junction to `C:\Users\木\Documents\ChatGPT\cv\projects\LockdownZone`)

---

## 阶段 0：建立可回退基线（完成）

### 1. 基线自动化验证
- 执行指令：`powershell -ExecutionPolicy Bypass -File .\Scripts\Play.ps1 -QA`
- 验证结果：**PASS**
  - 断言通过数：**255 / 255**
  - 失败数：**0**
  - 运行耗时：**54.43 秒**
  - 截图总数：**23 张**
- 基线存档路径：
  - 测试报告：`Docs/Overnight/Baseline/LZSliceQA_Report.txt`
  - 运行日志：`Docs/Overnight/Baseline/Play_Baseline.log`
  - 实机截图库：`Docs/Overnight/Baseline/Screenshots/` (23张)

### 2. 基线关卡核心问题清单（依据截图与源码位置）
1. **开场视线引导与教学层次（Opening Tutorial & Visual Hierarchy）**
   - **代码位置**：`Source/LockdownZone/LZGameMode.cpp` (L317-331), `LZGameMode.cpp` (L79-88)
   - **实机截图依据**：`Docs/Overnight/Baseline/Screenshots/SliceQA_01_Opening.png`
   - **现状问题**：玩家在 `(-3300, 0, 110)` 出生正对西向东观察窗，消防斧桌置于身后南侧 `(-3300, -400)`，手枪与手电置于左侧北桌 `(-3100, 490)`。初始视角无灯光汇聚指引，斧头完全处于视野之外。需通过出生点轻微朝向调整、醒目桌面灯带/台灯聚焦与地面脚印/视线诱导，建立“窗后感染者威胁 -> 左前武器桌（枪/手电） -> 右后应急工具（消防斧） -> 击碎玻璃出击”的清晰教学层次。

2. **断电后空间识别度与地标返程（Spatial Landmarks & Blackout Navigation）**
   - **代码位置**：`Source/LockdownZone/LZGameMode.cpp` (L258-264, L352-363, L396-415), `LZOfficeDressing.cpp`
   - **实机截图依据**：`Docs/Overnight/Baseline/Screenshots/SliceQA_06_Blackout.png`, `SliceQA_07_ServerRoomFuse.png`, `SliceQA_08_MainBreaker.png`
   - **现状问题**：第4次击杀后全场断电，南北两个关键房间（南机房取保险丝、北配电间恢复供电）在黑暗中门洞结构雷同，仅依赖墙体纯色（蓝 vs 红），缺乏形体剪影地标与中途确认物。需在机房入口增加机柜通风栅与地脚冷光指引，在北侧配电间增加警示条纹框架与应急红闪频标，在十字主走廊加入物理路牌与视线引导，使玩家在断电与手电照明下能凭空间记忆准确折返。

3. **探索支线视线预告与背包空间取舍（Route Sightlines & Backpack Trade-offs）**
   - **代码位置**：`Source/LockdownZone/LZGameMode.cpp` (L333-342), `LZLoot.cpp`
   - **实机截图依据**：`Docs/Overnight/Baseline/Screenshots/SliceQA_07b_OptionalServerParts.png`, `SliceQA_07c_InventoryLoot.png`
   - **现状问题**：北侧休息区与南侧机房深处的稀有高价值战利品（服务器备件，500价值，占同行两格）平铺于深处地面，主路上缺少透视视线预告（Framed Sightline）。玩家在走廊无法预先权衡“是否值得绕道”，且满包拒收时缺乏世界内空间占位暗示。需调整摆放高度与光影剪影，建立“走廊瞥见 -> 决定绕道 -> 空间取舍（丢弃弹药/医疗包或放弃备件）”的有意义决策。

---

## 下一步计划
- **阶段 1**：绘制实机精确坐标 SVG 平面图（`Docs/Portfolio/LevelPlan.svg`）并撰写一页设计简报，限定开场、中段取舍、断电返程三大命题。
- **阶段 2**：开场教学与首次交战优化。
- **阶段 3**：断电搜索与返程地标引导。
- **阶段 4**：搜打撤路线与背包取舍强化。
- **阶段 5**：策划作品集文档库（`Docs/Portfolio/`）与展示页建设。
- **阶段 6**：最终回归测试、报告生成与交付。
