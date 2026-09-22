import unreal

paths = [
    "/Game/Weapons/Pistol/Meshes/SM_Pistol",
    "/Game/Art/KenneyFurniture/SM_desk",
    "/Game/Art/KenneyFurniture/SM_chairDesk",
    "/Game/Art/KenneyFurniture/SM_computerScreen",
    "/Game/Art/KenneyFurniture/SM_bookcaseOpen",
    "/Game/Art/KenneyFurniture/SM_loungeSofa",
    "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple",
]
for path in paths:
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        unreal.log_error("ART_ASSET_MISSING " + path)
        continue
    bounds = asset.get_bounds()
    unreal.log("ART_BOUNDS {} origin={} extent={}".format(path, bounds.origin, bounds.box_extent))
