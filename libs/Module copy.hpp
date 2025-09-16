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
#include <algorithm>
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

// 添加一个辅助函数来连接字符串向量
std::string join(const std::vector<std::string> &vec, const std::string &delimiter)
{
    std::ostringstream oss;
    for (size_t i = 0; i < vec.size(); ++i)
    {
        if (i != 0)
        {
            oss << delimiter;
        }
        oss << vec[i];
    }
    return oss.str();
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

    inline void logf(const char *fmt, ...)
    {
        if (!g_on)
            return;
        ensure_open_default();
        if (!g_fp)
            return;

        SYSTEMTIME st;
        GetLocalTime(&st);
        char head[64];
        _snprintf_s(head, sizeof(head), _TRUNCATE,
                    "[%02u:%02u:%02u.%03u] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

        char body[1024];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf_s(body, sizeof(body), _TRUNCATE, fmt, ap);
        va_end(ap);

        std::lock_guard<std::mutex> lk(g_mtx);
        fputs(head, g_fp);
        fputs(body, g_fp);
        fputc('\n', g_fp);
        fflush(g_fp);
    }
}
#define LOGF(...) SimpleLog::logf(__VA_ARGS__)

// ==== 给 AHK 用的导出函数 ====
extern "C"
{
    __declspec(dllexport) void SetLogPathA(const char *path) { SimpleLog::set_path(path); }
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

    // ===== 新增：扫图参数 & 状态 =====
    static uint32_t realisticScanRange = 90;             // NEW: 替代 9999 的真实扫描半径（米，经验值75~90）
    static std::unordered_set<std::string> switchIds = { // NEW: “开关ID”列表（仅作名称/关键词匹配）
        "quest_spawn_trigger_fivestar_depths", "quest_spawn_trigger_fivestar"};

    // 全局死亡标记
    static std::atomic<bool> isPlayerDead{false}; // NEW

    // 深渊宝箱黑名单存放
    static std::deque<std::tuple<float, float, float>> blacklistedChests;
    static std::mutex blacklistMutex;

    // 穿墙开关状态 & Z 轴偏移
    static std::atomic<bool> noClipOn{false};
    static float noClipZOffset = 2.0f; // 想改大小可通过 UpdateConfig 配

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
         {"Game::Player::Bag::offset", &Game::Player::Bag::offsets},
         // NEW: 让这些键可被 UpdateConfig 识别
         {"Module::SwitchIds", &Module::switchIds},
         {"Module::realisticScanRange", &Module::realisticScanRange}};

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

    // NEW: 名字里如果包含任一 switchId，就认为是“开关”
    inline bool IsSwitchName(const std::string &name)
    {
        for (const auto &k : Module::switchIds)
            if (!k.empty() && name.find(k) != std::string::npos)
                return true;
        return false;
    }

    inline bool Arrived2D(Game &game, float tx, float tz, float eps = 1.0f)
    {
        float px = game.data.player.data.coord.data.x.UpdateAddress().UpdateData().data;
        float pz = game.data.player.data.coord.data.z.UpdateAddress().UpdateData().data;
        float dx = px - tx, dz = pz - tz;
        return (dx * dx + dz * dz) <= (eps * eps);
    }

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
        {0x74B065},
        {0x4C},
        {0x44},
        {0x3, "F3 0F 11 44 24 24 F3 0F 58 84 24 80 00 00 00 50 F3 0F 11 43 24 E8 XX XX XX XX 8D 44 24 34 50"}};
    Feature Feature::autoAttack = {
        {0xB18278},
        {0xF0},
        {0xF1},
        {0x1, "DF F1 DD D8 72 1F"}};
    Feature Feature::breakBlocks = {
        {0x965523},
        {0x01},
        {0x00},
        {0x3, "80 7F XX 00 0F 84 XX XX XX XX 8B 4B 08 E8 XX XX XX XX FF 75 0C 8B 4D 10 8B F0 FF 75 08 8B 45 14 83 EC 0C 8B 3E 8B D4 6A 01 89 0A 8B CE 89 42 04 8B 45 18"}};
    Feature Feature::byPass = {
        {0x1AC696},
        {0x47},
        {0x67},
        {0x1, "DC 67 68 C6"}};
    Feature Feature::clipCam = {
        {0xA7B51A},
        {0x90, 0x90, 0x90},
        {0x0F, 0x29, 0x01},
        {0x0, "0F 29 01 C7 41 34 00 00 00 00 0F"}};
    Feature Feature::disMount = {
        {0x340D7E},
        {0xEB},
        {0x74},
        {0x0, "74 XX 8B 07 8B CF 6A 00 6A 00 FF 50"}};
    Feature Feature::lockCam = {
        {0x968655},
        {0xEB},
        {0x74},
        {0x0, "74 05 8B 01 FF 50 0C 8B E5"}};
    Feature Feature::unlockMapLimit = {
        {0xA0ABBD},
        {0xEB},
        {0x77},
        {0x0, "77 XX B8 XX XX XX XX F3 0F 10 08 F3 0F 11 89 XX XX XX XX 8B 89"}};
    Feature Feature::quickMining = {
        {0xA7C348},
        {0xF0},
        {0xF1},
        {0x1, "DF F1 DD D8 72 61"}};
    Feature Feature::quickMiningGeode = {
        {0x8844F7},
        {0xF0},
        {0xF1},
        {0x1, "DF F1 DD D8 72 35 8D"}};
    Feature Feature::noGravity = {
        {0xEE285C, 0xC},
        {0x42, 0xC8},
        {0x0, 0x0},
        {-0x4, "F3 0F 11 45 FC D9 45 FC 8B E5 5D C3 D9 05 XX XX XX XX 8B E5 5D C3 D9 05 XX XX XX XX 8B E5 5D C3"}};

    Feature Feature::noClip = {
        {0x63F042},
        {0xE8, 0xFF, 0xFF, 0xFF, 0xFF, 0x90},
        {0x8B, 0x43, 0x14, 0x83, 0xC4, 0x8},
        {-0x5A3, "74 31 FF 73 14 8B 47 04 2B 07"},
        {0x58, 0x83, 0xC4, 0x08, 0x50, 0x8B, 0x43, 0x14, 0x53, 0x51, 0x83, 0xEC, 0x30, 0xF3, 0x0F, 0x7F, 0x44, 0x24, 0x20, 0xF3, 0x0F, 0x7F, 0x4C, 0x24, 0x10, 0xF3, 0x0F, 0x7F, 0x14, 0x24, 0xBB, 0xE0, 0xE6, 0x30, 0x1D, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x03, 0x1C, 0x8D, 0xFF, 0xFF, 0xFF, 0xFF, 0x8B, 0x1B, 0x83, 0xFB, 0x00, 0x0F, 0x84, 0x32, 0x00, 0x00, 0x00, 0x41, 0x83, 0xF9, 0x04, 0x7C, 0xE8, 0x0F, 0x10, 0x83, 0x80, 0x00, 0x00, 0x00, 0x0F, 0x28, 0xC8, 0x0F, 0xC2, 0xCC, 0x02, 0x0F, 0x28, 0xD5, 0x0F, 0xC2, 0xD0, 0x02, 0x66, 0x0F, 0xDB, 0xCA, 0x0F, 0x50, 0xC9, 0x83, 0xE1, 0x07, 0x83, 0xF9, 0x07, 0x0F, 0x85, 0x04, 0x00, 0x00, 0x00, 0xC6, 0x40, 0x01, 0x00, 0xF3, 0x0F, 0x6F, 0x14, 0x24, 0xF3, 0x0F, 0x6F, 0x4C, 0x24, 0x10, 0xF3, 0x0F, 0x6F, 0x44, 0x24, 0x20, 0x83, 0xC4, 0x30, 0x59, 0x5B, 0xC3, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0xC4, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00}};
    Feature Feature::unlockZoomLimit = {
        {0xA79496},
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

    void SetAutoRespawn(const Memory::DWORD &pid, const uint32_t &delay)
    {
        Game game(pid);
        game.UpdateAddress().data.player.UpdateAddress();
        while (functionRunMap[{pid, "SetAutoRespawn"}].load()) // CHANGED: 独立控制
        {
            bool dead = (game.data.player.data.health.UpdateAddress().UpdateData().data == 0);
            Module::isPlayerDead.store(dead);

            if (dead)
            {
                // TODO: 这里可补“模拟按键E”复活
                // e.g. SendInput / PostMessage / WriteMemory...
                // 当前保留未完工标记
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
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
        // Game game(pid);
        // game.UpdateAddress().data.player.UpdateAddress();
        // float xVel = 0, yVel = 0, zVel = 0;
        // float xPer = 0, zPer = 0;
        // float hrzMagnitude = 0, xMagnitude = 0, zMagnitude = 0;
        // while (functionRunMap[{pid, "SpeedUp"}].load())
        // {
        //     game.data.player.data.coord.UpdateAddress();
        //     game.data.player.data.camera.UpdateAddress();
        //     xVel = yVel = zVel = 0;
        //     xPer = game.data.player.data.camera.data.xPer.UpdateAddress().UpdateData().data;
        //     zPer = game.data.player.data.camera.data.zPer.UpdateAddress().UpdateData().data;
        //     hrzMagnitude = std::sqrt(xPer * xPer + zPer * zPer);
        //     xMagnitude = xPer / hrzMagnitude;
        //     zMagnitude = zPer / hrzMagnitude;
        //     // 未完工, 按键触发
        //     // if()
        //     // {
        //     //     xVel += xMagnitude * speed;
        //     //     zVel += zMagnitude * speed;
        //     // }
        //     // if()
        //     // {
        //     //     xVel += zMagnitude * speed;
        //     //     zVel -= xMagnitude * speed;
        //     // }
        //     // if()
        //     // {
        //     //     xVel -= xMagnitude * speed;
        //     //     zVel -= zMagnitude * speed;
        //     // }
        //     // if()
        //     // {
        //     //     xVel -= zMagnitude * speed;
        //     //     zVel += xMagnitude * speed;
        //     // }
        //     // if()
        //     //     yVel += speed;
        //     // if()
        //     //     yVel -= speed;
        //     game.data.player.data.coord.data.xVel.UpdateAddress() = xVel;
        //     game.data.player.data.coord.data.yVel.UpdateAddress() = yVel;
        //     game.data.player.data.coord.data.zVel.UpdateAddress() = zVel;
        //     std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        // }
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

    // FollowTarget 函数：实现跟随目标（玩家或实体）的核心逻辑
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
        // 创建Game对象，用于与游戏进程交互
        Game game(pid);
        // 更新游戏基址和玩家地址信息
        game.UpdateAddress().data.player.UpdateAddress();

        // --- 工具函数定义 ---

        // UpdateAddress: 更新玩家坐标相关地址信息
        auto UpdateAddress = [&game]()
        {
            game.data.player.data.coord.UpdateAddress();        // 更新坐标结构地址
            game.data.player.data.coord.data.x.UpdateAddress(); // 更新X坐标地址
            game.data.player.data.coord.data.y.UpdateAddress(); // 更新Y坐标地址
            game.data.player.data.coord.data.z.UpdateAddress(); // 更新Z坐标地址
        };

        // MoveEvent: 实现平滑移动逻辑，计算速度向量并写入游戏内存
        auto MoveEvent = [&game, &speed](const float &tx, float ty, const float &tz)
        {
            // 获取玩家当前坐标
            float x = game.data.player.data.coord.data.x.UpdateData().data;
            float y = game.data.player.data.coord.data.y.UpdateData().data;
            float z = game.data.player.data.coord.data.z.UpdateData().data;

            // 计算与目标点的差值
            float dx = tx - x, dy = ty - y, dz = tz - z;
            // 计算距离
            float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
            // 如果距离太小或无效，则不移动
            if (!(dist > 1.0f) || std::isnan(dist))
                return;

            // 保存上一次的速度值（用于平滑）
            static float pvx = 0.f, pvy = 0.f, pvz = 0.f;
            // 计算基础速度向量
            const float inv = speed / std::max(dist, 1e-3f);
            float dvx = dx * inv, dvy = dy * inv, dvz = dz * inv;

            // 使用平滑因子alpha进行速度平滑
            const float alpha = 0.35f;
            pvx = pvx + alpha * (dvx - pvx);
            pvy = pvy + alpha * (dvy - pvy);
            pvz = pvz + alpha * (dvz - pvz);

            // 将计算出的速度写入游戏内存
            game.data.player.data.coord.data.xVel.UpdateAddress() = pvx;
            game.data.player.data.coord.data.yVel.UpdateAddress() = pvy;
            game.data.player.data.coord.data.zVel.UpdateAddress() = pvz;
        };

        // Arrived2DLocal: 检查是否在2D平面上到达目标点（忽略Y轴）
        auto Arrived2DLocal = [](Game &gameRef, float tx, float tz, float eps) -> bool
        {
            // 获取玩家当前XZ坐标
            float px = gameRef.data.player.data.coord.data.x.UpdateAddress().UpdateData().data;
            float pz = gameRef.data.player.data.coord.data.z.UpdateAddress().UpdateData().data;
            // 计算距离平方
            float dx = px - tx, dz = pz - tz;
            // 检查是否在误差范围内
            return (dx * dx + dz * dz) <= (eps * eps);
        };

        // --- "清理区外"扫格状态（原有OUT扫格保留） ---

        // 静态变量，用于保存扫图状态
        static bool out_has_goal = false;        // 是否有目标点
        static float out_gx = 0.f, out_gz = 0.f; // 目标点坐标
        // 已访问的点集合（用于扫图算法）
        std::unordered_set<std::string> visitedPoints;

        // 卡格检测状态变量
        static float lastPx = 1e9f, lastPz = 1e9f; // 上一次的位置
        static uint32_t stall = 0;                 // 卡住计数器

        // StepOutsideScan: 扫图模式下的移动逻辑
        auto StepOutsideScan = [&](void)
        {
            // 如果不是扫图模式，直接返回
            if (!scanAll)
                return;

            // 如果没有目标点，获取下一个目标点
            if (!out_has_goal)
            {
                // 从当前位置开始扫图
                out_gx = game.data.player.data.coord.data.x.UpdateData().data;
                out_gz = game.data.player.data.coord.data.z.UpdateData().data;
                // 获取下一个扫图点
                GetNextPoint(out_gx, out_gz, visitedPoints);
                out_has_goal = true;
                // 记录日志
                LOGF("[扫格 OUT set-goal] -> (%.1f,%.1f)", out_gx, out_gz);
            }

            // 获取当前高度并限制在合理范围内
            float curY = game.data.player.data.coord.data.y.UpdateData().data;
            float targetX = out_gx, targetZ = out_gz;
            float targetY = std::clamp(curY, Module::minY, Module::maxY);

            // 向目标点移动
            MoveEvent(targetX, targetY, targetZ);

            // 如果到达目标点，获取下一个点
            if (Arrived2DLocal(game, out_gx, out_gz, 1.0f))
            {
                out_gx = game.data.player.data.coord.data.x.UpdateData().data;
                out_gz = game.data.player.data.coord.data.z.UpdateData().data;
                GetNextPoint(out_gx, out_gz, visitedPoints);
                LOGF("[扫格 OUT arrived->next] -> (%.1f,%.1f) visited=%zu", out_gx, out_gz, visitedPoints.size());
                return;
            }

            // 卡格检测：如果长时间在原地或接近目标点但未到达，强制换点
            float px = game.data.player.data.coord.data.x.UpdateData().data;
            float pz = game.data.player.data.coord.data.z.UpdateData().data;

            // 检查是否接近目标点
            bool nearTarget = std::fabs(px - out_gx) < 1.0f && std::fabs(pz - out_gz) < 1.0f;
            // 检查是否几乎没移动
            bool hardlyMove = std::fabs(px - lastPx) < 0.05f && std::fabs(pz - lastPz) < 0.05f;
            // 更新卡住计数器
            stall = (nearTarget || hardlyMove) ? (stall + 1) : 0;

            // 计算卡住阈值（基于延迟时间）
            const uint32_t stallLimit = std::max<uint32_t>(1500u / std::max<uint32_t>(1u, delay), 10u);
            // 如果卡住时间超过阈值，强制换点
            if (stall >= stallLimit)
            {
                out_gx = px;
                out_gz = pz;
                GetNextPoint(out_gx, out_gz, visitedPoints);
                stall = 0;
                LOGF("[扫格 OUT force-next] from=(%.1f,%.1f) -> (%.1f,%.1f)", px, pz, out_gx, out_gz);
            }
            // 更新上一次位置
            lastPx = px;
            lastPz = pz;
        };

        // --- 宝箱黑名单上下文（保持原行为） ---

        // 宝箱相关变量
        std::chrono::steady_clock::time_point chestStartTime; // 开始接近宝箱的时间
        bool isChest = false, chestTimerStarted = false;      // 宝箱状态标志
        float chestX = 0, chestY = 0, chestZ = 0;             // 宝箱坐标

        // MaybeAddChestBlacklist: 检查是否需要将宝箱加入黑名单
        auto MaybeAddChestBlacklist = [&](float px, float py, float pz, float cx, float cy, float cz) -> bool
        {
            // 计算与宝箱的距离
            float currentDist = CalculateDistance(px, py, pz, cx, cy, cz);
            // 如果距离足够近（6单位内）
            if (currentDist < 6.0f)
            {
                // 计算已经卡在宝箱附近的时间
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - chestStartTime).count();
                // 如果超过15秒，加入黑名单
                if (elapsed >= 15)
                {
                    LOGF("加入黑名单");
                    // 加锁保护黑名单列表
                    std::lock_guard<std::mutex> lock(Module::blacklistMutex);
                    // 如果黑名单已满，移除最旧的条目
                    if (Module::blacklistedChests.size() >= 1000)
                        Module::blacklistedChests.pop_front();
                    // 添加新条目
                    Module::blacklistedChests.emplace_back(cx, cy, cz);
                    return true; // 返回true表示已加入黑名单
                }
            }
            else
            {
                // 如果距离变远，重置计时器
                chestStartTime = std::chrono::steady_clock::now();
            }
            return false; // 返回false表示未加入黑名单
        };

        // --- 主循环 ---

        // 只要FollowTarget功能处于开启状态，就持续循环
        while (Module::functionRunMap[{pid, "FollowTarget"}].load())
        {
            // 更新地址信息
            UpdateAddress();

            // A) 清理区外扫格 + 同步找人/怪（直到找到为止）
            std::unique_ptr<Game::World::Player> player = nullptr;
            std::unique_ptr<Game::World::Entity> target = nullptr;

            // 循环直到找到目标或功能被关闭
            for (;;)
            {
                // 等待一段时间
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                // 更新地址信息
                UpdateAddress();

                // 执行扫图逻辑（如果开启扫图模式）
                StepOutsideScan();

                // 查找玩家和实体目标
                player = FindPlayer(game, players);
                target = FindTarget(game, targetBoss, false, false, targets, noTargets, Module::realisticScanRange);

                // 如果找到目标，跳出循环
                if (player || target)
                    break;
                // 如果功能被关闭，跳出循环
                if (!Module::functionRunMap[{pid, "FollowTarget"}].load())
                    break;
            }
            // 再次检查功能是否被关闭
            if (!Module::functionRunMap[{pid, "FollowTarget"}].load())
                break;
            // 如果没有找到任何目标，继续循环
            if (!player && !target)
                continue;

            // B) 若命中"开关"，仅做"计数+日志"，不进入任何清理区逻辑
            // 此处省略了开关相关的代码...

            // C) 外圈追击（保持原有所有行为）
            {
                // 初始化检查计数器
                uint32_t checkCounter = 0;
                // 重置宝箱计时器
                chestTimerStarted = false;

                // 内部循环：持续跟踪当前目标
                while (Module::functionRunMap[{pid, "FollowTarget"}].load())
                {
                    // 等待一段时间
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));

                    // 如果是在跟踪玩家，检查玩家是否仍然有效
                    if (player && !(player = FindPlayer(game, players)))
                        break;

                    // 计算目标坐标
                    float targetX = player ? player->data.x.UpdateAddress().UpdateData().data
                                           : target->data.x.UpdateData().data;
                    float targetY = player ? (player->data.y.UpdateAddress().UpdateData().data - 0.5f)
                                           : (target->data.y.UpdateData().data - 0.5f);
                    float targetZ = player ? player->data.z.UpdateAddress().UpdateData().data
                                           : target->data.z.UpdateData().data;

                    // 如果开启穿墙模式，调整Y坐标
                    if (Module::noClipOn.load())
                        targetY += Module::noClipZOffset;

                    // 处理实体目标
                    if (target)
                    {
                        // 检查是否是宝箱
                        isChest = target->data.name.UpdateData(128).data.find("gameplay/chest_quest_rune_vault_01") != std::string::npos;
                        if (isChest)
                        {
                            // 保存宝箱坐标
                            chestX = targetX;
                            chestY = targetY;
                            chestZ = targetZ;
                            // 启动宝箱计时器
                            if (!chestTimerStarted)
                            {
                                chestStartTime = std::chrono::steady_clock::now();
                                chestTimerStarted = true;
                            }
                        }
                        else
                        {
                            // 如果不是宝箱，关闭计时器
                            chestTimerStarted = false;
                        }

                        // 检查目标是否已死亡
                        if (!target->data.isDeath.UpdateData().data)
                            break;

                        // 定期检查目标状态
                        if ((checkCounter % 100) == 0 ||
                            target->data.health.UpdateData().data < 1 ||
                            (targetX < 1 && targetY < 1 && targetZ < 1))
                        {
                            // 获取实体列表
                            const auto &entitys2 = game.data.world.UpdateAddress().UpdateData().data.entitys;
                            // 检查目标是否仍在列表中
                            if (std::find(entitys2.begin(), entitys2.end(), *target) == entitys2.end())
                                break;
                        }
                    }
                    else
                    {
                        // 如果不是实体目标，关闭宝箱计时器
                        chestTimerStarted = false;
                    }

                    // 更新地址并移动
                    UpdateAddress();
                    MoveEvent(targetX, targetY, targetZ);

                    // 处理宝箱黑名单逻辑
                    if (isChest && chestTimerStarted)
                    {
                        // 获取玩家当前位置
                        float px = game.data.player.data.coord.data.x.UpdateData().data;
                        float py = game.data.player.data.coord.data.y.UpdateData().data;
                        float pz = game.data.player.data.coord.data.z.UpdateData().data;
                        // 检查是否需要将宝箱加入黑名单
                        if (MaybeAddChestBlacklist(px, py, pz, chestX, chestY, chestZ))
                        {
                            // 如果加入了黑名单，重置计时器并跳出循环
                            chestTimerStarted = false;
                            break;
                        }
                    }

                    // 增加检查计数器
                    ++checkCounter;
                }
            }

            // 等待一段时间再开始下一轮循环
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
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

        static const std::vector<std::string> priorityChests = {
            "gameplay/chest_quest_rune_vault_01",
            "gameplay/chest_quest_standard_large",
            "gameplay/chest_quest_standard_small"};

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

            bool isPriorityChest = false;
            for (const auto &chest : priorityChests)
            {
                if (name.find(chest) != std::string::npos)
                {
                    isPriorityChest = true;
                    break;
                }
            }

            if (isPriorityChest)
            {
                float ex = entity.data.x.UpdateData().data;
                float ez = entity.data.z.UpdateData().data;
                bool blacklisted = false;
                {
                    std::lock_guard<std::mutex> lock(Module::blacklistMutex);
                    for (const auto &b : Module::blacklistedChests)
                    {
                        float bx = std::get<0>(b), bz = std::get<2>(b);
                        float dx = ex - bx, dz = ez - bz;
                        if (dx * dx + dz * dz < 64.0f)
                        {
                            blacklisted = true;
                            break;
                        } // XZ 8m
                    }
                }
                if (!blacklisted)
                {
                    LOGF("Found priority chest: %s (distance: %.2f)", name.c_str(), dist);
                    return std::make_unique<Game::World::Entity>(entity);
                }
            }
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

            // rune_vault 黑名单（XZ 8m）
            if (name.find("gameplay/chest_quest_rune_vault_01") != std::string::npos)
            {
                float ex = entity.data.x.UpdateData().data;
                float ez = entity.data.z.UpdateData().data;
                bool blacklisted = false;
                {
                    std::lock_guard<std::mutex> lock(Module::blacklistMutex);
                    for (const auto &b : Module::blacklistedChests)
                    {
                        float bx = std::get<0>(b), bz = std::get<2>(b);
                        float dx = ex - bx, dz = ez - bz;
                        if (dx * dx + dz * dz < 64.0f)
                        {
                            blacklisted = true;
                            break;
                        } // XZ 8m
                    }
                }
                if (blacklisted)
                    continue;
            }

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
        const float sqrt3 = std::sqrt(3.0f);
        const float e = std::max(1.0f, static_cast<float>(entityScand)); // 网格步距
        const int R = static_cast<int>(std::max(1.0f, static_cast<float>(mapWidth) / (2.0f * e)));

        auto clampIndex = [&](int &i, int &j)
        {
            i = std::clamp(i, -R, R);
            j = std::clamp(j, -R, R);
            while (i * i + i * j + j * j > R * R)
            {
                if (std::abs(i) > std::abs(j))
                    i += (i > 0 ? -1 : +1);
                else
                    j += (j > 0 ? -1 : +1);
            }
        };
        auto posToIJ = [&](float X, float Z, int &i, int &j)
        {
            j = static_cast<int>(std::round(Z / (e * sqrt3)));
            i = static_cast<int>(std::round((X - e * j) / (2 * e)));
            clampIndex(i, j);
        };
        auto ijToPos = [&](int i, int j, float &X, float &Z)
        {
            X = e * (2 * i + j);
            Z = e * sqrt3 * j;
        };
        auto keyIJ = [&](int i, int j)
        {
            return std::to_string(i) + "|" + std::to_string(j);
        };

        int ci, cj;
        posToIJ(x, z, ci, cj);

        static const int di[6] = {+1, -1, 0, 0, +1, -1};
        static const int dj[6] = {0, 0, +1, -1, -1, +1};

        // 从当前格 BFS，但**不把当前格作为第一返回点**（避免第一步距离≈0导致原地打转）
        std::queue<std::pair<int, int>> q;
        std::unordered_set<std::string> seen;
        q.push({ci, cj});
        seen.insert(keyIJ(ci, cj));

        while (!q.empty())
        {
            auto [ti, tj] = q.front();
            q.pop();
            for (int k = 0; k < 6; ++k)
            {
                int ni = ti + di[k], nj = tj + dj[k];
                clampIndex(ni, nj);
                std::string K = keyIJ(ni, nj);
                if (seen.count(K))
                    continue;
                seen.insert(K);

                if (!visitedPoints.count(K))
                {
                    visitedPoints.insert(K);
                    ijToPos(ni, nj, x, z);
#ifdef ENTITY_FIELD_DEBUG
                    LOGF("[扫格 OUT next] i=%d j=%d -> (%.1f, %.1f) visited=%zu", ni, nj, x, z, visitedPoints.size());
#endif
                    return;
                }
                q.push({ni, nj});
            }
        }

        // 已全覆盖：重置并强制往内侧挪一步，保证目标 != 当前
        visitedPoints.clear();
        int ni = std::clamp(ci - 1, -R, R), nj = cj;
        clampIndex(ni, nj);
        visitedPoints.insert(keyIJ(ni, nj));
        ijToPos(ni, nj, x, z);
#ifdef ENTITY_FIELD_DEBUG
        LOGF("[扫格 OUT reset] i=%d j=%d -> (%.1f, %.1f) (reset)", ni, nj, x, z);
#endif
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
    // NEW: “开关ID”列表，形如：UpdateConfig("Module::SwitchIds","开关1|开关2|开关3")
    else if (_key == "Module::SwitchIds")
    {
        auto *ids = reinterpret_cast<std::unordered_set<std::string> *>(Module::configMap[_key]);
        ids->clear();
        for (auto &s : _value)
        {
            auto t = trim(s);
            if (!t.empty())
                ids->insert(t);
        }
    }
    // NEW: 真实扫描半径（米）：UpdateConfig("Module::realisticScanRange","90")
    else if (_value.size() >= 1 && _key == "Module::realisticScanRange")
    {
        Module::realisticScanRange = std::stoul(_value[0]);
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