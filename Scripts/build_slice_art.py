"""Build original, low-poly Zero Tower modules in UE 5.8.

Run in a dedicated full editor process (a transient blank map is used):
  UnrealEditor.exe LockdownZone.uproject \
    -ExecutePythonScript=Scripts/build_slice_art.py -unattended -nosplash -d3d11 -sm5

No existing level is saved or changed. Existing named generated meshes are validated and reused.
Use the explicit --rebuild-generated argument to replace only the three dedicated generated meshes.
All dimensions below are centimetres. See build_slice_art_README.md for placement.
"""
import json
import os
import sys
import runpy
import unreal

ROOT = "/Game/Art/ZeroTower"
MAT_ROOT = ROOT + "/Materials"
REBUILD_GENERATED = "--rebuild-generated" in sys.argv
GENERATED_MESH_NAMES = {"SM_ServerRack", "SM_IndustrialDoor", "SM_OfficeWallPanel"}
if "nullrhi" in unreal.SystemLibrary.get_command_line().lower():
    raise RuntimeError("A real RHI is required: NullRHI silently replaces merged material sections with WorldGridMaterial.")
LIB = unreal.EditorAssetLibrary
MAT = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
MESHES = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
ACTORS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
CUBE = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
if MESHES is None:
    raise RuntimeError("Use full editor -ExecutePythonScript, not -run=pythonscript: StaticMeshEditorSubsystem is unavailable in this commandlet.")

# RGB values are linear, deliberately restrained for the exposure of indoor scenes.
PALETTE = {
    "M_ZT_PaintedSteel": ((0.115, 0.155, 0.175), 0.40, 0.62, 0.0),
    "M_ZT_DarkMetal": ((0.025, 0.035, 0.042), 0.72, 0.46, 0.0),
    "M_ZT_Recess": ((0.008, 0.012, 0.016), 0.10, 0.87, 0.0),
    "M_ZT_WallPlaster": ((0.38, 0.42, 0.43), 0.00, 0.90, 0.0),
    "M_ZT_Floor": ((0.12, 0.145, 0.15), 0.00, 0.88, 0.0),
    "M_ZT_Trim": ((0.21, 0.255, 0.275), 0.35, 0.66, 0.0),
    "M_ZT_SafetyYellow": ((0.52, 0.30, 0.055), 0.08, 0.68, 0.0),
    "M_ZT_LEDGreen": ((0.045, 0.52, 0.29), 0.00, 0.38, 2.5),
    "M_ZT_LEDAmber": ((0.80, 0.21, 0.035), 0.00, 0.38, 2.0),
    "M_ZT_ConcreteDamage": ((0.17, 0.185, 0.18), 0.00, 0.97, 0.0),
}


def material(name, values):
    path = MAT_ROOT + "/" + name
    existing = LIB.load_asset(path) if LIB.does_asset_exist(path) else None
    if existing:
        return existing
    rgb, metallic, roughness, glow = values
    asset = TOOLS.create_asset(name, MAT_ROOT, unreal.Material, unreal.MaterialFactoryNew())
    if not asset:
        raise RuntimeError("Cannot create " + path)
    color = MAT.create_material_expression(asset, unreal.MaterialExpressionVectorParameter, -420, -180)
    color.set_editor_property("parameter_name", "Tint")
    color.set_editor_property("default_value", unreal.LinearColor(*rgb, 1.0))
    MAT.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    for index, (label, value, prop) in enumerate([
        ("Metallic", metallic, unreal.MaterialProperty.MP_METALLIC),
        ("Roughness", roughness, unreal.MaterialProperty.MP_ROUGHNESS),
    ]):
        scalar = MAT.create_material_expression(asset, unreal.MaterialExpressionScalarParameter, -420, index * 130)
        scalar.set_editor_property("parameter_name", label)
        scalar.set_editor_property("default_value", value)
        MAT.connect_material_property(scalar, "", prop)
    if glow:
        strength = MAT.create_material_expression(asset, unreal.MaterialExpressionScalarParameter, -650, 330)
        strength.set_editor_property("parameter_name", "EmissiveStrength")
        strength.set_editor_property("default_value", glow)
        multiply = MAT.create_material_expression(asset, unreal.MaterialExpressionMultiply, -180, 250)
        MAT.connect_material_expressions(color, "", multiply, "A")
        MAT.connect_material_expressions(strength, "", multiply, "B")
        MAT.connect_material_property(multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MAT.recompile_material(asset)
    LIB.save_loaded_asset(asset, only_if_is_dirty=False)
    return asset


MATERIALS = {name: material(name, values) for name, values in PALETTE.items()}
# MeshMergeHelpers::ExtractSections substitutes DefaultMaterial when no render
# resource exists. Real RHI + a completed shader resource are mandatory here.
for material_name, asset in MATERIALS.items():
    # DressOffice uses ISM batches. This flag must be serialized before game load;
    # runtime cannot compile a missing instanced vertex-factory permutation.
    usage = unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES
    MAT.set_material_usage(asset, usage)
    if not MAT.has_material_usage(asset, usage):
        raise RuntimeError("InstancedStaticMeshes usage was not enabled for " + material_name)
    errors = MAT.recompile_material(asset)
    if errors:
        raise RuntimeError("Material compilation failed for {}: {}".format(material_name, errors))
    statistics = MAT.get_statistics(asset)  # UE source: FMaterialResource::FinishCompilation().
    unreal.log("ZERO_TOWER_MATERIAL_READY {} instanced_static_meshes={} {}".format(material_name, MAT.has_material_usage(asset, usage), statistics))
    LIB.save_loaded_asset(asset, only_if_is_dirty=False)


def part(position, dimensions, material_name, roll=0):
    return (position, dimensions, material_name, roll)


def rack_parts():
    p = []
    # Front is -X. The thin outer frame encloses recessed removable rack units.
    p.append(part((2, 0, 110), (94, 76, 216), "M_ZT_DarkMetal"))
    p.append(part((49, 0, 110), (2, 80, 220), "M_ZT_PaintedSteel"))
    for y in (-39, 39):
        p.append(part((0, y, 110), (100, 2, 220), "M_ZT_PaintedSteel"))
    for z in (3, 217):
        p.append(part((0, 0, z), (100, 80, 6), "M_ZT_PaintedSteel"))
    p.append(part((-46, 0, 110), (4, 70, 202), "M_ZT_Recess"))
    for y in (-34, 34):
        p.append(part((-49, y, 110), (2, 4, 208), "M_ZT_PaintedSteel"))
    for index, z in enumerate(range(29, 194, 23)):
        p.append(part((-48.2, 0, z), (2.4, 61, 19), "M_ZT_PaintedSteel"))
        # Simple coherent grille marks, under 1000 triangles for the entire rack.
        for dz in (-4, 0, 4):
            p.append(part((-49.6, -6, z + dz), (0.4, 37, 1.2), "M_ZT_Recess"))
        p.append(part((-49.8, 22, z + 3), (0.4, 2.5, 2.5), "M_ZT_LEDGreen" if index % 3 else "M_ZT_LEDAmber"))
        p.append(part((-49.8, 27, z + 3), (0.4, 1.5, 2.5), "M_ZT_Recess"))
    p.append(part((-49.5, 0, 207), (1, 40, 3), "M_ZT_SafetyYellow"))
    return p


def door_parts():
    p = [part((0, 0, 0), (9, 230, 300), "M_ZT_PaintedSteel")]
    # Decorative edge frames and two inset panels; a single reusable door mesh.
    for x in (-5.1, 5.1):
        for y in (-111, 111):
            p.append(part((x, y, 0), (1.8, 8, 300), "M_ZT_DarkMetal"))
        for z in (-145, 145):
            p.append(part((x, 0, z), (1.8, 230, 10), "M_ZT_DarkMetal"))
        for y in (-55, 55):
            p.append(part((x, y, 17), (1.0, 99, 214), "M_ZT_Trim"))
            p.append(part((x, y, -113), (1.2, 99, 23), "M_ZT_DarkMetal"))
        p.append(part((x, 0, 0), (1.8, 3, 280), "M_ZT_Recess"))
        for y in (-16, 16):
            p.append(part((x, y, -12), (1.8, 4, 37), "M_ZT_DarkMetal"))
        p.append(part((x, 0, 116), (1.8, 76, 6), "M_ZT_SafetyYellow"))
    return p


def wall_parts():
    p = [part((0, 0, 0), (8, 400, 300), "M_ZT_WallPlaster")]
    # Both sides finished; narrow joints break up long greybox walls.
    for x in (-4.5, 4.5):
        p.append(part((x, 0, -118), (1, 400, 54), "M_ZT_PaintedSteel"))
        p.append(part((x, 0, -144), (1, 400, 12), "M_ZT_DarkMetal"))
        p.append(part((x, 0, 144), (1, 400, 12), "M_ZT_Trim"))
        for y in (-198, 0, 198):
            p.append(part((x, y, 0), (1, 2, 280), "M_ZT_Trim"))
        p.append(part((x, 0, -85), (1, 400, 3), "M_ZT_Trim"))
    return p


def build_mesh(name, parts, expected_size, expected_center):
    path = ROOT + "/" + name
    if REBUILD_GENERATED and LIB.does_asset_exist(path):
        if name not in GENERATED_MESH_NAMES or not path.startswith(ROOT + "/"):
            raise RuntimeError("Refusing to replace asset outside the generated mesh allowlist: " + path)
        if not LIB.delete_asset(path):
            raise RuntimeError("Unable to replace dedicated generated mesh: " + path)
    if LIB.does_asset_exist(path):
        mesh = LIB.load_asset(path)
        unreal.log("ZERO_TOWER_REUSE " + path)
    else:
        actors = []
        try:
            for index, (location, dimensions, material_name, roll) in enumerate(parts):
                actor = ACTORS.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location), unreal.Rotator(0, 0, roll))
                if not actor:
                    raise RuntimeError("Failed to spawn module piece")
                actors.append(actor)
                actor.set_actor_label("ZT_Build_{}_{}".format(name, index))
                component = actor.static_mesh_component
                component.set_static_mesh(CUBE)
                component.set_material(0, MATERIALS[material_name])
                actor.set_actor_scale3d(unreal.Vector(*(v / 100.0 for v in dimensions)))
                actor.set_actor_location(unreal.Vector(*location), False, True)
                actor.set_actor_rotation(unreal.Rotator(0, 0, roll), False)
            settings = unreal.MeshMergingSettings()
            settings.set_editor_property("pivot_type", unreal.MeshMergePivotType.WORLD_ORIGIN)
            settings.set_editor_property("merge_materials", False)
            settings.set_editor_property("merge_physics_data", False)
            settings.set_editor_property("generate_light_map_uv", True)
            settings.set_editor_property("target_light_map_resolution", 128)
            settings.set_editor_property("lod_selection_type", unreal.MeshLODSelectionType.SPECIFIC_LOD)
            settings.set_editor_property("specific_lod", 0)
            options = unreal.MergeStaticMeshActorsOptions()
            # UE MeshMergeUtilities adds the SM_ prefix to the package basename.
            options.set_editor_property("base_package_name", ROOT + "/" + name.removeprefix("SM_"))
            options.set_editor_property("destroy_source_actors", False)
            options.set_editor_property("spawn_merged_actor", False)
            options.set_editor_property("mesh_merging_settings", settings)
            MESHES.merge_static_mesh_actors(actors, options)
            mesh = LIB.load_asset(path)
            if not mesh:
                raise RuntimeError("Mesh merge did not create " + path)
            MESHES.remove_collisions(mesh)
            if MESHES.add_simple_collisions(mesh, unreal.ScriptCollisionShapeType.BOX) < 0:
                raise RuntimeError("Failed to create collision for " + path)
            LIB.save_loaded_asset(mesh, only_if_is_dirty=False)
        finally:
            for actor in actors:
                if unreal.SystemLibrary.is_valid(actor):
                    ACTORS.destroy_actor(actor)
    expected_materials = {piece[2] for piece in parts}
    slots = list(mesh.get_editor_property("static_materials"))
    actual_materials = set()
    material_paths = []
    for slot in slots:
        slot_material = slot.get_editor_property("material_interface")
        if not slot_material or not slot_material.get_path_name().startswith(MAT_ROOT + "/"):
            raise RuntimeError("Missing or default material on {}: {}".format(path, slot_material))
        actual_materials.add(slot_material.get_name())
        material_paths.append(slot_material.get_path_name())
        # Public slot label can be descriptive; preserve imported slot identities
        # because MeshDescription uses them to map polygon groups on subsequent builds.
        slot.set_editor_property("material_slot_name", unreal.Name(slot_material.get_name()))
    if actual_materials != expected_materials:
        raise RuntimeError("Material sections lost on {}: actual={} expected={}".format(path, actual_materials, expected_materials))
    mesh.set_editor_property("static_materials", slots)
    LIB.save_loaded_asset(mesh, only_if_is_dirty=False)
    unreal.log("ZERO_TOWER_MATERIALS {} slots={} materials={}".format(path, len(slots), material_paths))
    bounds = mesh.get_bounds()
    actual = tuple(getattr(bounds.box_extent, axis) * 2 for axis in ("x", "y", "z"))
    center = tuple(getattr(bounds.origin, axis) for axis in ("x", "y", "z"))
    if any(abs(a - b) > 0.1 for a, b in zip(actual, expected_size)):
        raise RuntimeError("Unexpected mesh dimensions: {} {} expected {}".format(path, actual, expected_size))
    if any(abs(a - b) > 0.1 for a, b in zip(center, expected_center)):
        raise RuntimeError("Unexpected mesh pivot: {} {} expected {}".format(path, center, expected_center))
    unreal.log("ZERO_TOWER_MESH {} size={} bounds_center={} pieces={} triangles<={}".format(path, actual, center, len(parts), len(parts) * 12))
    return {"asset": path, "size_cm": actual, "bounds_center_cm": center, "max_triangles": len(parts) * 12, "simple_collision": "one box", "material_slots": material_paths}


if not CUBE:
    raise RuntimeError("Engine basic cube asset is missing")
# Never touch/sync the gameplay level. This dedicated editor process owns a disposable blank map.
unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
manifest = [
    build_mesh("SM_ServerRack", rack_parts(), (100, 80, 220), (0, 0, 110)),
    build_mesh("SM_IndustrialDoor", door_parts(), (12, 230, 300), (0, 0, 0)),
    build_mesh("SM_OfficeWallPanel", wall_parts(), (10, 400, 300), (0, 0, 0)),
]
LIB.save_directory(ROOT, only_if_is_dirty=True, recursive=True)
manifest_file = os.path.join(unreal.Paths.project_saved_dir(), "ZeroTowerAssetManifest.json")
with open(manifest_file, "w", encoding="utf-8") as output:
    json.dump({"meshes": manifest, "materials": PALETTE}, output, ensure_ascii=False, indent=2)
# Audit serialized assets through the project's shared inspection script.
runpy.run_path(os.path.join(unreal.Paths.project_dir(), "Scripts", "audit_assets.py"), run_name="__main__")
runpy.run_path(os.path.join(unreal.Paths.project_dir(), "Scripts", "audit_zero_tower_usage.py"), run_name="__main__")
unreal.log("ZERO_TOWER_ART_BUILD_COMPLETE " + manifest_file)



