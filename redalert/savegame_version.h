#ifndef SAVEGAME_VERSION_H
#define SAVEGAME_VERSION_H

// This value is written into every Red Alert save header. Keep the calculation
// beside the save code, but expose it so CI can detect an accidental ABI break.
// Include this header only after the game object types below are complete.
constexpr unsigned int Red_Alert_Savegame_Version()
{
    return DESCRIP_MAX + 0x01000006
        + (sizeof(AircraftClass) + sizeof(AircraftTypeClass) + sizeof(AnimClass) + sizeof(AnimTypeClass)
           + sizeof(BaseClass) + sizeof(BuildingClass) + sizeof(BuildingTypeClass) + sizeof(BulletClass)
           + sizeof(BulletTypeClass) + sizeof(CellClass) + sizeof(FactoryClass) + sizeof(HouseClass)
           + sizeof(HouseTypeClass) + sizeof(InfantryClass) + sizeof(InfantryTypeClass) + sizeof(LayerClass)
           + sizeof(MouseClass) + sizeof(OverlayClass) + sizeof(OverlayTypeClass) + sizeof(SmudgeClass)
           + sizeof(SmudgeTypeClass) + sizeof(TeamClass) + sizeof(TeamTypeClass) + sizeof(TemplateClass)
           + sizeof(TemplateTypeClass) + sizeof(TerrainClass) + sizeof(TerrainTypeClass) + sizeof(TriggerClass)
           + sizeof(TriggerTypeClass) + sizeof(UnitClass) + sizeof(UnitTypeClass) + sizeof(VesselClass)
           + sizeof(ScenarioClass) + sizeof(ChronalVortexClass));
}

constexpr unsigned int Red_Alert_Savegame_Header_Version()
{
#ifdef FIXIT_CSII
    return Red_Alert_Savegame_Version() + 1;
#else
    return Red_Alert_Savegame_Version();
#endif
}

constexpr bool Is_Red_Alert_Savegame_Version_Compatible(unsigned int version)
{
    return version == Red_Alert_Savegame_Version() || version == Red_Alert_Savegame_Header_Version();
}

#ifdef __APPLE__
// Changing either value breaks existing RAtouch saves. An intentional break
// requires a documented migration/release note and an explicit canary update.
static_assert(Red_Alert_Savegame_Version() == 0x01007E46, "RAtouch Apple save ABI changed");
static_assert(Red_Alert_Savegame_Header_Version() == 0x01007E47, "RAtouch Apple save header changed");
#endif

#endif
