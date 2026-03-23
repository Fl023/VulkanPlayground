#pragma once

#include <cstdint>
#include <functional>

class UUID {
public:
    // Generates a random 128-bit UUID
    UUID();
    
    // For deserializing an existing UUID
    UUID(uint64_t high, uint64_t low);
    
    UUID(const UUID&) = default;

    // We need equality operators so std::unordered_map can handle hash collisions
    bool operator==(const UUID& other) const {
        return m_High == other.m_High && m_Low == other.m_Low;
    }

    bool operator!=(const UUID& other) const {
        return !(*this == other);
    }

    uint64_t GetHigh() const { return m_High; }
    uint64_t GetLow() const { return m_Low; }

private:
    uint64_t m_High;
    uint64_t m_Low;
};

// Custom hash function injected into the std namespace
namespace std {
    template <typename T> struct hash;

    template<>
    struct hash<UUID> {
        std::size_t operator()(const UUID& uuid) const {
            // Combine the hashes of the two 64-bit halves.
            // The bit-shift (<< 1) prevents symmetric values (like high=5, low=5) 
            // from canceling each other out during the XOR (^).
            return std::hash<uint64_t>()(uuid.GetHigh()) ^ 
                  (std::hash<uint64_t>()(uuid.GetLow()) << 1);
        }
    };
}