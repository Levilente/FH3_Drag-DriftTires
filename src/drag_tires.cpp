// Drag & Drift Tires for Forza Horizon 3 (FH3 Mod Platform mod).
//
// Adds two tire compounds to every car's Garage > Upgrades > Tires and Rims > Tire Compound list:
//   - "Drag Tire Compound":  the game's own hidden 19_Drag compound with the game's own drag tire model (tire_drag / tire_OW_drag).
//   - "Drift Tire Compound": the game's own hidden 18_Drift compound, on the car's normal race tire model.
// Both compounds and the drag tire models are already in the game's files, but no car's upgrade list uses them (only the Hot Wheels
// Bone Shaker and Twin Mill have the compounds, on a normal tire model).
//
// How it works: pure data. The mod queues SQL for the platform (add_sql), which applies it to a copy of gamedbRC.slt that the game reads
// instead (mediaoverride; the game's own files are never touched):
//   - one List_UpgradeTireCompound row per car and compound (the row says compound id, price and tire model),
//   - the names/descriptions as new strings (add_string_key) in an Upgrades row for the level the rows use.
// Cars that already have a compound (the two Hot Wheels cars) are left alone. A new database copy is built on the first start and
// whenever the settings below change; the texts show from the next launch.
#include "fh3mod.h"

#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

// Game constants (read from gamedbRC.slt): List_TireCompound ids, and the Upgrades rows for the Tire Compound part (UpgradeTypes id 21).
constexpr int kCompoundDrift = 18;   // List_TireCompound: 18_Drift
constexpr int kCompoundDrag  = 19;   // List_TireCompound: 19_Drag
constexpr int kCompoundRace  = 12;   // List_TireCompound: 12_DOTCompetition (the "Race" compound, 'b' tire model)

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

// fh3-mods\DragTires.ini (written with these defaults on first start).
bool g_drag        = true;
bool g_drift       = true;
int g_drag_price   = 10000;
int g_drift_price  = 10000;
bool g_verbose     = false;   // Log=1 writes lines to platform.log

std::wstring g_cfg_path;

void note(const char* fmt, ...) {   // only with Log=1
    if (!g_verbose) return;
    char line[600];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(line, sizeof line, fmt, ap);
    va_end(ap);
    if (g_api && g_mod) g_api->log(g_mod, line);
}

int cfg_int(const wchar_t* key, int def) {
    wchar_t buf[64] = L"";
    GetPrivateProfileStringW(L"DragTires", key, L"", buf, 64, g_cfg_path.c_str());
    if (buf[0]) return static_cast<int>(std::wcstol(buf, nullptr, 10));
    wchar_t v[32];
    _snwprintf(v, 32, L"%d", def);
    WritePrivateProfileStringW(L"DragTires", key, v, g_cfg_path.c_str());
    return def;
}

void load_config() {
    g_drag        = cfg_int(L"Drag", 1) != 0;
    g_drift       = cfg_int(L"Drift", 1) != 0;
    g_drag_price  = cfg_int(L"DragPrice", g_drag_price);
    g_drift_price = cfg_int(L"DriftPrice", g_drift_price);
    g_verbose     = cfg_int(L"Log", 0) != 0;
    if (g_drag_price < 0) g_drag_price = 0;
    if (g_drift_price < 0) g_drift_price = 0;
}

std::string text_ref(uint32_t key) {   // how the database refers to a text in Upgrades.str
    return "_&" + std::to_string((static_cast<unsigned long long>(FH3_TABLE_UPGRADES) << 32) | key);
}

// One compound for every car that doesn't have it yet: a List_UpgradeTireCompound row at `level`.
//   `model_sql` is the SQL expression for the tire model name; `s` is the car's stock row.
std::string rows_sql(int compound, int level, int id_offset, int price, const std::string& model_sql) {
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
    return std::string(head) + model_sql + "," + model_sql + "," + model_sql + tail;
}

void add_texts_and_sql() {
    std::string sql;
    if (g_drag) {
        g_api->add_string_key(g_mod, "Upgrades.str", kKeyDragName, L"Drag Tire Compound");
        g_api->add_string_key(g_mod, "Upgrades.str", kKeyDragDesc,
                              L"Installs drag slicks for the best possible traction in a straight line. Made for launches and the quarter mile, "
                              L"not for corners.");
        char buf[900];
        std::snprintf(buf, sizeof buf,
                      "INSERT OR REPLACE INTO Upgrades (id,TypeId,IsException,Name,Level,Description,IconPath,ImagePath) "
                      "VALUES (%d,'21',0,'%s',%d,'%s','%s','%s');",
                      kDragUpgradeId, text_ref(kKeyDragName).c_str(), kDragLevel, text_ref(kKeyDragDesc).c_str(), kIcon, kImage);
        sql += buf;
        // The game's drag tire models: tire_drag (normal cars) and tire_OW_drag (open-world cars, whose stock tire model starts with OW_).
        sql += rows_sql(kCompoundDrag, kDragLevel, 20, g_drag_price,
                        "CASE WHEN s.TireModelName LIKE 'OW\\_%' ESCAPE '\\' THEN 'OW_drag' ELSE 'drag' END");
    }
    if (g_drift) {
        g_api->add_string_key(g_mod, "Upgrades.str", kKeyDriftName, L"Drift Tire Compound");
        g_api->add_string_key(g_mod, "Upgrades.str", kKeyDriftDesc,
                              L"Installs a tire compound made for sliding: it lets go of the road early and smoothly, so the car breaks away "
                              L"and holds a drift with less effort.");
        char buf[900];
        std::snprintf(buf, sizeof buf,
                      "INSERT OR REPLACE INTO Upgrades (id,TypeId,IsException,Name,Level,Description,IconPath,ImagePath) "
                      "VALUES (%d,'21',0,'%s',%d,'%s','%s','%s');",
                      kDriftUpgradeId, text_ref(kKeyDriftName).c_str(), kDriftLevel, text_ref(kKeyDriftDesc).c_str(), kIcon, kImage);
        sql += buf;
        // The car's own race tire model (the compound-12 row that isn't a Redline/Horizon edition), else a plain race tire model.
        char model[700];
        std::snprintf(model, sizeof model,
                      "COALESCE((SELECT r.TireModelName FROM List_UpgradeTireCompound r WHERE r.Ordinal=s.Ordinal AND r.TireCompoundID=%d "
                      "AND r.TireModelName NOT LIKE '%%Redline%%' AND r.TireModelName NOT LIKE '%%orizon%%' ORDER BY r.Level LIMIT 1),"
                      "CASE WHEN s.TireModelName LIKE 'OW\\_%%' ESCAPE '\\' THEN 'OW_b' ELSE 'b' END)",
                      kCompoundRace);
        sql += rows_sql(kCompoundDrift, kDriftLevel, 21, g_drift_price, model);
    }
    if (!sql.empty()) {
        g_api->add_sql(g_mod, sql.c_str());
        note("queued the database changes: drag %d (%d CR), drift %d (%d CR)", g_drag ? 1 : 0, g_drag_price, g_drift ? 1 : 0, g_drift_price);
    }
}

}  // namespace

FH3MOD_EXPORT int fh3mod_init(const FH3ModApi* api) {
    if (api->api_version < 10) return 1;   // add_sql (2) and add_string_key (10)
    g_api = api;

    FH3ModInfo info{};
    info.size             = sizeof info;
    info.name             = "Drag and Drift Tires";
    info.version          = DRAGTIRES_VERSION;
    info.github_repo      = nullptr;
    info.platform_version = FH3MOD_PLATFORM_VERSION;
    info.check_updates    = 0;
    info.description      = "Drag and drift tire compounds for every car: Garage > Upgrades > Tires and Rims > Tire Compound.";
    info.offline_only     = 1;
    g_mod                 = api->register_mod(&info);
    if (!g_mod) return 1;

    g_cfg_path = std::wstring(api->azimuth_dir) + L"\\fh3-mods\\DragTires.ini";
    load_config();
    add_texts_and_sql();
    return 0;
}
