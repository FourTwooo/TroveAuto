#define ENTITY_FIELD_DEBUG
/* Module
 *    -> Auther: Angels-D
 *
 * LastChange
 *    -> 2025/04/16 04:40
 *    -> 1.0.0
 *
 * Build
 *    -> g++ -shared -static -Os -Wall -o Module.dll -x c++ ./libs/Module.hpp
 */

/* 地址寻址
 * 玩家: (+Size Value) 55 8B EC 83 E4 F8 83 EC 08 F3 0F 2A 45 10 56 8B F1 57 8B 3D
 * 世界: (+10   Value) 55 8B EC 83 7D 08 04 75 10 A1 XX XX XX XX 85 C0 74 07 C6 80 59 01 00 00 01 5D C2 04 00
 * 设置: (+Size Value) 89 45 F4 8B 11 FF 52 0C 8B 0D XX XX XX XX 8B D8 6A 03 68 XX XX XX XX 8B 11 FF 52 0C 8B 0D
 * 聊天: (+2    Value) 8B 0D XX XX XX XX 6A 00 6A 01 C6 41 20 00 E8 XX XX XX XX 6A 08 8D 8D 60 FF FF FF C7 85 60 FF FF FF 00 00 00 00 C7 85 64 FF FF FF 00 00 00 00 C7 85 68 FF FF FF 00 00 00 00 C6 45 F0 01
 * 隐藏特效: (+3 Address) F3 0F 11 44 24 24 F3 0F 58 84 24 80 00 00 00 50 F3 0F 11 43 24 E8 XX XX XX XX 8D 44 24 34 50
 * 自动攻击: (+1 Address) DF F1 DD D8 72 1F
 * 打破障碍: (+3 Address) 80 7F XX 00 0F 84 XX XX XX XX 8B 4B 08 E8 XX XX XX XX FF 75 0C 8B 4D 10 8B F0 FF 75 08 8B 45 14 83 EC 0C 8B 3E 8B D4 6A 01 89 0A 8B CE 89 42 04 8B 45 18
 * 视角遮挡: (+0 Address) 0F 29 01 C7 41 34 00 00 00 00 0F
 * 保持骑乘: (+0 Address) 74 XX 8B 07 8B CF 6A 00 6A 00 FF 50
 * 钓鱼地址: (+0 Address) 10 14 XX XX 00 00 00 00 FF 00 00 00 00
 * 视角固定: (+0 Address) 74 05 8B 01 FF 50 0C 8B E5
 * 地图放大: (+0 Address) 77 XX B8 XX XX XX XX F3 0F 10 08 F3 0F 11 89 XX XX XX XX 8B 89
 * 快速挖矿: (+1 Address) DF F1 DD D8 72 61
 * 快速挖矿（晶洞）: (+1 Address) DF F1 DD D8 72 35 8D
 * 账号地址: (-9 Address) FF 70 1C FF 70 18 8D 45 B0
 * 视野放大: (+3 Address) F3 0F 11 5F 2C
 * 穿墙: (+0 Address) 74 31 FF 73 14 8B 47 04 2B 07
 * 绕道: (+1 Address) DC 67 68 C6
 * 技能栏: 5X CF XX XX 55 CF XX XX 5X CF XX XX 55 CF XX XX FE FF FF FF 00 00 00 00 65 CF XX XX 0X 00 00 00 55 CF XX XX XX XX XX XX 55 CF XX XX 55 CF XX XX
 *            (-170H Value) 数量
 *            (+0 Value) 55释放,5D按下
 *            (+8 Value) 55未装备,5D装备
 *            (+28 Value) 00 右键 | 02 1技能 | 04 2技能 | 06 SHIFT | 08 未知 | 0A 药瓶 | 0C R物品 | 0E T物品
 */

#ifndef _MODULE_HPP_
#define _MODULE_HPP_

#define DLL_EXPORT __declspec(dllexport)

#include <map>
#include <queue>
#include <unordered_set>

// #include "WindowInput.hpp"
#include "Game.hpp"
// ==== Logger（极简，线程安全，文件落地）====
#include <windows.h>
#include <mutex>
#include <cstdio>
#include <algorithm>
#include <unordered_map>
#include <sstream>
#include <cctype>
#include <string>
#include <atomic>
#include <regex>
#include <deque>
#include <cstring>
#include <chrono>
#include <thread>
#include <cmath>
#include <random>
#include <iterator>
#include <vector>

bool isDevTools = false;

float CalculateDistance(const float &ax, const float &ay, const float &az, const float &bx, const float &by, const float &bz)
{
    float dx = ax - bx;
    float dy = ay - by;
    float dz = az - bz;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// 定义一个trim函数
std::string trim(const std::string &str)
{
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos)
    {
        return "";
    }
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

// // 添加一个辅助函数来连接字符串向量
// std::string join(const std::vector<std::string> &vec, const std::string &delimiter)
// {
//     std::ostringstream oss;
//     for (size_t i = 0; i < vec.size(); ++i)
//     {
//         if (i != 0)
//         {
//             oss << delimiter;
//         }
//         oss << vec[i];
//     }
//     return oss.str();
// }

// 创建正则表达式向量的辅助函数
auto createRegexVector = [](const auto &strings)
{
    std::vector<std::regex> result;
    for (const auto &s : strings)
    {
        if (auto trimmed = trim(s); !trimmed.empty())
        {
            result.emplace_back(trimmed);
        }
    }
    return result;
};
// ===== AHK event bridge (回调) =====
extern "C"
{
    typedef void(__stdcall *AhkEventCallbackA)(unsigned evtId, const char *payload);
    __declspec(dllexport) void __stdcall SetAhkEventCallbackA(AhkEventCallbackA cb);
}

// 事件ID定义（你也可以放到头文件里统一管理）
enum : unsigned
{
    EVT_RESP_CAN_REVIVE = 1001, // AHK去按“复活键”
    EVT_USE_POTION = 1002,      // AHK去按“药水键Q”
};

namespace AhkEvt
{
    static AhkEventCallbackA g_cbA = nullptr;
    inline void Emit(unsigned id, const char *msg)
    {
        if (g_cbA)
            g_cbA(id, msg ? msg : "");
    }
}

extern "C" void __stdcall SetAhkEventCallbackA(AhkEventCallbackA cb)
{
    AhkEvt::g_cbA = cb;
}

// ===== AHK log bridge (回调) =====
extern "C"
{
    typedef void(__stdcall *AhkLogCallbackA)(int level, const char *msg); // level: 0..5
    __declspec(dllexport) void __stdcall SetAhkLogCallbackA(AhkLogCallbackA cb);
    __declspec(dllexport) void __stdcall SetLogRoute(unsigned route);
    // route 位标志：1=回调到 AHK，2=写文件(保持原状)，4=OutputDebugStringA
}

namespace AhkBridge
{
    static AhkLogCallbackA g_cbA = nullptr;
    static unsigned g_route = 2; // 默认沿用旧行为=只写文件
}

extern "C" void __stdcall SetAhkLogCallbackA(AhkLogCallbackA cb)
{
    AhkBridge::g_cbA = cb;
}
extern "C" void __stdcall SetLogRoute(unsigned route)
{
    AhkBridge::g_route = route;
}

namespace SimpleLog
{
    static std::mutex g_mtx;
    static FILE *g_fp = nullptr;
    static bool g_on = true;

    static void ensure_open_default()
    {
        if (!g_fp)
        {
            char tmp[MAX_PATH], path[MAX_PATH];
            DWORD n = GetTempPathA(sizeof(tmp), tmp);
            if (n == 0 || n > sizeof(tmp))
                lstrcpynA(tmp, "C:\\Windows\\Temp\\", sizeof(tmp));
            wsprintfA(path, "%s%s", tmp, "TroveAuto_Entity.log");
            g_fp = fopen(path, "a");
        }
    }

    inline void set_path(const char *p)
    {
        std::lock_guard<std::mutex> lk(g_mtx);
        if (g_fp)
        {
            fclose(g_fp);
            g_fp = nullptr;
        }
        g_fp = fopen(p, "a");
    }

    inline void set_on(bool on)
    {
        std::lock_guard<std::mutex> lk(g_mtx);
        g_on = on;
    }

    // level 映射：0=trace, 1=debug, 2=info, 3=warn, 4=err, 5=critical
    inline void logf_level(int level, const char *fmt, ...)
    {
        if (!g_on)
            return;

        // 组装一行文本：时间头 + body
        SYSTEMTIME st;
        GetLocalTime(&st);
        // char head[64];
        // _snprintf_s(head, sizeof(head), _TRUNCATE,
        //             "[%02u:%02u:%02u.%03u] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

        char body[1024];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf_s(body, sizeof(body), _TRUNCATE, fmt, ap);
        va_end(ap);

        char line[64 + 1024 + 8];
        _snprintf_s(line, sizeof(line), _TRUNCATE, "%s", body);

        // 1) 回调到 AHK（若开启）
        if ((AhkBridge::g_route & 1) && AhkBridge::g_cbA)
        {
            // 注意：可能在 DLL 的后台线程里回调到 AHK（AHK 能处理绝大部分回调场景）
            AhkBridge::g_cbA(level, line);
        }

        // 2) OutputDebugString（可选）
        if (AhkBridge::g_route & 4)
            OutputDebugStringA(line);

        // 3) 写文件（若开启）
        if (AhkBridge::g_route & 2)
        {
            ensure_open_default();
            if (!g_fp)
                return;
            std::lock_guard<std::mutex> lk(g_mtx);
            fputs(line, g_fp);
            fputc('\n', g_fp);
            fflush(g_fp);
        }
    }

    inline void logf(const char *fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        char buf[1024];
        _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
        va_end(ap);
        logf_level(2 /*info*/, "%s", buf);
    }
}

// 级别宏（可按需替换你现有的 LOGF）
#define LOGT(...) SimpleLog::logf_level(0, __VA_ARGS__)
#define LOGD(...) SimpleLog::logf_level(1, __VA_ARGS__)
#define LOGI(...) SimpleLog::logf_level(2, __VA_ARGS__)
#define LOGW(...) SimpleLog::logf_level(3, __VA_ARGS__)
#define LOGE(...) SimpleLog::logf_level(4, __VA_ARGS__)
#define LOGC(...) SimpleLog::logf_level(5, __VA_ARGS__)
#define LOGF(...) LOGI(__VA_ARGS__) // 兼容：旧 LOGF 默认当 info

// #define LOGF(...) SimpleLog::logf(__VA_ARGS__)

// ==== 给 AHK 用的导出函数 ====
extern "C"
{
    __declspec(dllexport) void LogOn(int on) { SimpleLog::set_on(on != 0); }
}
// Module.h

namespace Module
{
    struct Feature
    {
        Memory::Offsets offsets;
        std::vector<BYTE> dataOn, dataOff;
        Game::Signature signature;
        std::vector<BYTE> dataIn;
        static Feature hideAnimation;
        static Feature autoAttack;
        static Feature breakBlocks;
        static Feature byPass;
        static Feature clipCam;
        static Feature disMount;
        static Feature lockCam;
        static Feature unlockMapLimit;
        static Feature quickMining;
        static Feature quickMiningGeode;
        static Feature noGravity;
        static Feature noClip;
        static Feature unlockZoomLimit;
    };

    static uint32_t bossLevel = 4;
    static uint32_t tpStep = 4;
    static uint32_t mapWidth = 4300;
    static uint32_t entityScand = 100;
    static float maxY = 400, minY = -45;
    static std::pair<float, float> aimOffset = {1.25, 0.25};

    // 短暂黑名单
    std::vector<std::string> timeOutIds = {};

    // ===== 新增：扫图参数 & 状态 =====
    static uint32_t realisticScanRange = 3000;            // NEW: 替代 9999 的真实扫描半径（米，经验值75~90）
    static std::unordered_set<std::string> switchIds = { // NEW: “开关ID”列表（仅作名称/关键词匹配）
        "quest_spawn_trigger_fivestar_depths", "quest_spawn_trigger_fivestar"};

    // 全局死亡标记
    // static std::atomic<bool> isPlayerDead{false}; // NEW
    // static std::atomic<bool> respawnNotified{false};

    // NEW：AutoPotion 开关&参数（默认同你 AHK：3000ms，stopAtCount=0 表示忽略库存）
    static std::atomic<bool> autoPotionEnabled{false};
    static std::atomic<uint32_t> apMinIntervalMs{3000};
    static std::atomic<uint32_t> apStopAtCount{0}; // 你暂时不做库存判定，可先不使用

    // NEW：AutoRespawn 开关（Health 开启时置 true）
    static std::atomic<bool> autoRespawnEnabled{false};

    // 深渊宝箱黑名单存放
    static std::deque<std::tuple<float, float, float>> blacklistedChests;
    static std::mutex blacklistMutex;

    // 穿墙开关状态 & Z 轴偏移
    static std::atomic<bool> noClipOn{false};

    static std::map<std::string, void *> configMap =
        {{"Module::bossLevel", &Module::bossLevel},
         {"Module::tpStep", &Module::tpStep},
         {"Module::mapWidth", &Module::mapWidth},
         {"Module::entityScand", &Module::entityScand},
         {"Module::maxY", &Module::maxY},
         {"Module::minY", &Module::minY},
         {"Module::aimOffset", &Module::aimOffset},
         {"Module::Feature::hideAnimation", &Module::Feature::hideAnimation},
         {"Module::Feature::autoAttack", &Module::Feature::autoAttack},
         {"Module::Feature::breakBlocks", &Module::Feature::breakBlocks},
         {"Module::Feature::byPass", &Module::Feature::byPass},
         {"Module::Feature::clipCam", &Module::Feature::clipCam},
         {"Module::Feature::disMount", &Module::Feature::disMount},
         {"Module::Feature::lockCam", &Module::Feature::lockCam},
         {"Module::Feature::unlockMapLimit", &Module::Feature::unlockMapLimit},
         {"Module::Feature::quickMining", &Module::Feature::quickMining},
         {"Module::Feature::quickMiningGeode", &Module::Feature::quickMiningGeode},
         {"Module::Feature::noGravity", &Module::Feature::noGravity},
         {"Module::Feature::noClip", &Module::Feature::noClip},
         {"Module::Feature::unlockZoomLimit", &Module::Feature::unlockZoomLimit},
         {"Game::moduleName", &Game::moduleName},
         {"Game::World::signature", &Game::World::signature},
         {"Game::World::offsets", &Game::World::offsets},
         {"Game::World::Data::playerCountOffsets", &Game::World::Data::playerCountOffsets},
         {"Game::World::NodeInfo::offsets", &Game::World::NodeInfo::offsets},
         {"Game::World::NodeInfo::Data::baseAddressOffsets", &Game::World::NodeInfo::Data::baseAddressOffsets},
         {"Game::World::NodeInfo::Data::stepOffsets", &Game::World::NodeInfo::Data::stepOffsets},
         {"Game::World::NodeInfo::Data::sizeOffsets", &Game::World::NodeInfo::Data::sizeOffsets},
         {"Game::World::Entity::offsets", &Game::World::Entity::offsets},
         {"Game::World::Entity::Data::levelOffsets", &Game::World::Entity::Data::levelOffsets},
         {"Game::World::Entity::Data::nameOffsets", &Game::World::Entity::Data::nameOffsets},
         {"Game::World::Entity::Data::isDeathOffsets", &Game::World::Entity::Data::isDeathOffsets},
         {"Game::World::Entity::Data::healthOffsets", &Game::World::Entity::Data::healthOffsets},
         {"Game::World::Entity::Data::xOffsets", &Game::World::Entity::Data::xOffsets},
         {"Game::World::Entity::Data::yOffsets", &Game::World::Entity::Data::yOffsets},
         {"Game::World::Entity::Data::zOffsets", &Game::World::Entity::Data::zOffsets},
         {"Game::World::Player::offsets", &Game::World::Player::offsets},
         {"Game::World::Player::Data::nameOffsets", &Game::World::Player::Data::nameOffsets},
         {"Game::World::Player::Data::xOffsets", &Game::World::Player::Data::xOffsets},
         {"Game::World::Player::Data::yOffsets", &Game::World::Player::Data::yOffsets},
         {"Game::World::Player::Data::zOffsets", &Game::World::Player::Data::zOffsets},
         {"Game::Player::signature", &Game::Player::signature},
         {"Game::Player::offsets", &Game::Player::offsets},
         {"Game::Player::Data::itemRSignature", &Game::Player::Data::itemRSignature},
         {"Game::Player::Data::itemTSignature", &Game::Player::Data::itemTSignature},
         {"Game::Player::Data::nameOffsets", &Game::Player::Data::nameOffsets},
         {"Game::Player::Data::healthOffsets", &Game::Player::Data::healthOffsets},
         {"Game::Player::Data::drawDistanceOffsets", &Game::Player::Data::drawDistanceOffsets},
         {"Game::Player::Data::itemROffsets", &Game::Player::Data::itemROffsets},
         {"Game::Player::Data::itemTOffsets", &Game::Player::Data::itemTOffsets},
         {"Game::Player::Camera::offsets", &Game::Player::Camera::offsets},
         {"Game::Player::Camera::Data::xPerOffsets", &Game::Player::Camera::Data::xPerOffsets},
         {"Game::Player::Camera::Data::yPerOffsets", &Game::Player::Camera::Data::yPerOffsets},
         {"Game::Player::Camera::Data::zPerOffsets", &Game::Player::Camera::Data::zPerOffsets},
         {"Game::Player::Camera::Data::vOffsets", &Game::Player::Camera::Data::vOffsets},
         {"Game::Player::Camera::Data::hOffsets", &Game::Player::Camera::Data::hOffsets},
         {"Game::Player::Coord::offsets", &Game::Player::Coord::offsets},
         {"Game::Player::Coord::Data::xOffsets", &Game::Player::Coord::Data::xOffsets},
         {"Game::Player::Coord::Data::yOffsets", &Game::Player::Coord::Data::yOffsets},
         {"Game::Player::Coord::Data::zOffsets", &Game::Player::Coord::Data::zOffsets},
         {"Game::Player::Coord::Data::xVelOffsets", &Game::Player::Coord::Data::xVelOffsets},
         {"Game::Player::Coord::Data::yVelOffsets", &Game::Player::Coord::Data::yVelOffsets},
         {"Game::Player::Coord::Data::zVelOffsets", &Game::Player::Coord::Data::zVelOffsets},
         {"Game::Player::Fish::signature", &Game::Player::Fish::signature},
         {"Game::Player::Fish::offsets", &Game::Player::Fish::offsets},
         {"Game::Player::Fish::Data::waterTakeOffsets", &Game::Player::Fish::Data::waterTakeOffsets},
         {"Game::Player::Fish::Data::lavaTakeOffsets", &Game::Player::Fish::Data::lavaTakeOffsets},
         {"Game::Player::Fish::Data::chocoTakeOffsets", &Game::Player::Fish::Data::chocoTakeOffsets},
         {"Game::Player::Fish::Data::plasmaTakeOffsets", &Game::Player::Fish::Data::plasmaTakeOffsets},
         {"Game::Player::Fish::Data::waterStatusOffsets", &Game::Player::Fish::Data::waterStatusOffsets},
         {"Game::Player::Fish::Data::lavaStatusOffsets", &Game::Player::Fish::Data::lavaStatusOffsets},
         {"Game::Player::Fish::Data::chocoStatusOffsets", &Game::Player::Fish::Data::chocoStatusOffsets},
         {"Game::Player::Fish::Data::plasmaStatusOffsets", &Game::Player::Fish::Data::plasmaStatusOffsets},
         {"Game::Player::Bag::offset", &Game::Player::Bag::offsets}};

    static std::map<std::pair<int, std::string>, std::atomic<bool>> functionRunMap;

    void SetFeature(const Feature &feature, const Memory::DWORD &pid, const bool &on = true);
    void SetNoClip(const Feature &feature, const Memory::DWORD &pid, const bool &on = true);
    void SetAutoAttack(const Memory::DWORD &pid, const uint32_t &keep = 300, const uint32_t &delay = 1000);
    void SetAutoRespawn(const Memory::DWORD &pid, const uint32_t &delay = 50);

    void AutoAim(const Memory::DWORD &pid, const bool &targetBoss = true, const bool &targetPlant = false, const bool &targetNormal = false, const std::vector<std::string> &targets = {}, const std::vector<std::string> &noTargets = {}, const uint32_t &range = 45, const uint32_t &delay = 50);
    void SpeedUp(const Memory::DWORD &pid, const float &speed = 50, const uint32_t &delay = 50, const std::vector<std::string> &hotKey = {"W", "A", "S", "D", "Space", "Shift"});

    void Tp2Forward(const Memory::DWORD &pid, const float &tpRange = tpStep, const uint32_t &delay = 50);
    void Tp2Target(const Memory::DWORD &pid, const float &targetX, const float &targetY, const float &targetZ, const uint32_t &delay = 50, const uint32_t &tryAgainMax = 10);
    void FollowTarget(const Memory::DWORD &pid, const std::vector<std::string> &players = {}, const std::vector<std::string> &targets = {}, const std::vector<std::string> &noTargets = {}, const bool &targetBoss = false, const bool &scanAll = false, const float &speed = 50, const uint32_t &delay = 50);

    std::unique_ptr<Game::World::Player> FindPlayer(Game &game, const std::vector<std::string> &targets);
    std::unique_ptr<Game::World::Entity> FindTarget(Game &game, const bool &targetBoss = true, const bool &targetPlant = false, const bool &targetNormal = false, const std::vector<std::string> &targets = {}, const std::vector<std::string> &noTargets = {}, const uint32_t &range = 45);

    void GetNextPoint(float &x, float &z, std::unordered_set<std::string> &visitedPoints);

    inline bool Arrived2D(Game &game, float tx, float tz, float eps = 1.0f)
    {
        float px = game.data.player.data.coord.data.x.UpdateAddress().UpdateData().data;
        float pz = game.data.player.data.coord.data.z.UpdateAddress().UpdateData().data;
        float dx = px - tx, dz = pz - tz;
        return (dx * dx + dz * dz) <= (eps * eps);
    }

    struct PrioritizedEntity;
    struct MoveCtx; // 前置声明
    void KillEntitys(const Memory::DWORD &pid, std::vector<PrioritizedEntity> &entitys, Game &game, MoveCtx &ctx, uint32_t sleepTime, std::string tab, Game::World::Entity overEntity, std::vector<std::string> targets, std::vector<std::string> noTargets);

};

extern "C"
{
    DLL_EXPORT void UpdateConfig(const char *key, const char *value);
    DLL_EXPORT void FunctionOn(const Memory::DWORD pid, const char *funtion, const char *argv = "", const bool waiting = false);
    DLL_EXPORT void FunctionOff(const Memory::DWORD pid, const char *funtion = nullptr);
    DLL_EXPORT void WhichTarget(const Memory::DWORD pid, char *result, const uint32_t size, const char *argv = "");
}

// Module.cpp

std::vector<std::string> split(const std::string &str, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    for (char ch : str)
        if (ch == delimiter)
        {
            if (!token.empty())
            {
                tokens.emplace_back(token);
                token.clear();
            }
        }
        else
            token += ch;
    if (!token.empty())
        tokens.emplace_back(token);
    return tokens;
}

std::pair<float, float> CalculateAngles(const float &ax, const float &ay, const float &az, const float &bx, const float &by, const float &bz)
{
    float dx = bx - ax;
    float dy = ay - by;
    float dz = bz - az;

    // 计算水平角h
    float ah = std::atan2(dx, dz); // 参数顺序为Δx（东向分量）, Δz（南向分量）

    // 计算俯仰角v
    float horizontal_dist = std::sqrt(dx * dx + dz * dz);
    float av = std::atan2(dy, horizontal_dist);
    return {ah, av};
}

namespace Module
{
    static std::atomic<uint32_t> switchActivatedCount{0};

    Feature Feature::hideAnimation = {
        {0x74DF95},
        {0x4C},
        {0x44},
        {0x3, "F3 0F 11 44 24 24 F3 0F 58 84 24 80 00 00 00 50 F3 0F 11 43 24 E8 XX XX XX XX 8D 44 24 34 50"}};
    Feature Feature::autoAttack = {
        {0xB28678},
        {0xF0},
        {0xF1},
        {0x1, "DF F1 DD D8 72 1F"}};
    Feature Feature::breakBlocks = {
        {0x8E9913},
        {0x01},
        {0x00},
        {0x3, "80 7F XX 00 0F 84 XX XX XX XX 8B 4B 08 E8 XX XX XX XX FF 75 0C 8B 4D 10 8B F0 FF 75 08 8B 45 14 83 EC 0C 8B 3E 8B D4 6A 01 89 0A 8B CE 89 42 04 8B 45 18"}};
    Feature Feature::byPass = {
        {0x1E0626},
        {0x47},
        {0x67},
        {0x1, "DC 67 68 C6"}};
    Feature Feature::clipCam = {
        {0x8D453A},
        {0x90, 0x90, 0x90},
        {0x0F, 0x29, 0x01},
        {0x0, "0F 29 01 C7 41 34 00 00 00 00 0F"}};
    Feature Feature::disMount = {
        {0x33CA8E},
        {0xEB},
        {0x74},
        {0x0, "74 XX 8B 07 8B CF 6A 00 6A 00 FF 50"}};
    Feature Feature::lockCam = {
        {0x8E2C45},
        {0xEB},
        {0x74},
        {0x0, "74 05 8B 01 FF 50 0C 8B E5"}};
    Feature Feature::unlockMapLimit = {
        {0x8926AD},
        {0xEB},
        {0x77},
        {0x0, "77 XX B8 XX XX XX XX F3 0F 10 08 F3 0F 11 89 XX XX XX XX 8B 89"}};
    Feature Feature::quickMining = {
        {0xAD2948},
        {0xF0},
        {0xF1},
        {0x1, "DF F1 DD D8 72 61"}};
    Feature Feature::quickMiningGeode = {
        {0xB12FB7},
        {0xF0},
        {0xF1},
        {0x1, "DF F1 DD D8 72 35 8D"}};
    Feature Feature::noGravity = {
        {0xEE285C, 0xC},
        {0x42, 0xC8},
        {0x0, 0x0},
        {-0x4, "F3 0F 11 45 FC D9 45 FC 8B E5 5D C3 D9 05 XX XX XX XX 8B E5 5D C3 D9 05 XX XX XX XX 8B E5 5D C3"}};

    Feature Feature::noClip = {
        {0x646B72},
        {0xE8, 0xFF, 0xFF, 0xFF, 0xFF, 0x90},
        {0x8B, 0x43, 0x14, 0x83, 0xC4, 0x8},
        {-0x5A3, "74 31 FF 73 14 8B 47 04 2B 07"},
        {0x58, 0x83, 0xC4, 0x08, 0x50, 0x8B, 0x43, 0x14, 0x53, 0x51, 0x83, 0xEC, 0x30, 0xF3, 0x0F, 0x7F, 0x44, 0x24, 0x20, 0xF3, 0x0F, 0x7F, 0x4C, 0x24, 0x10, 0xF3, 0x0F, 0x7F, 0x14, 0x24, 0xBB, 0xE0, 0xE6, 0x30, 0x1D, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x03, 0x1C, 0x8D, 0xFF, 0xFF, 0xFF, 0xFF, 0x8B, 0x1B, 0x83, 0xFB, 0x00, 0x0F, 0x84, 0x32, 0x00, 0x00, 0x00, 0x41, 0x83, 0xF9, 0x04, 0x7C, 0xE8, 0x0F, 0x10, 0x83, 0x80, 0x00, 0x00, 0x00, 0x0F, 0x28, 0xC8, 0x0F, 0xC2, 0xCC, 0x02, 0x0F, 0x28, 0xD5, 0x0F, 0xC2, 0xD0, 0x02, 0x66, 0x0F, 0xDB, 0xCA, 0x0F, 0x50, 0xC9, 0x83, 0xE1, 0x07, 0x83, 0xF9, 0x07, 0x0F, 0x85, 0x04, 0x00, 0x00, 0x00, 0xC6, 0x40, 0x01, 0x00, 0xF3, 0x0F, 0x6F, 0x14, 0x24, 0xF3, 0x0F, 0x6F, 0x4C, 0x24, 0x10, 0xF3, 0x0F, 0x6F, 0x44, 0x24, 0x20, 0x83, 0xC4, 0x30, 0x59, 0x5B, 0xC3, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0xC4, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00}};
    Feature Feature::unlockZoomLimit = {
        {0x8D2476},
        {0x57},
        {0x5F},
        {0x3, "F3 0F 11 5F 2C"}};

    void SetFeature(const Feature &feature, const Memory::DWORD &pid, const bool &on)
    {
        Game game(pid);
        game.UpdateAddress();
        const Memory::Address address = game.GetAddress(game.baseAddress, feature.offsets);
        game.WriteMemory(on ? feature.dataOn : feature.dataOff, address);
    }

    void SetNoClip(const Feature &feature, const Memory::DWORD &pid, const bool &on)
    {
        Game game(pid);
        game.UpdateAddress();
        const Memory::Address address = game.GetAddress(game.baseAddress, feature.offsets);
        Memory::Address inAddress;
        memcpy(&inAddress, feature.dataOn.data() + 0x1, sizeof(Memory::Address));
        inAddress += address + 5;
        if (inAddress != 0x0 && inAddress != 0xFFFFFFFF)
            game.FreeMemory(0, MEM_RELEASE, inAddress);
        if (on)
        {
            DWORD temp;
            memcpy((void *)(feature.dataIn.data() + 0x1F), &game.data.player.UpdateAddress().address, sizeof(Memory::Address));
            inAddress = game.AllocMemory(feature.dataIn.size(), MEM_COMMIT, PAGE_READWRITE);
            temp = inAddress + 0x83;
            memcpy((void *)(feature.dataIn.data() + 0x2B), &temp, sizeof(Memory::Address));
            temp = inAddress - address - 5;
            memcpy((void *)(feature.dataOn.data() + 0x1), &temp, sizeof(Memory::Address));
            game.WriteMemory(feature.dataIn, inAddress);
            VirtualProtectEx(game.hProcess, reinterpret_cast<LPVOID>(inAddress), feature.dataIn.size(), PAGE_EXECUTE, &temp);
        }
        game.WriteMemory(on ? feature.dataOn : feature.dataOff, address);
        // 新增：记录当前是否开启穿墙
        Module::noClipOn.store(on);
    }

    void SetAutoAttack(const Memory::DWORD &pid, const uint32_t &keep, const uint32_t &delay)
    {
        while (functionRunMap[{pid, "SetAutoAttack"}].load())
        {
            SetFeature(Feature::autoAttack, pid, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(keep));
            SetFeature(Feature::autoAttack, pid, false);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
    }

    bool AutoRespawn(const Memory::DWORD &pid, bool is_respawn)
    {
        Game game(pid);
        game.UpdateAddress().data.player.UpdateAddress();
        double hp = game.data.player.data.health
                        .UpdateAddress()
                        .UpdateData()
                        .data;
        if (hp <= 0.0)
        {
            if (is_respawn)
            {
                char buf[64];
                _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%lu|%s", (unsigned long)pid, "byFollow");
                AhkEvt::Emit(EVT_RESP_CAN_REVIVE, buf);
            }
            return true;
        }
        return false;
    }

    void SetAutoRespawn(const Memory::DWORD &pid, const uint32_t &delay)
    {
        Game game(pid);
        game.UpdateAddress().data.player.UpdateAddress();

        const uint32_t tick = delay ? delay : 50;

        while (functionRunMap[{pid, "SetAutoRespawn"}].load())
        {
            double hp = game.data.player.data.health
                            .UpdateAddress()
                            .UpdateData()
                            .data;

            if (hp <= 0.0 && !(functionRunMap[{pid, "FollowTarget"}].load()))
            {
                // 跟随中 → byFollow；否则 → now
                bool following = functionRunMap[{pid, "FollowTarget"}].load();
                char buf[64];
                _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%lu|%s",
                            (unsigned long)pid, "now");
                AhkEvt::Emit(EVT_RESP_CAN_REVIVE, buf);

                // 不用任何标记，简单节流一下，避免每 tick 都狂发
                std::this_thread::sleep_for(std::chrono::milliseconds(following ? 5000 : 200));
                continue; // 继续下一轮检测
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(tick));
        }
    }

    // 纯本地状态变量（线程内）
    void SetAutoPotion(const Memory::DWORD &pid,
                       uint32_t minIntervalMs,
                       uint32_t stopAtCount /* 先留参数位，库存判定你后面要用也行 */,
                       uint32_t delay /* 采样间隔，默认 50ms */)
    {
        Game game(pid);
        game.UpdateAddress().data.player.UpdateAddress();

        double lastHp = -1.0;
        auto lastDrink = std::chrono::steady_clock::now() - std::chrono::hours(1);

        const uint32_t tick = delay ? delay : 50;

        while (functionRunMap[{pid, "SetAutoPotion"}].load())
        {
            // 采 HP
            double hp = game.data.player.data.health
                            .UpdateAddress()
                            .UpdateData()
                            .data;

            // HP 下降触发
            if (lastHp >= 0.0 && hp < lastHp)
            {
                auto now = std::chrono::steady_clock::now();
                auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastDrink).count();
                if (diff >= (int)minIntervalMs)
                {
                    // 通知 AHK 去按 Q
                    char buf[64];
                    _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%lu|hp_drop", (unsigned long)pid);
                    AhkEvt::Emit(EVT_USE_POTION, buf);
                    lastDrink = now;
                    // LOGF("[AutoPotion] %.3f -> %.3f, emit EVT_USE_POTION", lastHp, hp);
                }
            }

            lastHp = hp;
            std::this_thread::sleep_for(std::chrono::milliseconds(tick));
        }
    }

    void AutoAim(const Memory::DWORD &pid, const bool &targetBoss, const bool &targetPlant, const bool &targetNormal, const std::vector<std::string> &targets, const std::vector<std::string> &noTargets, const uint32_t &range, const uint32_t &delay)
    {
        Game game(pid);
        game.UpdateAddress().data.player.UpdateAddress();
        std::unique_ptr<Game::World::Entity> target = nullptr;
        auto UpdateAddress = [&game]()
        {
            game.data.player.data.coord.UpdateAddress();
            game.data.player.data.camera.UpdateAddress();
            game.data.player.data.coord.data.x.UpdateAddress();
            game.data.player.data.coord.data.y.UpdateAddress();
            game.data.player.data.coord.data.z.UpdateAddress();
            game.data.player.data.camera.data.v.UpdateAddress();
            game.data.player.data.camera.data.h.UpdateAddress();
        };
        while (functionRunMap[{pid, "AutoAim"}].load())
        {
            uint32_t step = 0;
            UpdateAddress();
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            target = FindTarget(game, targetBoss, targetPlant, targetNormal, targets, noTargets, range);
            if (!target)
                continue;
            while (functionRunMap[{pid, "AutoAim"}].load() &&
                   CalculateDistance(
                       game.data.player.data.coord.data.x.UpdateData().data,
                       game.data.player.data.coord.data.y.UpdateData().data + aimOffset.first,
                       game.data.player.data.coord.data.z.UpdateData().data,
                       target->data.x.UpdateData().data,
                       target->data.y.UpdateData().data + aimOffset.second,
                       target->data.z.UpdateData().data) <= range &&
                   target->data.isDeath.UpdateData().data)
            {
                UpdateAddress();
                auto vh = CalculateAngles(
                    game.data.player.data.coord.data.x.data,
                    game.data.player.data.coord.data.y.data,
                    game.data.player.data.coord.data.z.data,
                    target->data.x.data,
                    target->data.y.data,
                    target->data.z.data);
                game.data.player.data.camera.data.v = vh.first;
                game.data.player.data.camera.data.h = vh.second;
                if (((step = (step + 1) % 100) == 0) ||
                    target->data.health.UpdateData().data < 1 ||
                    (target->data.x.data < 1 && target->data.y.data < 1 && target->data.z.data < 1))
                {
                    const auto &entitys = game.data.world.UpdateAddress().UpdateData().data.entitys;
                    if (std::find(entitys.begin(), entitys.end(), *target) == entitys.end())
                        break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            }
        }
    }

    void SpeedUp(const Memory::DWORD &pid, const float &speed, const uint32_t &delay, const std::vector<std::string> &hotKey)
    {
    }

    void Tp2Forward(const Memory::DWORD &pid, const float &tpRange, const uint32_t &delay)
    {
        Game game(pid);
        game.UpdateAddress().data.player.UpdateAddress();
        game.data.player.data.coord.UpdateAddress();
        game.data.player.data.camera.UpdateAddress();
        Tp2Target(
            pid,
            game.data.player.data.coord.data.x.UpdateAddress().UpdateData().data +
                game.data.player.data.camera.data.xPer.UpdateAddress().UpdateData().data * tpRange,
            game.data.player.data.coord.data.y.UpdateAddress().UpdateData().data +
                game.data.player.data.camera.data.yPer.UpdateAddress().UpdateData().data * tpRange,
            game.data.player.data.coord.data.z.UpdateAddress().UpdateData().data +
                game.data.player.data.camera.data.zPer.UpdateAddress().UpdateData().data * tpRange,
            delay);
    }

    void Tp2Target(const Memory::DWORD &pid, const float &targetX, const float &targetY, const float &targetZ, const uint32_t &delay, const uint32_t &tryAgainMax)
    {
        Game game(pid);
        game.UpdateAddress().data.player.UpdateAddress();
        uint32_t tryAgain = 0;
        float dist = 0, lastDist = 0, x = 0, y = 0, z = 0;
        do
        {
            game.data.player.data.coord.UpdateAddress();
            x = game.data.player.data.coord.data.x.UpdateAddress().UpdateData().data;
            y = game.data.player.data.coord.data.y.UpdateAddress().UpdateData().data;
            z = game.data.player.data.coord.data.z.UpdateAddress().UpdateData().data;
            dist = CalculateDistance(x, y, z, targetX, targetY, targetZ);
            tryAgain += dist >= lastDist || dist <= tpStep;
            game.data.player.data.coord.data.x = (dist > tpStep ? x + (targetX - x) / dist * tpStep : targetX);
            game.data.player.data.coord.data.y = (dist > tpStep ? y + (targetY - y) / dist * tpStep : targetY);
            game.data.player.data.coord.data.z = (dist > tpStep ? z + (targetZ - z) / dist * tpStep : targetZ);
            lastDist = dist;
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        } while (tryAgain < tryAgainMax && dist > tpStep);
    }

    struct PrioritizedEntity
    {
        int priority;
        Game::World::Entity entity;
        float distance;
        std::string name;

        // 添加一个构造函数
        PrioritizedEntity(int p, Game::World::Entity e, float d, std::string n)
            : priority(p), entity(e), distance(d), name(n) {}

        // 自定义比较运算符，用于排序
        bool operator<(const PrioritizedEntity &other) const
        {
            if (priority != other.priority)
                return priority > other.priority; // 优先级高的在前
            return distance < other.distance;     // 距离近的在前
        }
    };

    inline bool BlackListed(float ex, float ez)
    {
        // 加锁保护黑名单访问（多线程安全）
        std::lock_guard<std::mutex> lock(Module::blacklistMutex);

        // 遍历黑名单中的所有宝箱
        for (const auto &b : Module::blacklistedChests)
        {
            // 获取黑名单中宝箱的X和Z坐标
            float bx = std::get<0>(b), bz = std::get<2>(b);

            // 计算与黑名单宝箱的距离（只考虑XZ平面，忽略Y轴）
            float dx = ex - bx, dz = ez - bz;

            // 如果距离小于8米（64是8的平方），则认为被黑名单覆盖
            if (dx * dx + dz * dz < 64.0f)
            {
                return true;
            }
        }
        return false;
    }

    // 定义优先处理的宝箱类型列表（这些宝箱会被优先考虑）
    static const std::vector<std::string> priorityChests = {
        "gameplay/chest_quest_rune_vault_01",     // 5*宝箱
        "gameplay/chest_quest_standard_large",    // 标准大宝箱
        "gameplay/chest_quest_standard_small",    // 标准小宝箱
        "gameplay/chest_quest_geode_5star_large", // 深渊大宝箱
        "gameplay/chest_quest_geode_5star_small", // 深渊小宝箱
        "chest_quest_recipe.*"                    // 配方宝箱
    };

    static const std::vector<std::string> ButtonStarts = {
        "quest_assault_trigger",
        "quest_spawn_trigger_fivestar_depths",
        "quest_spawn_trigger_fivestar"};

    std::vector<PrioritizedEntity> GetEntitysWithPriority(
        Game &game,
        float ax,
        float ay,
        float az,
        const uint32_t &range,
        const bool &targetBoss,             // 是否以Boss级实体为目标
        const bool &targetPlant,            // 是否以植物类实体为目标
        const bool &targetNormal,           // 是否以普通实体为目标
        std::vector<std::string> targets,   // 目标名称白名单（正则表达式）
        std::vector<std::string> noTargets, // 目标名称黑名单（正则表达式）
        uint32_t paramBossLevel)
    {

        auto b2s = [](bool v)
        { return v ? "true" : "false"; };
        auto joinQuoted = [](const std::vector<std::string> &v)
        {
            std::string s;
            s.reserve(64);
            s += '[';
            for (size_t i = 0; i < v.size(); ++i)
            {
                if (i)
                    s += ", ";
                s += '\'';
                s += v[i];
                s += '\'';
            }
            s += ']';
            return s;
        };

        noTargets.reserve(noTargets.size() + timeOutIds.size());                 // 先预留，避免多次扩容
        noTargets.insert(noTargets.end(), timeOutIds.begin(), timeOutIds.end()); // 追加

        // 使用辅助函数创建正则表达式向量
        std::vector<std::regex> targetRegexs = createRegexVector(targets);
        std::vector<std::regex> noTargetRegexs = createRegexVector(noTargets);

        std::vector<PrioritizedEntity> result;

        auto &entitys = game.data.world.UpdateAddress().UpdateData().data.entitys;
        float distance = 0;
        int priority = 0;
        for (auto &entity : entitys)
        {
            distance = 0;
            priority = 0;
            entity.UpdateAddress().UpdateData();
            entity.data.level.UpdateData();

            // 如果实体已死亡，跳过处理
            if (entity.data.isDeath.UpdateData().data == 0)
                continue;

            float mx = entity.data.x.UpdateData().data; // 实体X坐标
            float my = entity.data.y.UpdateData().data; // 实体Y坐标
            float mz = entity.data.z.UpdateData().data; // 实体Z坐标
            // 计算实体与目标之间的距离
            distance = CalculateDistance(
                ax, ay, az, // 目标坐标
                mx, my, mz);

            if (distance > range)
                continue;

            auto name = entity.data.name.UpdateData(128).data;

            // 检查实体是否在黑名单中（使用正则表达式匹配）
            bool is_noTargetRegexs = false;
            for (const auto &rx : noTargetRegexs)
            {
                if (std::regex_search(name, rx))
                {
                    is_noTargetRegexs = true;
                    break;
                } // 子串匹配更稳
            }
            if (is_noTargetRegexs)
                continue;

            // 检查实体是否在白名单中（使用正则表达式匹配）
            bool is_targetRegexs = false;
            for (const auto &rx : targetRegexs)
            {
                if (std::regex_search(name, rx))
                {
                    is_targetRegexs = true;
                    break;
                } // 子串匹配更稳
            }
            if (is_targetRegexs)
            {
                priority += 800;
                if (name.find("gameplay/chest_quest_rune_vault_01") != std::string::npos && BlackListed(mx, mz))
                {
                    continue;
                }

                for (const auto &chest : priorityChests)
                {
                    // 如果实体名称包含优先宝箱的关键字
                    if (name.find(chest) != std::string::npos)
                    {
                        priority += 1000;
                    }
                }

                for (const auto &start : ButtonStarts)
                {
                    // 如果实体名称包含优先开关的关键字
                    if (name.find(start) != std::string::npos)
                    {
                        priority += 100;
                    }
                }
            }

            // 如果实体名称包含"npc"（是非玩家角色）
            if (std::regex_match(name, std::regex(".*npc.*")))
            {
                if (!(entity.data.level.UpdateData().data >= paramBossLevel))
                {
                    continue;
                }

                // 如果设置了以Boss为目标，并且（实体等级达到Boss级别或名称包含"boss"）
                if (targetBoss)
                {
                    priority += 200;
                    if (std::regex_match(name, std::regex(".*boss.*")))
                    {
                        priority += 400;
                    }
                }

                // 如果设置了以普通实体为目标
                if (targetNormal)
                {
                    priority += 500;
                }
            }
            if (priority == 0)
            {
                continue;
            }

            if (isDevTools)
            {
                LOGD("[GetEntitysWithPriority] name=%s priority=%d  ax=%.2f ay=%.2f az=%.2f range=%u boss=%s plant=%s normal=%s level=%u "
                     "targets=%s noTargets=%s",
                     name.c_str(),
                     priority,
                     ax, ay, az, range,
                     b2s(targetBoss), b2s(targetPlant), b2s(targetNormal),
                     paramBossLevel,
                     joinQuoted(targets).c_str(),
                     joinQuoted(noTargets).c_str());
            }
            // 使用构造函数创建 PrioritizedEntity 对象
            result.push_back(PrioritizedEntity(priority, entity, distance, name));
        }

        // 排序
        std::sort(result.begin(), result.end());

        return result;
    }

    struct MoveCtx
    {
        float x = 0.f, y = 0.f, z = 0.f;
        float dist = 0.f, lastDist = 0.f;
        uint32_t step = 0;
        uint32_t safeDelay = 16;
        float speed = 50.f;
        Memory::DWORD pid = 0;
    };

    bool MoveUntilReached(
        Game &game,
        float targetX, float targetY, float targetZ)
    {
        game.UpdateAddress().data.player.UpdateAddress();
        float stop_eps = 1.0f;
        uint32_t timeout_ms = 60000;
        float speed = 50.f; // 固定速度（与老版 MoveEvent 保持一致）
        uint32_t delay_ms = 20;
        auto t0 = std::chrono::steady_clock::now();
        auto baseline_t = t0;
        float baseline_d = std::numeric_limits<float>::infinity();
        bool allow_tp = true;

        for (;;)
        {
            // 刷新坐标
            game.data.player.data.coord.UpdateAddress();
            float cx = game.data.player.data.coord.data.x.UpdateAddress().UpdateData().data;
            float cy = game.data.player.data.coord.data.y.UpdateAddress().UpdateData().data;
            float cz = game.data.player.data.coord.data.z.UpdateAddress().UpdateData().data;

            // 到达判定
            float dist = CalculateDistance(cx, cy, cz, targetX, targetY, targetZ);
            if (std::isnan(dist) || dist <= stop_eps)
            {
                game.data.player.data.coord.data.xVel.UpdateAddress() = 0;
                game.data.player.data.coord.data.yVel.UpdateAddress() = 0;
                game.data.player.data.coord.data.zVel.UpdateAddress() = 0;
                return !std::isnan(dist); // NaN 视为失败
            }

            // 正常推进：写速度 = 单位方向 * speed
            game.data.player.data.coord.data.xVel.UpdateAddress() = (targetX - cx) / dist * speed;
            game.data.player.data.coord.data.yVel.UpdateAddress() = (targetY - cy) / dist * speed;
            game.data.player.data.coord.data.zVel.UpdateAddress() = (targetZ - cz) / dist * speed;

            // 卡住监测 + 小跳解卡（复用你原本窗口：10*delay/20*delay）
            auto now = std::chrono::steady_clock::now();
            auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - baseline_t).count();
            if (dt >= (int)delay_ms * 20)
            {
                baseline_t = now;
                baseline_d = dist;
            }
            else if (dt >= (int)delay_ms * 10)
            {
                if (allow_tp && tpStep && std::abs(baseline_d - dist) < tpStep)
                {
                    uint32_t tries = (std::abs(baseline_d - dist) < 1.0f) ? 10u : 1u;
                    Tp2Target(game.pid, targetX, targetY, targetZ, delay_ms, tries);
                    // 重置基线，下一窗再评估
                    baseline_t = std::chrono::steady_clock::now();
                    baseline_d = std::numeric_limits<float>::infinity();
                }
            }

            // 超时保护
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - t0).count() >= timeout_ms)
            {
                game.data.player.data.coord.data.xVel.UpdateAddress() = 0;
                game.data.player.data.coord.data.yVel.UpdateAddress() = 0;
                game.data.player.data.coord.data.zVel.UpdateAddress() = 0;
                return false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
    }

    void FollowTarget(
        const Memory::DWORD &pid,                  // 目标进程ID
        const std::vector<std::string> &players,   // 要跟随的玩家名称列表
        const std::vector<std::string> &targets,   // 要攻击的目标名称列表（白名单）
        const std::vector<std::string> &noTargets, // 要忽略的目标名称列表（黑名单）
        const bool &targetBoss,                    // 是否攻击Boss级目标
        const bool &scanAll,                       // 是否开启全图扫描模式
        const float &speed,                        // 移动速度
        const uint32_t &delay)                     // 循环延迟时间（毫秒）
    {
        // 扫图相关参数
        static std::unordered_set<std::string> visitedPoints;
        static bool hasGoal = false;
        // static float targetX = 0.f, targetZ = 0.f;

        MoveCtx mv{.safeDelay = delay ? delay : 1, .speed = speed, .pid = pid};

        // 创建Game对象，用于与游戏进程交互
        Game game(pid);
        // game.UpdateAddress();
        // game.UpdateData(); // 使用新的方法名
        // uint64_t worldId = game.worldId.data;
        // LOGF("[当前世界ID] => %d", worldId);
        Game::World::Entity e(static_cast<Object<> &>(game));
        e.address = 0;

        // 更新游戏基址和玩家地址信息
        game.UpdateAddress().data.player.UpdateAddress().data.coord.UpdateAddress().UpdateData();
        float ax = game.data.player.data.coord.data.x.UpdateData().data;
        float ay = game.data.player.data.coord.data.y.UpdateData().data;
        float az = game.data.player.data.coord.data.z.UpdateData().data;
        float x = 0, y = 0, z = 0, targetX = 0;
        float targetY = 0, targetZ = 0, dist = 0, lastDist = 9999;
        std::chrono::steady_clock::time_point findTime, moveTime;

        auto MoveEvent = [&game, &x, &y, &z, &dist, &lastDist, &moveTime, &delay, &speed](
                             const float &targetX, float targetY, const float &targetZ)
        {
            x = game.data.player.data.coord.data.x.UpdateData().data;
            y = game.data.player.data.coord.data.y.UpdateData().data;
            z = game.data.player.data.coord.data.z.UpdateData().data;
            game.data.player.data.coord.data.xVel.UpdateAddress() = 0;
            game.data.player.data.coord.data.yVel.UpdateAddress() = 0;
            game.data.player.data.coord.data.zVel.UpdateAddress() = 0;
            if ((dist = CalculateDistance(x, y, z, targetX, targetY, targetZ)) <= 1 || std::isnan(dist))
                return;
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - moveTime)
                    .count() >= delay * 20)
            {
                moveTime = std::chrono::steady_clock::now();
                lastDist = dist;
            }

            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - moveTime)
                        .count() >= delay * 10 &&
                tpStep && abs(lastDist - dist) < tpStep)
                Tp2Target(game.pid, targetX, targetY, targetZ, delay, abs(lastDist - dist) < 1 ? 10 : 1);
            else
            {
                game.data.player.data.coord.data.xVel.UpdateAddress() = (targetX - x) / dist * speed;
                game.data.player.data.coord.data.yVel.UpdateAddress() = (targetY - y) / dist * speed;
                game.data.player.data.coord.data.zVel.UpdateAddress() = (targetZ - z) / dist * speed;
            }
        };

        while (functionRunMap[{pid, "FollowTarget"}].load())
        {

            if (functionRunMap[{pid, "SetAutoRespawn"}].load())
            {
                if (AutoRespawn(pid, true))
                {
                    LOGW("[玩家死亡] 等待复活...");
                    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
                    continue;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            // 更新游戏基址和玩家地址信息
            game.UpdateAddress().data.player.UpdateAddress().data.coord.UpdateAddress().UpdateData();
            ax = game.data.player.data.coord.data.x.UpdateData().data;
            ay = game.data.player.data.coord.data.y.UpdateData().data;
            az = game.data.player.data.coord.data.z.UpdateData().data;

            if (!players.empty())
            {
                std::unique_ptr<Game::World::Player> player = nullptr;
                game.data.player.data.coord.UpdateAddress();        // 更新坐标结构地址
                game.data.player.data.coord.data.x.UpdateAddress(); // 更新X坐标地址
                game.data.player.data.coord.data.y.UpdateAddress(); // 更新Y坐标地址
                game.data.player.data.coord.data.z.UpdateAddress(); // 更新Z坐标地址

                player = FindPlayer(game, players);
                if (player)
                {
                    float playerX = player->data.x.UpdateAddress().UpdateData().data;
                    float playerY = player->data.y.UpdateAddress().UpdateData().data;
                    float playerZ = player->data.z.UpdateAddress().UpdateData().data;
                    MoveEvent(playerX, playerY, playerZ);
                    continue;
                }
            }

            // 获取与自己距离最近的实体列表
            std::vector<PrioritizedEntity> entitys = GetEntitysWithPriority(
                game,
                ax, ay, az,
                realisticScanRange,
                targetBoss,
                false,
                false,
                targets,
                noTargets,
                bossLevel);

            if (entitys.empty())
            {
                if (scanAll)
                {
                    if (!hasGoal)
                    {
                        targetX = game.data.player.data.coord.data.x.UpdateData().data;
                        targetZ = game.data.player.data.coord.data.z.UpdateData().data;
                        // 扫图逻辑
                        GetNextPoint(
                            targetX,
                            targetZ,
                            visitedPoints);
                        hasGoal = true;
                        LOGF("[初始启动扫描无名单开始移动] => (%.0f, %.0f)", targetX, targetZ); // 只在这里打
                    }

                    MoveEvent(targetX, targetY = std::clamp((std::max)(targetY, game.data.player.data.coord.data.y.UpdateData().data), minY, maxY), targetZ);
                    // 到达格心 → 再取下一格
                    if (Module::Arrived2D(game, targetX, targetZ, 1.0f))
                    {
                        targetX = game.data.player.data.coord.data.x.UpdateData().data;
                        targetZ = game.data.player.data.coord.data.z.UpdateData().data;
                        GetNextPoint(targetX, targetZ, visitedPoints);
                        LOGF("[周边扫描无名单前往新格中心] => (%.0f,%.0f,%.0f)", targetX, targetY, targetZ);
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    }
                }
            }
            else
            {
                // 清理逻辑
                KillEntitys(pid, entitys, game, mv, 500, "", e, targets, noTargets);
            }
        }
    }

    inline bool EntityStillExists(Game &g, Game::World::Entity &e)
    {
        const auto &es = g.data.world.UpdateAddress().UpdateData().data.entitys;
        return std::find(es.begin(), es.end(), e) != es.end();
    }

    // 判近似：|a-b| <= eps
    inline bool Approx(float a, float b, float eps) { return std::fabs(a - b) <= eps; }

    // 哨兵坐标判定（两种模式：全部近 0；或 z ≈ 2）
    inline bool IsSentinelXYZ(float x, float y, float z, float eps = 5.f)
    {
        const bool nearZero = (std::fabs(x) <= eps && std::fabs(y) <= eps && std::fabs(z) <= eps);
        const bool zNearTwo = (std::fabs(x) <= eps && std::fabs(y) <= eps && Approx(z, 2.0f, eps));
        return nearZero || zNearTwo;
    }

    // 放在 namespace Module 里
    inline bool IsEmptyId(const std::string &s)
    {
        // 只要不含任何可打印字符(>32)就视为“空ID”
        for (unsigned char c : s)
            if (c > 32)
                return false;
        return true;
    }

    struct MoveCtx;
    void KillEntitys(
        const Memory::DWORD &pid,
        std::vector<PrioritizedEntity> &entitys,
        Game &game, MoveCtx &ctx,
        uint32_t sleepTime,
        std::string tab,
        Game::World::Entity overEntity,
        std::vector<std::string> targets,
        std::vector<std::string> noTargets)
    {
        int idx = 0;
        int total = static_cast<int>(entitys.size());
        static std::unordered_set<uintptr_t> printed;

        Game::World::Entity e(static_cast<Object<> &>(game));
        e.address = 0; // 标记为空

        std::vector<std::string> tr = {};
        std::vector<std::string> nr = noTargets;

        // 遍历entitys并输出日志
        for (auto &pe : entitys)
        {
            ++idx;
            // 依据名字是否命中 ButtonStarts 选择不同超时
            auto isButtonStart = [&](const std::string &nm)
            {
                return std::any_of(
                    ButtonStarts.begin(), ButtonStarts.end(),
                    [&](const std::string &key)
                    {
                        return !key.empty() && nm.find(key) != std::string::npos; // 子串命中即可
                    });
            };
            auto makeKey = [&](uintptr_t a, const std::string &nm, const std::string &tb) -> uint64_t
            {
                return (uint64_t)a ^ (std::hash<std::string>{}(tb) << 1) ^ (std::hash<std::string>{}(nm) << 2);
            };

            bool is_continue = false;
            Game::World::Entity entity = pe.entity;
            std::string name = entity.data.name.UpdateData(128).data; // 每次都取 string

            const auto t0 = std::chrono::steady_clock::now();
            const uint32_t timeoutMs = isButtonStart(name) ? 4u * 60u * 1000u : 1u * 60u * 1000u; // 4min / 2min

            while (true)
            {
                if (functionRunMap[{pid, "FollowTarget"}].load() == false)
                    return;

                if (functionRunMap[{pid, "SetAutoRespawn"}].load())
                {
                    game.UpdateAddress().data.player.UpdateAddress();
                    double hp = game.data.player.data.health
                                    .UpdateAddress()
                                    .UpdateData()
                                    .data;
                    if (hp <= 0.0)
                    {
                        if (tab != "")
                        {
                            LOGW("[玩家死亡] 优先退出多层方法嵌套");
                            return;
                        }
                        if (AutoRespawn(pid, false))
                        {
                            LOGW("[玩家死亡] 退出待清理名单, 等待复活...");
                            return;
                        }
                    }
                }

                entity.UpdateAddress().UpdateData();
                // ★ 每圈刷新：地址 & 名字
                uintptr_t addr = (uintptr_t)entity.address;
                name = entity.data.name.UpdateData(128).data;
                float ex = entity.data.x.UpdateData().data;
                float ey = entity.data.y.UpdateData().data;
                float ez = entity.data.z.UpdateData().data;
                // ★ 用“地址+tab”做复合键，避免递归时同地址被抑制
                uint64_t pkey = makeKey(addr, name, tab);

                if (IsEmptyId(name))
                {
                    // LOGF("[空ID]优先级: %d, 等级:%u, 坐标(%.2f, %.2f, %.2f), 已跳过",
                    //      pe.priority,
                    //      entity.data.level.UpdateAddress().UpdateData().data,
                    //      ex, ey, ez);
                    printed.erase(pkey);
                    // is_continue = true;
                    break; // 遇到空ID就跳过
                }

                if (entity.data.isDeath.UpdateAddress().UpdateData().data == 0 || !EntityStillExists(game, entity))
                {
                    std::string TAB = std::string("[已消失]" + tab + " => ");
                    LOGF("[%02d|%02d]%sid: %s, 优先级: %d",
                         idx, total,
                         TAB.c_str(),
                         name.c_str(),
                         pe.priority);
                    printed.erase(pkey);
                    break;
                }

                if (name != pe.name)
                {
                    if (isDevTools)
                    {
                        LOGW("[%02d|%02d][警告] 遇到实体名字变化，已跳过该目标 (旧=%s, 新=%s)", idx, total, pe.name.c_str(), name.c_str());
                    }
                    is_continue = true;
                    break;
                }

                // 加入这个方法迫于无奈 发现祭坛嵌套刷小怪代码 有极小规律卡传送门不停跟踪. 很难理解
                if (overEntity.address != 0)
                {
                    overEntity.UpdateAddress().UpdateData();
                    if (overEntity.data.isDeath.UpdateAddress().UpdateData().data == 0 || !EntityStillExists(game, overEntity))
                    {
                        // LOGF("祭坛已消失 退出多层方法嵌套");
                        return;
                    }
                }

                // 超时保护：超时则退出该目标
                const auto now = std::chrono::steady_clock::now();
                const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - t0).count();
                if (elapsed >= timeoutMs)
                {
                    is_continue = true;
                    game.UpdateAddress().data.player.UpdateAddress().data.coord.UpdateAddress().UpdateData();
                    float ax = game.data.player.data.coord.data.x.UpdateData().data;
                    float ay = game.data.player.data.coord.data.y.UpdateData().data;
                    float az = game.data.player.data.coord.data.z.UpdateData().data;
                    float currentDist = CalculateDistance(ax, ay, az, ex, ey, ez);
                    if (currentDist >= 5.0f)
                    {
                        LOGW("[%02d|%02d][警告超时退出] => id:%s, 在%.1fS内无法接近名单.目标距离过远(%.2fm) 疑似穿墙/绕过功能失效?", idx, total, name.c_str(), elapsed / 1000.0, currentDist);
                        break;
                    }

                    std::vector<std::regex> targetRegexs = createRegexVector(targets);
                    // 检查实体是否在黑名单中（使用正则表达式匹配）
                    bool is_targets = false;
                    for (const auto &rx : targetRegexs)
                    {
                        if (std::regex_search(name, rx))
                        {
                            is_targets = true;
                            break;
                        } // 子串匹配更稳
                    }
                    if (is_targets)
                    {
                        LOGE("[%02d|%02d][超时退出] id:%s, 优先级:%d, 已运行=%.1fs (上限=%.1fs) [未拉入临时黑名单. 因为他存在目标名单中.]",
                             idx, total, name.c_str(), pe.priority, elapsed / 1000.0, timeoutMs / 1000.0);
                    }
                    else
                    {
                        timeOutIds.emplace_back(name);
                        LOGE("[%02d|%02d][超时退出] id:%s, 优先级:%d, 已运行=%.1fs (上限=%.1fs) [已拉入本次运行临时黑名单. 如需匹配请重启软件]",
                             idx, total, name.c_str(), pe.priority, elapsed / 1000.0, timeoutMs / 1000.0);
                    }

                    break;
                }

                if (IsSentinelXYZ(ex, ey, ez, /*eps=*/5.f))
                {
                    // LOGF("[哨兵坐标]id: %s, 优先级: %d, 等级:%u, 坐标(%.2f, %.2f, %.2f), 已跳过",
                    //      name.c_str(),
                    //      pe.priority,
                    //      entity.data.level.UpdateAddress().UpdateData().data,
                    //      ex, ey, ez);
                    is_continue = true;
                    break; // 遇到哨兵坐标就跳过
                }

                // 进入该目标时打印一次
                if (printed.insert(pkey).second)
                {
                    std::string TAB = std::string("[已发现]" + tab + " => ");
                    if (isDevTools)
                    {
                        LOGF("[%02d|%02d]%sid:%s, old_id:%s, 优先级:%d, 初距:%.2f, 等级:%u, 坐标(%.2f, %.2f, %.2f)",
                             idx, total,
                             TAB.c_str(),
                             name.c_str(),
                             pe.name.c_str(),
                             pe.priority,
                             pe.distance, // 这是快照距离；若要实时距离，可改成现算
                             entity.data.level.UpdateAddress().UpdateData().data,
                             ex, ey, ez);
                    }
                    else
                    {
                        LOGF("[%02d|%02d]%sid:%s, 优先级:%d, 初距:%.2f, 等级:%u, 坐标(%.2f, %.2f, %.2f)",
                             idx, total,
                             TAB.c_str(),
                             pe.name.c_str(),
                             pe.priority,
                             pe.distance, // 这是快照距离；若要实时距离，可改成现算
                             entity.data.level.UpdateAddress().UpdateData().data,
                             ex, ey, ez);
                    }
                }

                if (name.find("gameplay/chest_quest_rune_vault_01") != std::string::npos)
                {
                    auto chestStartTime = std::chrono::steady_clock::now();
                    while (BlackListed(ex, ez) == false)
                    {
                        if (functionRunMap[{pid, "FollowTarget"}].load() == false)
                            return;
                        // MoveEvent(game, ctx, ex, ey + 1.5f, ez);
                        MoveUntilReached(game, ex, ey + 1.5f, ez);
                        game.UpdateAddress().data.player.UpdateAddress().data.coord.UpdateAddress().UpdateData();
                        float ax = game.data.player.data.coord.data.x.UpdateData().data;
                        float ay = game.data.player.data.coord.data.y.UpdateData().data;
                        float az = game.data.player.data.coord.data.z.UpdateData().data;
                        float currentDist = CalculateDistance(ax, ay, az, ex, ey, ez);
                        if (currentDist < 6.0f)
                        {
                            // 计算已经卡在宝箱附近的时间
                            auto now = std::chrono::steady_clock::now();
                            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - chestStartTime).count();
                            // 如果超过15秒，加入黑名单
                            if (elapsed >= 15)
                            {
                                LOGF("[深渊核心宝箱] => 已滞留满时间");
                                // 加锁保护黑名单列表
                                std::lock_guard<std::mutex> lock(Module::blacklistMutex);
                                // 如果黑名单已满，移除最旧的条目
                                if (Module::blacklistedChests.size() >= 1000)
                                    Module::blacklistedChests.pop_front();
                                // 添加新条目
                                Module::blacklistedChests.emplace_back(ex, ey, ez);
                                break;
                            }
                        }
                    }
                    break;
                }

                if (std::find(ButtonStarts.begin(), ButtonStarts.end(), name) != ButtonStarts.end())
                {
                    std::vector<std::string> _targets = {};
                    std::vector<std::string> _noTargets = noTargets;
                    if (name.find("quest_assault_trigger") != std::string::npos)
                    {
                        // 先到达开关
                        while (true)
                        {
                            game.UpdateAddress().data.player.UpdateAddress().data.coord.UpdateAddress().UpdateData();
                            float ax = game.data.player.data.coord.data.x.UpdateData().data;
                            float ay = game.data.player.data.coord.data.y.UpdateData().data;
                            float az = game.data.player.data.coord.data.z.UpdateData().data;
                            float currentDist = CalculateDistance(ax, ay, az, ex, ey, ez);
                            if (currentDist < 2.0f)
                            {
                                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                                break;
                            }
                            MoveUntilReached(game, ex, ey, ez);
                        }
                        _noTargets.emplace_back("quest_assault_trigger");
                        _noTargets.emplace_back(".*portal_.*");
                        std::vector<PrioritizedEntity> _entitys = GetEntitysWithPriority(
                            game,
                            ex, ey, ez,
                            15,
                            true,
                            false,
                            true,
                            _targets,
                            _noTargets,
                            2);
                        if (!_entitys.empty())
                        {
                            KillEntitys(pid, _entitys, game, ctx, 1, std::string(tab + " [[祭坛]") + name + "]", entity, targets, noTargets);
                        }
                    }
                    else if (name.find("quest_spawn_trigger_fivestar") != std::string::npos || name.find("quest_spawn_trigger_fivestar_depths") != std::string::npos)
                    {
                        _targets.emplace_back("quest_assault_trigger");
                        if (name.find("quest_spawn_trigger_fivestar_depths") != std::string::npos)
                        {
                            _targets.emplace_back("gameplay/chest_quest_rune_vault_01");
                            _targets.emplace_back("gameplay/chest_quest_geode_5star_large");
                            _targets.emplace_back("gameplay/chest_quest_geode_5star_small");
                        }
                        else if (name.find("quest_spawn_trigger_fivestar") != std::string::npos)
                        {
                            _targets.emplace_back("gameplay/chest_quest_standard_large");
                            _targets.emplace_back("gameplay/chest_quest_standard_small");
                        }
                        // MoveEventBlocking(game, ctx, ex, ey + 2.0f, ez,
                        //                   ctx.safeDelay, /*baseSpeed=*/ctx.speed,
                        //                   /*stopDist=*/1.0f, /*timeout=*/60000,
                        //                   /*slowRadius=*/8.0f, /*accelPerTick=*/0.8f, /*speedCap=*/10.0f);

                        // 等待按钮被开启 偶然发现这里的开关存活状态居然会失效 那就采用坐标+停留时间判定
                        auto chestStartTime = std::chrono::steady_clock::now();
                        while (entity.data.isDeath.UpdateAddress().UpdateData().data != 0 || EntityStillExists(game, entity))
                        {
                            if (functionRunMap[{pid, "FollowTarget"}].load() == false)
                                return;
                            // MoveEvent(game, ctx, ex, ey + 1.5f, ez);
                            MoveUntilReached(game, ex, ey + 1.5f, ez);
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            game.UpdateAddress().data.player.UpdateAddress().data.coord.UpdateAddress().UpdateData();
                            float ax = game.data.player.data.coord.data.x.UpdateData().data;
                            float ay = game.data.player.data.coord.data.y.UpdateData().data;
                            float az = game.data.player.data.coord.data.z.UpdateData().data;
                            float currentDist = CalculateDistance(ax, ay, az, ex, ey, ez);
                            if (currentDist < 3.0f)
                            {
                                // 计算已经卡在宝箱附近的时间
                                auto now = std::chrono::steady_clock::now();
                                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - chestStartTime).count();
                                // 如果超过15秒，加入黑名单
                                if (elapsed >= 3)
                                {
                                    LOGF("[5*开关存活状态疑似bug] => 坐标已滞留满3秒");
                                    entity.data.isDeath.UpdateAddress().UpdateData().data = 0;
                                    break;
                                }
                            }
                        }

                        std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
                        int emptyStreak = 0;
                        while (emptyStreak < 3 && functionRunMap[{pid, "FollowTarget"}].load())
                        {
                            if (functionRunMap[{pid, "FollowTarget"}].load() == false)
                                return;
                            if (functionRunMap[{pid, "SetAutoRespawn"}].load())
                            {
                                if (AutoRespawn(pid, false))
                                {

                                    LOGW("[玩家死亡] 退出5*副本...");
                                    return;
                                }
                            }
                            std::vector<PrioritizedEntity> _entitys5 = GetEntitysWithPriority(
                                game,
                                ex, ey, ez,
                                130,
                                /*targetBoss=*/true,
                                /*targetPlant=*/false,
                                /*targetNormal=*/false,
                                _targets,
                                _noTargets,
                                bossLevel);

                            if (!_entitys5.empty())
                            {
                                KillEntitys(pid, _entitys5, game, ctx, 500,
                                            std::string(tab + " [[5*]") + name + "]",
                                            e, targets, noTargets);
                                emptyStreak = 0;
                                continue;
                            }
                            else
                            {
                                // MoveEvent(game, ctx, ex, ey, ez);
                                MoveUntilReached(game, ex, ey, ez);
                                std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
                            }

                            ++emptyStreak;
                        }
                        LOGF("[%02d|%02d][5*指定范围已全部清除]id: %s, 优先级: %d",
                             idx, total,
                             name.c_str(),
                             pe.priority);
                    }
                }

                // LOGF("[移动中]id: %s, 优先级: %d, 距离: %.2f, 坐标(%.2f, %.2f, %.2f)",
                //      name.c_str(),
                //      pe.priority,
                //      CalculateDistance(
                //          game.data.player.data.coord.data.x.UpdateData().data,
                //          game.data.player.data.coord.data.y.UpdateData().data,
                //          game.data.player.data.coord.data.z.UpdateData().data,
                //          ex, ey, ez),
                //      ex, ey, ez + 2.0f);

                // 加2米 不然在地下做运动呢
                // MoveEventBlocking(game, ctx, ex, ey + 2.0f, ez,
                //                   ctx.safeDelay, /*baseSpeed=*/ctx.speed,
                //                   /*stopDist=*/1.0f, /*timeout=*/60000,
                //                   /*slowRadius=*/8.0f, /*accelPerTick=*/0.8f, /*speedCap=*/10.0f);

                // MoveEvent(game, ctx, ex, ey + 1.5f, ez);
                MoveUntilReached(game, ex, ey + 1.5f, ez);
            }
            if (is_continue)
            {
                if (isDevTools)
                {
                    LOGW("[跳过目标] id:%s", name.c_str());
                }
                continue;
            }
            if (sleepTime != 0 && overEntity.address == 0)
            {
                if (name.find("gameplay/chest_quest_rune_vault_01") != std::string::npos)
                {
                    continue;
                }

                using clock = std::chrono::steady_clock;
                const auto deadline = clock::now() + std::chrono::milliseconds(2000);
                const auto tick = std::chrono::milliseconds(120); // 轮询步长(可调)

                std::vector<PrioritizedEntity> chestList;

                while (clock::now() < deadline && functionRunMap[{pid, "FollowTarget"}].load())
                {
                    // 刷新自己坐标
                    game.UpdateAddress().data.player.UpdateAddress().data.coord.UpdateAddress().UpdateData();
                    float lx = game.data.player.data.coord.data.x.UpdateData().data;
                    float ly = game.data.player.data.coord.data.y.UpdateData().data;
                    float lz = game.data.player.data.coord.data.z.UpdateData().data;

                    // 拉取 100m 内候选
                    chestList = GetEntitysWithPriority(
                        game,
                        lx, ly, lz,
                        /*range=*/10,
                        /*targetBoss=*/false,
                        /*targetPlant=*/false,
                        /*targetNormal=*/false,
                        priorityChests, nr,
                        /*paramBossLevel=*/1);

                    // 只保留优先宝箱三类
                    chestList.erase(
                        std::remove_if(
                            chestList.begin(), chestList.end(),
                            [&](PrioritizedEntity &pe)
                            {
                                std::string nm = pe.entity.data.name.UpdateData(128).data;
                                bool isPriorityChest = std::any_of(
                                    priorityChests.begin(), priorityChests.end(),
                                    [&](const std::string &key)
                                    { return !key.empty() && nm.find(key) != std::string::npos; });
                                return !isPriorityChest;
                            }),
                        chestList.end());

                    if (!chestList.empty())
                    {
                        // 命中则立刻处理，不再等足 2 秒
                        KillEntitys(pid, chestList, game, ctx, /*sleepTime=*/0, /*tab=*/"", e, targets, noTargets);
                        break;
                    }

                    std::this_thread::sleep_for(tick); // 短退避后继续探测
                }
            }
        }
    }

    std::unique_ptr<Game::World::Player> FindPlayer(Game &game, const std::vector<std::string> &targets)
    {
        auto &players = game.data.world.UpdateAddress().UpdateData().data.players;
        std::vector<std::regex> targetRegexs;
        for (auto target : targets)
            targetRegexs.emplace_back(target);
        for (auto &player : players)
            player.UpdateAddress().data.name.UpdateAddress().UpdateData(64);
        for (auto targetRegex : targetRegexs)
            for (auto &player : players)
                if (std::regex_match(player.data.name.data, targetRegex))
                    return std::make_unique<Game::World::Player>(player);
        return nullptr;
    }

    // ========================== REPLACE ENTIRE METHOD ==========================
    // namespace Module { ... } scope
    // 修正：优先宝箱与通用分支的黑名单匹配都用 XZ 半径 8m；其它逻辑保持一致
    std::unique_ptr<Game::World::Entity> FindTarget(
        Game &game,
        const bool &targetBoss,
        const bool &targetPlant,
        const bool &targetNormal,
        const std::vector<std::string> &targets,
        const std::vector<std::string> &noTargets,
        const uint32_t &range)
    {
        auto &entitys = game.data.world.UpdateAddress().UpdateData().data.entitys;
        std::vector<std::regex> targetRegexs, noTargetRegexs;
        std::unique_ptr<Game::World::Entity> result = nullptr;
        float dist = 0, bestDist = 9999;
        uint32_t bestIndex = 0x7FFFFFFF;

        for (auto target : targets)
        {
            std::string trimmedTarget = trim(target);
            if (!trimmedTarget.empty())
                targetRegexs.emplace_back(trimmedTarget);
        }
        for (auto noTarget : noTargets)
        {
            std::string trimmedNoTarget = trim(noTarget);
            if (!trimmedNoTarget.empty())
                noTargetRegexs.emplace_back(trimmedNoTarget);
        }

        game.data.player.data.coord.data.x.UpdateData();
        game.data.player.data.coord.data.y.UpdateData();
        game.data.player.data.coord.data.z.UpdateData();

        // 优先宝箱
        for (auto &entity : entitys)
        {
            entity.UpdateAddress().UpdateData();

            if (entity.data.isDeath.UpdateData().data == 0)
                continue;

            dist = CalculateDistance(
                game.data.player.data.coord.data.x.data,
                game.data.player.data.coord.data.y.data,
                game.data.player.data.coord.data.z.data,
                entity.data.x.UpdateData().data,
                entity.data.y.UpdateData().data,
                entity.data.z.UpdateData().data);

            if (dist > range)
                continue;

            auto name = entity.data.name.UpdateData(128).data;
        }

        // 通用分支
        for (auto &entity : entitys)
        {
            entity.UpdateAddress().UpdateData();

            if (entity.data.isDeath.UpdateData().data == 0)
                continue;

            dist = CalculateDistance(
                game.data.player.data.coord.data.x.data,
                game.data.player.data.coord.data.y.data,
                game.data.player.data.coord.data.z.data,
                entity.data.x.UpdateData().data,
                entity.data.y.UpdateData().data,
                entity.data.z.UpdateData().data);

            if (dist > range || dist >= bestDist)
                continue;

            auto name = entity.data.name.UpdateData(128).data;

            // 先排除 noTargets
            size_t i = 0;
            while (i < noTargetRegexs.size() && !std::regex_match(name, noTargetRegexs[i]))
                i++;
            if (i < noTargetRegexs.size())
                continue;

            // targets 命中
            i = 0;
            while (i < targetRegexs.size() && !std::regex_match(name, targetRegexs[i]))
                i++;
            if (i < targetRegexs.size())
            {
                bestDist = dist;
                bestIndex = i;
                result = std::make_unique<Game::World::Entity>(entity);
                continue;
            }

            // plant
            if (targetPlant && std::regex_match(name, std::regex(".*plant.*")))
            {
                bestDist = dist;
                bestIndex = i;
                result = std::make_unique<Game::World::Entity>(entity);
                continue;
            }

            // npc：boss / normal
            if (std::regex_match(name, std::regex(".*npc.*")))
            {
                i++;
                if (i > bestIndex)
                    continue;

                if (targetBoss &&
                    (entity.data.level.UpdateData().data >= bossLevel ||
                     std::regex_match(name, std::regex(".*boss.*"))))
                {
                    bestDist = dist;
                    bestIndex = i;
                    result = std::make_unique<Game::World::Entity>(entity);
                    continue;
                }

                i++;
                if (i > bestIndex)
                    continue;

                if (targetNormal)
                {
                    bestDist = dist;
                    bestIndex = i;
                    result = std::make_unique<Game::World::Entity>(entity);
                    continue;
                }
            }
        }
        return result;
    }

    void GetNextPoint(float &x, float &z, std::unordered_set<std::string> &visitedPoints)
    {
        const float sqrt3 = std::sqrt(3);
        const float e = entityScand;
        const int R = static_cast<int>(mapWidth / (2 * e));
        auto alignToGrid = [&](float &x, float &z)
        {
            int j = static_cast<int>(std::round(z / (e * sqrt3)));
            int i = static_cast<int>(std::round((x - e * j) / (2 * e)));
            auto clampIndex = [&](int &i, int &j)
            {
                i = std::clamp(i, -R, R);
                j = std::clamp(j, -R, R);
                while (i * i + i * j + j * j > R * R)
                {
                    if (std::abs(i) > std::abs(j))
                        i > 0 ? i-- : i++;
                    else
                        j > 0 ? j-- : j++;
                }
            };
            clampIndex(i, j);
            x = e * (2 * i + j);
            z = e * sqrt3 * j;
        };
        auto getKey = [](float x, float z)
        {
            return std::to_string(static_cast<int>(std::round(x))) + "|" +
                   std::to_string(static_cast<int>(std::round(z)));
        };
        alignToGrid(x, z);
        if (visitedPoints.insert(getKey(x, z)).second)
            return;
        std::vector<std::pair<float, float>> directions = {
            {2 * e, 0}, {-2 * e, 0}, {e, e * sqrt3}, {-e, e * sqrt3}, {e, -e * sqrt3}, {-e, -e * sqrt3}};
        std::shuffle(directions.begin(), directions.end(), std::mt19937(std::random_device{}()));
        std::queue<std::pair<float, float>> searchQueue;
        std::unordered_set<std::string> checkedPoints;
        searchQueue.push({x, z});
        checkedPoints.insert(getKey(x, z));
        while (!searchQueue.empty())
        {
            auto [currentX, currentZ] = searchQueue.front();
            searchQueue.pop();
            for (const auto &[dx, dz] : directions)
            {
                float newX = currentX + dx;
                float newZ = currentZ + dz;
                alignToGrid(newX, newZ);
                std::string key = getKey(newX, newZ);
                float dist = std::sqrt(newX * newX + newZ * newZ);
                if (dist > mapWidth)
                    continue;
                if (!checkedPoints.count(key))
                {
                    checkedPoints.insert(key);
                    if (!visitedPoints.count(key))
                    {
                        visitedPoints.insert(key);
                        x = newX;
                        z = newZ;
                        return;
                    }
                    searchQueue.push({newX, newZ});
                }
            }
        }
        visitedPoints.clear();
        visitedPoints.insert(getKey(x, z));
    }
}

void UpdateConfig(const char *key, const char *value)
{
    const std::string _key = key;
    const std::vector<std::string> _value = split(value, '|');
    if (_value.empty() ||
        Module::configMap.find(_key) == Module::configMap.end())
        return;
    if (_value.size() >= 1 && _key == "Module::bossLevel")
        Module::bossLevel = std::stoul(_value[0]);
    else if (_value.size() >= 1 && _key == "Module::tpStep")
        Module::tpStep = std::stoul(_value[0]);
    else if (_value.size() >= 1 && _key == "Module::mapWidth")
        Module::mapWidth = std::stoul(_value[0]);
    else if (_value.size() >= 1 && _key == "Module::entityScand")
        Module::entityScand = std::stoul(_value[0]);
    else if (_value.size() >= 1 && _key == "Module::maxY")
        Module::maxY = std::stof(_value[0]);
    else if (_value.size() >= 1 && _key == "Module::minY")
        Module::minY = std::stof(_value[0]);
    else if (_value.size() >= 2 &&
             _key == "Module::aimOffset")
        Module::aimOffset = {std::stof(_value[0]), std::stof(_value[1])};
    else if (_key.find("ffsets") != std::string::npos)
    {
        Memory::Offsets *offsets = (Memory::Offsets *)Module::configMap[_key];
        offsets->clear();
        for (auto offset : _value)
            offsets->push_back(std::stol(offset, nullptr, 16));
    }
    else if (_value.size() >= 2 &&
             _key.find("ignature") != std::string::npos)
    {
        Object<>::Signature *signature = (Object<>::Signature *)Module::configMap[_key];
        signature->first = std::stol(_value[0], nullptr, 16);
        signature->second = _value[1];
    }
    else if (_value.size() >= 4 &&
             _key.find("Module::Feature") != std::string::npos)
    {
        Module::Feature *feature = (Module::Feature *)Module::configMap[_key];
        if (_value[0] != "-")
        {
            feature->offsets.clear();
            for (auto offset : split(_value[0], ','))
                feature->offsets.emplace_back(std::stol(offset, nullptr, 16));
        }
        if (_value[1] != "-")
        {
            feature->dataOn.clear();
            for (auto dataOn : split(_value[1], ','))
                feature->dataOn.emplace_back(std::stoul(dataOn, nullptr, 16));
        }
        if (_value[2] != "-")
        {
            feature->dataOff.clear();
            for (auto dataOff : split(_value[2], ','))
                feature->dataOff.emplace_back(std::stoul(dataOff, nullptr, 16));
        }
        if (_value[3] != "-")
        {
            const auto signature = split(_value[3], ',');
            feature->signature.first = std::stol(signature[0], nullptr, 16);
            feature->signature.second = signature[1];
        }
    }
}

void FunctionOn(const Memory::DWORD pid, const char *funtion, const char *argv, const bool waiting)
{
    std::thread *thread = nullptr;
    const std::vector<std::string> _argv = split(argv, '|');
    Module::functionRunMap[{pid, funtion}].store(true);
    if (std::strcmp(funtion, "AutoAim") == 0)
        thread = new std::thread(
            Module::AutoAim, pid,
            std::stoul(_argv[0]),
            std::stoul(_argv[1]),
            std::stoul(_argv[2]),
            split(_argv[3], ','),
            split(_argv[4], ','),
            std::stoul(_argv[5]),
            std::stoul(_argv[6]));
    else if (std::strcmp(funtion, "SpeedUp") == 0)
        thread = new std::thread(
            Module::SpeedUp, pid,
            std::stof(_argv[0]),
            std::stoul(_argv[1]),
            split(_argv[2], ','));
    else if (std::strcmp(funtion, "Tp2Forward") == 0)
        thread = new std::thread(
            Module::Tp2Forward, pid,
            std::stof(_argv[0]),
            std::stoul(_argv[1]));
    else if (std::strcmp(funtion, "Tp2Target") == 0)
        thread = new std::thread(
            Module::Tp2Target, pid,
            std::stof(_argv[0]),
            std::stof(_argv[1]),
            std::stof(_argv[2]),
            std::stoul(_argv[3]),
            std::stoul(_argv[4]));
    else if (std::strcmp(funtion, "FollowTarget") == 0)
        thread = new std::thread(
            Module::FollowTarget, pid,
            split(_argv[0], ','),
            split(_argv[1], ','),
            split(_argv[2], ','),
            std::stoul(_argv[3]),
            std::stoul(_argv[4]),
            std::stof(_argv[5]),
            std::stoul(_argv[6]));
    else if (std::strcmp(funtion, "SetNoClip") == 0)
        thread = new std::thread(Module::SetNoClip, Module::Feature::noClip, pid, std::stoul(_argv[0]));
    else if (std::strcmp(funtion, "SetAutoAttack") == 0)
        thread = new std::thread(Module::SetAutoAttack, pid, std::stoul(_argv[0]), std::stoul(_argv[1]));
    else if (std::strcmp(funtion, "SetAutoRespawn") == 0)
        thread = new std::thread(Module::SetAutoRespawn, pid, std::stoul(_argv[0]));
    else if (std::strcmp(funtion, "SetAutoPotion") == 0)
        thread = new std::thread(Module::SetAutoPotion, pid, std::stoul(_argv[0]), std::stoul(_argv[1]), std::stoul(_argv[2]));
    else if (std::strcmp(funtion, "SetHideAnimation") == 0)
        thread = new std::thread(Module::SetFeature, Module::Feature::hideAnimation, pid, std::stoul(_argv[0]));
    else if (std::strcmp(funtion, "SetBreakBlocks") == 0)
        thread = new std::thread(Module::SetFeature, Module::Feature::breakBlocks, pid, std::stoul(_argv[0]));
    else if (std::strcmp(funtion, "SetByPass") == 0)
        thread = new std::thread(Module::SetFeature, Module::Feature::byPass, pid, std::stoul(_argv[0]));
    else if (std::strcmp(funtion, "SetClipCam") == 0)
        thread = new std::thread(Module::SetFeature, Module::Feature::clipCam, pid, std::stoul(_argv[0]));
    else if (std::strcmp(funtion, "SetDisMount") == 0)
        thread = new std::thread(Module::SetFeature, Module::Feature::disMount, pid, std::stoul(_argv[0]));
    else if (std::strcmp(funtion, "SetLockCam") == 0)
        thread = new std::thread(Module::SetFeature, Module::Feature::lockCam, pid, std::stoul(_argv[0]));
    else if (std::strcmp(funtion, "SetUnlockMapLimit") == 0)
        thread = new std::thread(Module::SetFeature, Module::Feature::unlockMapLimit, pid, std::stoul(_argv[0]));
    else if (std::strcmp(funtion, "SetQuickMining") == 0)
        thread = new std::thread(Module::SetFeature, Module::Feature::quickMining, pid, std::stoul(_argv[0]));
    else if (std::strcmp(funtion, "SetQuickMiningGeode") == 0)
        thread = new std::thread(Module::SetFeature, Module::Feature::quickMiningGeode, pid, std::stoul(_argv[0]));
    else if (std::strcmp(funtion, "SetNoGravity") == 0)
        thread = new std::thread(Module::SetFeature, Module::Feature::noGravity, pid, std::stoul(_argv[0]));
    else if (std::strcmp(funtion, "SetUnlockZoomLimit") == 0)
        thread = new std::thread(Module::SetFeature, Module::Feature::unlockZoomLimit, pid, std::stoul(_argv[0]));
    if (thread && waiting)
    {
        thread->join();
        Module::functionRunMap[{pid, funtion}].store(false);
    }
}

void FunctionOff(const Memory::DWORD pid, const char *funtion)
{
    if (funtion == nullptr)
        for (auto &runThread : Module::functionRunMap)
            runThread.second.store(false);
    else
        Module::functionRunMap[{pid, funtion}].store(false);
}

void WhichTarget(const Memory::DWORD pid, char *result, const uint32_t size, const char *argv)
{
    Game game(pid);
    game.UpdateAddress().data.player.UpdateAddress().data.coord.UpdateAddress();
    game.data.player.data.coord.data.x.UpdateAddress();
    game.data.player.data.coord.data.y.UpdateAddress();
    game.data.player.data.coord.data.z.UpdateAddress();
    const std::vector<std::string> _argv = split(argv, '|');
    auto target = Module::FindTarget(
        game,
        std::stoul(_argv[0]),
        std::stoul(_argv[1]),
        std::stoul(_argv[2]),
        split(_argv[3], ','),
        split(_argv[4], ','),
        std::stoul(_argv[5]));
    if (target == nullptr)
        result[0] = '\0';
    else
    {
        auto vh = CalculateAngles(
            game.data.player.data.coord.data.x.data,
            game.data.player.data.coord.data.y.data,
            game.data.player.data.coord.data.z.data,
            target->data.x.data,
            target->data.y.data,
            target->data.z.data);

        game.data.player.data.camera.UpdateAddress();
        game.data.player.data.camera.data.v.UpdateAddress() = vh.first;
        game.data.player.data.camera.data.h.UpdateAddress() = vh.second;

        char buffer[1024] = "";
        sprintf(buffer, "%s,%d,%.3f,%.1f,%.1f,%.1f,%.1f",
                target->data.name.UpdateData(128).data.c_str(),
                std::clamp(target->data.level.UpdateData().data, 0u, 99u),
                std::clamp(target->data.health.UpdateData().data, 0.0, 999999999999999.0),
                CalculateDistance(game.data.player.data.coord.data.x.data,
                                  game.data.player.data.coord.data.y.data,
                                  game.data.player.data.coord.data.z.data,
                                  target->data.x.data,
                                  target->data.y.data,
                                  target->data.z.data),
                target->data.x.data,
                target->data.y.data,
                target->data.z.data);
        std::memcpy(result, buffer, std::min<uint32_t>(size, strlen(buffer)));
    }
}

#endif