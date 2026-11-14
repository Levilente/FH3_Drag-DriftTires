// Drag & Drift Tires for Forza Horizon 3 (FH3 Mod Platform mod).
//
// Adds two tire compounds to every car's Garage > Upgrades > Tires and Rims > Tire Compound list:
//   - "Drag Tire Compound":  a new compound made from the Race compound with much more straight-line grip, on the car's Race tire models.
//   - "Drift Tire Compound": a new compound made from the Sport compound with much less grip, on the car's Sport tire models.
// The drift compound is already in the game's files, but no car's upgrade list uses it (only the Hot Wheels Twin Mill has it). The game's
// own 19_Drag compound is weaker than Race on a cold tire and the game's drag tire model (tire_drag) shows a car with no tires, so
// neither is used.
//
// How it works: pure data. The mod queues SQL for the platform (add_sql), which applies it to a copy of gamedbRC.slt that the game reads
// instead (mediaoverride; the game's own files are never touched):
//   - one List_UpgradeTireCompound row per car and compound (the row says compound id, price and tire model),
//   - the names/descriptions as new strings (add_string_key) in an Upgrades row for the level the rows use.
// Cars that already have a compound (the two Hot Wheels cars) are left alone. A new database copy is built on the first start and
// whenever the settings below change; the texts show from the next launch.
#include "fh3mod.h"

#include <windows.h>

#include <cstdio>
#include <string>

namespace {

// Game constants (read from gamedbRC.slt): List_TireCompound ids, and the Upgrades rows for the Tire Compound part (UpgradeTypes id 21).
constexpr int kCompoundDrift = 24;   // List_TireCompound: new row made by this mod (a copy of 11_DOT_Sport with much less grip)
constexpr int kCompoundSport = 11;   // List_TireCompound: 11_DOT_Sport, the copy's base
constexpr int kCompoundDrag  = 23;   // List_TireCompound: new row made by this mod (a copy of 12_DOTCompetition with more straight-line grip)
constexpr int kCompoundRace  = 12;   // List_TireCompound: 12_DOTCompetition, the copy's base
constexpr int kLevelSport    = 2;    // List_UpgradeTireCompound.Level of the "Sport Tire Compound" (the drift tires use its tire models)
constexpr int kLevelRace     = 3;    // ... of the "Race Tire Compound" (the drag tires use its tire models)

// The Level a compound is stored under in List_UpgradeTireCompound; the shop lists a car's compounds by level, names them from the
// Upgrades row with the same level. Both levels are new, with new Upgrades rows: nothing the game has is changed.
constexpr int kDragLevel         = 10;
constexpr int kDragUpgradeId     = 9000010;
constexpr int kDriftLevel        = 11;
constexpr int kDriftUpgradeId    = 9000011;

// Keys of the new texts in Upgrades.str.
constexpr uint32_t kKeyDragName  = 0xD4A60101u;
constexpr uint32_t kKeyDragDesc  = 0xD4A60102u;
constexpr uint32_t kKeyDriftName = 0xD4A60103u;
constexpr uint32_t kKeyDriftDesc = 0xD4A60104u;

const char kIcon[]  = "GAME:\\Media\\UI\\Textures\\Data_Bound\\Upgrade_Parts\\Tire_Rims\\tire_compound_3.swatchbin";
const char kImage[] = "GAME:\\Media\\UI\\Textures\\Data_Bound\\Upgrade_Parts\\Tire_Rims\\tire_compound_LG_3.swatchbin";

const FH3ModApi* g_api = nullptr;
FH3Mod* g_mod          = nullptr;

// Fixed values (the mod has no settings file).
constexpr int kDragPrice     = 10000;
constexpr int kDriftPrice    = 10000;
constexpr double kLongGrip   = 1.70;   // the drag compound's straight-line (launch and acceleration) grip, as a multiple of the Race compound's
constexpr double kLatGrip    = 0.55;   // its cornering grip, as a multiple of the Race compound's
// The drift compound is made from the Sport compound with much less grip, so a powerful car breaks traction and holds a slide on the throttle
// (the game's own 18_Drift compound has MORE acceleration grip than Race, 1.55 against 1.35, and doesn't slide).
constexpr double kDriftAccelGrip = 0.80;   // acceleration grip (launch and high speed), as a multiple of the Sport compound's
constexpr double kDriftLatGrip   = 0.80;   // cornering grip
constexpr double kDriftBrakeGrip = 0.90;   // braking grip

std::string text_ref(uint32_t key) {   // how the database refers to a text in Upgrades.str
    return "_&" + std::to_string((static_cast<unsigned long long>(FH3_TABLE_UPGRADES) << 32) | key);
}

// The game has no drag tire model (tire_drag / tire_OW_drag are in the files but show a car with no tires), so each new compound borrows the tire
// models of one of the car's own existing compounds: the row at `from_level` (Race = 3, Sport = 2), else the car's stock models.
//   `col` is the model column to copy (TireModelName, WetTireModelName or SnowTireModelName); `s` is the car's stock row.
std::string borrow_sql(const char* col, int from_level) {
    char b[400];
    std::snprintf(b, sizeof b, "COALESCE((SELECT r.%s FROM List_UpgradeTireCompound r WHERE r.Ordinal=s.Ordinal AND r.Level=%d LIMIT 1),s.%s)", col,
                  from_level, col);
    return b;
}

// One compound for every car that doesn't have it yet: a List_UpgradeTireCompound row at `level`, on the tire models of `from_level`.
std::string rows_sql(int compound, int level, int id_offset, int price, int from_level) {
    char head[512];
    std::snprintf(head, sizeof head,
                  "INSERT OR IGNORE INTO List_UpgradeTireCompound "
                  "(Id,Ordinal,Level,IsStock,TireCompoundID,ManufacturerID,Price,MassDiff,DragScale,WindInstabilityScale,RequiresGraphics,"
                  "TireModelName,WetTireModelName,SnowTireModelName) "
                  "SELECT s.Ordinal*1000+%d, s.Ordinal, %d, 0, %d, 0, %d, 0.0, 1.0, 1.0, 0, ",
                  id_offset, level, compound, price);
    char tail[640];
    std::snprintf(tail, sizeof tail,
                  " FROM List_UpgradeTireCompound s WHERE s.IsStock=1 "
                  "AND NOT EXISTS (SELECT 1 FROM List_UpgradeTireCompound x WHERE x.Ordinal=s.Ordinal AND (x.Level=%d OR x.TireCompoundID=%d));",
                  level, compound);
    return std::string(head) + borrow_sql("TireModelName", from_level) + "," + borrow_sql("WetTireModelName", from_level) + "," +
           borrow_sql("SnowTireModelName", from_level) + tail;
}

void add_texts_and_sql() {
    std::string sql;
    {
        g_api->add_string_key(g_mod, "Upgrades.str", kKeyDragName, L"Drag Tire Compound");
        g_api->add_string_key(g_mod, "Upgrades.str", kKeyDragDesc,
                              L"Installs drag slicks for far more traction in a straight line: much quicker launches and acceleration. "
                              L"Cornering grip is far lower than the Race compound: made for going straight.");
        char buf[900];
        std::snprintf(buf, sizeof buf,
                      "INSERT OR REPLACE INTO Upgrades (id,TypeId,IsException,Name,Level,Description,IconPath,ImagePath) "
                      "VALUES (%d,'21',0,'%s',%d,'%s','%s','%s');",
                      kDragUpgradeId, text_ref(kKeyDragName).c_str(), kDragLevel, text_ref(kKeyDragDesc).c_str(), kIcon, kImage);
        sql += buf;
        // The drag compound: a new List_TireCompound row, a copy of the Race compound (12) with more straight-line grip (acceleration at
        // low and high speed) and much less cornering grip. The game's own 19_Drag compound is left alone (it only has grip
        // when the tires are hot, and the game doesn't heat them up noticeably).
        char c[1200];
        std::snprintf(c, sizeof c,
                      "DROP TABLE IF EXISTS temp.dragtires_c;"
                      "CREATE TEMP TABLE dragtires_c AS SELECT * FROM List_TireCompound WHERE TireCompoundID=%d;"
                      "UPDATE dragtires_c SET TireCompoundID=%d, DisplayName='23_DragTires', "
                      "TorqueFreeLongFrictionScaleAccel0=TorqueFreeLongFrictionScaleAccel0*%.4f, "
                      "TorqueFreeLongFrictionScaleAccel1=TorqueFreeLongFrictionScaleAccel1*%.4f, "
                      "TorqueFreeLatFrictionScale=TorqueFreeLatFrictionScale*%.4f;"
                      "DELETE FROM List_TireCompound WHERE TireCompoundID=%d;"
                      "INSERT INTO List_TireCompound SELECT * FROM dragtires_c;"
                      "DROP TABLE dragtires_c;",
                      kCompoundRace, kCompoundDrag, kLongGrip, kLongGrip, kLatGrip, kCompoundDrag);
        sql += c;
        sql += rows_sql(kCompoundDrag, kDragLevel, 20, kDragPrice, kLevelRace);
    }
    {
        g_api->add_string_key(g_mod, "Upgrades.str", kKeyDriftName, L"Drift Tire Compound");
        g_api->add_string_key(g_mod, "Upgrades.str", kKeyDriftDesc,
                              L"Installs a low-grip compound made for sliding: the rear lets go easily under power and the car holds a drift "
                              L"on the throttle. Not for grip driving.");
        char buf[900];
        std::snprintf(buf, sizeof buf,
                      "INSERT OR REPLACE INTO Upgrades (id,TypeId,IsException,Name,Level,Description,IconPath,ImagePath) "
                      "VALUES (%d,'21',0,'%s',%d,'%s','%s','%s');",
                      kDriftUpgradeId, text_ref(kKeyDriftName).c_str(), kDriftLevel, text_ref(kKeyDriftDesc).c_str(), kIcon, kImage);
        sql += buf;
        char d[1400];
        std::snprintf(d, sizeof d,
                      "DROP TABLE IF EXISTS temp.drifttires_c;"
                      "CREATE TEMP TABLE drifttires_c AS SELECT * FROM List_TireCompound WHERE TireCompoundID=%d;"
                      "UPDATE drifttires_c SET TireCompoundID=%d, DisplayName='24_DriftTires', "
                      "TorqueFreeLongFrictionScaleAccel0=TorqueFreeLongFrictionScaleAccel0*%.4f, "
                      "TorqueFreeLongFrictionScaleAccel1=TorqueFreeLongFrictionScaleAccel1*%.4f, "
                      "TorqueFreeLatFrictionScale=TorqueFreeLatFrictionScale*%.4f, "
                      "TorqueFreeLongFrictionScaleBrake=TorqueFreeLongFrictionScaleBrake*%.4f;"
                      "DELETE FROM List_TireCompound WHERE TireCompoundID=%d;"
                      "INSERT INTO List_TireCompound SELECT * FROM drifttires_c;"
                      "DROP TABLE drifttires_c;",
                      kCompoundSport, kCompoundDrift, kDriftAccelGrip, kDriftAccelGrip, kDriftLatGrip, kDriftBrakeGrip, kCompoundDrift);
        sql += d;
        sql += rows_sql(kCompoundDrift, kDriftLevel, 21, kDriftPrice, kLevelSport);
    }
    g_api->add_sql(g_mod, sql.c_str());
}

}  // namespace

FH3MOD_EXPORT int fh3mod_init(const FH3ModApi* api) {
    if (api->api_version < 10) return 1;   // add_sql (2) and add_string_key (10)
    g_api = api;

    FH3ModInfo info{};
    info.size             = sizeof info;
    info.name             = "Drag and Drift Tires";
    info.version          = DRAGTIRES_VERSION;
    info.github_repo      = "Levilente/FH3_Drag-DriftTires";
    info.platform_version = FH3MOD_PLATFORM_VERSION;
    info.check_updates    = 1;
    info.description      = "Drag and drift tire compounds for every car: Garage > Upgrades > Tires and Rims > Tire Compound.";
    info.offline_only     = 1;
    g_mod                 = api->register_mod(&info);
    if (!g_mod) return 1;

    add_texts_and_sql();
    return 0;
}
