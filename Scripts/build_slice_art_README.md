# Zero Tower 原创轻量模块

`build_slice_art.py` 面向本机 UE 5.8，使用引擎 Cube 为构件，合并成可复用 StaticMesh。无外部素材、纹理或下载依赖；不修改玩法 C++ 和现有关卡。仅在 `/Game/Art/ZeroTower` 创建资产。运行须使用独立 full editor 进程；脚本创建临时空地图，不保存地图，默认不删除或覆盖已存在网格；同路径资产复用时校验尺寸和材质集合。显式参数 `--rebuild-generated` 只允许重建本脚本的三个专属 SM，使用前应备份其 .uasset。共享材质会重新编译并保存。

调用示例（实际绝对项目与脚本路径由调用者指定）：

```powershell
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' '<project>/LockdownZone.uproject' '-ExecutePythonScript=<project>/Scripts/build_slice_art.py' -unattended -nosplash -d3d11 -sm5
```

生成阶段也不能使用 `-NullRHI`：无渲染资源时 UE MeshMergeHelpers::ExtractSections 会将所有材质替换为 WorldGridMaterial。脚本已主动拒绝该参数，改用真实 D3D11，并逐个执行 recompile_material 与 get_statistics；后者在本机源码中明确执行 FinishCompilation。

不能使用 `-run=pythonscript`：实测该 commandlet 的 StaticMeshEditorSubsystem 未初始化。`-ExecutePythonScript` 会等待 AssetRegistry 就绪、执行脚本，并由 UE 原生执行器自动退出编辑器。

UE 5.8 本地源码已核对：`StaticMeshEditorSubsystem.merge_static_mesh_actors`、`MergeStaticMeshActorsOptions`；新版原点设置为 `MeshMergingSettings.pivot_type = MeshMergePivotType.WORLD_ORIGIN`。

## 网格与放置

| 路径 | X×Y×Z 尺寸（cm） | 原点 | 朝向与放置 |
| --- | --- | --- | --- |
| `/Game/Art/ZeroTower/SM_ServerRack` | 100×80×220 | 底部中央 (0,0,0)；包围盒中心 (0,0,110) | 前门面向 -X；Actor Z 直接放地面；朝向 +X 时 yaw=180 |
| `/Game/Art/ZeroTower/SM_IndustrialDoor` | 12×230×300 | 几何中心 | 厚度 X，宽度 Y，高度 Z；双面装饰；地面 Z+150 处放置 |
| `/Game/Art/ZeroTower/SM_OfficeWallPanel` | 10×400×300 | 几何中心 | 厚度 X，沿 Y 延伸；双面装饰；地面 Z+150 处放置；沿 X 延伸时 yaw=90 |

每个网格保存一个 Box 简单碰撞，适用于静态机柜、实心墙、关闭状态门扇。可交互门由现有玩法控制碰撞开关/门扇位移；此模型不包含通道门框。不要用门模型叠在可通行门洞里并保持碰撞开启。

网格由统一材质的少量盒形部件合并，机柜包括边框、8 层机架单元、凹槽、指示灯和少量安全黄；门包括双扇分缝、加强边框、踢板与拉手；墙板包括踢脚、墙裙、细分缝。无 Nanite 需求。材质槽数：机柜 6、门 5、墙板 4；槽名等于其实际 M_ZT 材质名，墙板包含 M_ZT_WallPlaster。机柜 <1000 三角面；门与墙各 <400 三角面；准确构件数与几何尺寸写入日志和 `Saved/ZeroTowerAssetManifest.json`。

生成后不要调用 `SetMaterial(0, genericMaterial)`：保留原生材质槽能显示凹槽、门饰和指示灯。需要统一改色时，可使用下面材质的参数或派生材质实例。

## 材质

位于 `/Game/Art/ZeroTower/Materials/`。所有材质采用参数 `Tint`（线性 RGB）、`Metallic`、`Roughness`；发光材质附加 `EmissiveStrength`。不含环境贴图或贴花。

| 名称 | Tint（线性 RGB） | Metallic | Roughness | 发光强度 |
| --- | --- | --- | --- | --- |
| M_ZT_PaintedSteel | .115,.155,.175 | .40 | .62 | 0 |
| M_ZT_DarkMetal | .025,.035,.042 | .72 | .46 | 0 |
| M_ZT_Recess | .008,.012,.016 | .10 | .87 | 0 |
| M_ZT_WallPlaster | .38,.42,.43 | 0 | .90 | 0 |
| M_ZT_Floor | .12,.145,.15 | 0 | .88 | 0 |
| M_ZT_Trim | .21,.255,.275 | .35 | .66 | 0 |
| M_ZT_SafetyYellow | .52,.30,.055 | .08 | .68 | 0 |
| M_ZT_LEDGreen | .045,.52,.29 | 0 | .38 | 2.5 |
| M_ZT_LEDAmber | .80,.21,.035 | 0 | .38 | 2.0 |
| M_ZT_ConcreteDamage | .17,.185,.18 | 0 | .97 | 0 |

`M_ZT_Floor`、`M_ZT_ConcreteDamage` 是场景地板/破损痕迹的基础共享材质，当前模块没有自动使用它们。指示灯发光只是视觉提示，机房主要照明应继续使用现有关卡灯光。

## 校验

本脚本负责校验每个生成或复用网格的实际包围盒和原点（误差阈值 0.1cm），并比较实际材质集合与构件使用的材质集合，拒绝缺失/默认/意外材质。完成后自动执行 audit_assets.py；失败会抛异常。成功终止标记为 `ZERO_TOWER_ART_BUILD_COMPLETE`。合并前只生成临时 Actor，finally 清理本次生成的 Actor，不触碰其他 Actor。后续需要在真实关卡运行、截图检查材质、碰撞和门口通行，不能将脚本静态检查视为运行测试。


2026-09-22 实际运行记录：`BuildSliceArtFullEditor2.log` 成功输出 `ZERO_TOWER_ART_BUILD_COMPLETE`，三个 `.uasset` 已保存且 UE 返回的实际包围盒全部通过校验。第一次 full editor 实测发现 MeshMergeUtilities 会自动追加 `SM_` 前缀，已修正 base package basename；构件在生成后显式设置 transform，避免编辑器视口自动放置影响原点。成功进程已正常退出。生成资产大小分别约 42KB、25KB、20KB，不含共享材质。真实关卡截图由整合阶段执行。


2026-09-22 材质复验：早期 NullRHI 运行虽然尺寸通过，却将合并材质替换为默认 WorldGridMaterial。该结果已撤销：原始三个 SM 备份在本任务 work/asset-before-material-fix，真实 D3D11 full editor 重建日志 BuildSliceArtMaterialFix.log 校验通过（机柜6槽、门5槽、墙4槽）。audit_assets.py 全项目24 meshes/56 slots，default=0、null=0、errors=0。必须以此次材质审计而非早期尺寸日志作为生成完成依据。

ISM 材质用法：DressOffice 的地板/天花/标线通过 InstancedStaticMeshComponent 合批，因此生成脚本对全部10种新建或复用材质执行 `set_material_usage(...MATUSAGE_INSTANCED_STATIC_MESHES)`，然后重新编译并保存。`audit_zero_tower_usage.py` 同时读取 HasMaterialUsage 和序列化属性 used_with_instanced_static_meshes，任何一项未开启都会失败；报告为 Saved/ZeroTowerMaterialUsageAudit.json。这避免游戏运行时因缺少实例化顶点工厂 shader permutation 而退回默认材质。
