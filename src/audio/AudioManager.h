#pragma once
#include <string>
#include <unordered_map>
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

namespace nexus::audio {

class AudioManager {
public:
    static AudioManager& get() {
        static AudioManager instance;
        return instance;
    }

    bool init() {
        if (m_initialized) return true;
        ma_result r = ma_engine_init(nullptr, &m_engine);
        if (r != MA_SUCCESS) return false;
        m_initialized = true;
        return true;
    }

    void shutdown() {
        if (!m_initialized) return;
        for (auto& kv : m_sounds) ma_sound_uninit(&kv.second);
        m_sounds.clear();
        ma_engine_uninit(&m_engine);
        m_initialized = false;
    }

    bool loadSound(const std::string& name, const std::string& path) {
        if (!m_initialized) return false;
        auto& snd = m_sounds[name];
        ma_result r = ma_sound_init_from_file(&m_engine, path.c_str(), 0, nullptr, nullptr, &snd);
        if (r != MA_SUCCESS) { m_sounds.erase(name); return false; }
        return true;
    }

    void play(const std::string& name, float volume = 1.0f) {
        auto it = m_sounds.find(name);
        if (it == m_sounds.end()) return;
        ma_sound_set_volume(&it->second, volume);
        ma_sound_start(&it->second);
    }

    void stop(const std::string& name) {
        auto it = m_sounds.find(name);
        if (it == m_sounds.end()) return;
        ma_sound_stop(&it->second);
    }

    void setMasterVolume(float v) {
        if (m_initialized) ma_engine_set_volume(&m_engine, v);
    }

private:
    AudioManager() = default;
    ~AudioManager() { shutdown(); }
    ma_engine m_engine;
    std::unordered_map<std::string, ma_sound> m_sounds;
    bool m_initialized = false;
};

} // namespace nexus::audio
