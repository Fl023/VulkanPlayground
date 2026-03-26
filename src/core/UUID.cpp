#include "pch.hpp"
#include "UUID.hpp"

#include <random>

static std::random_device s_RandomDevice;
static std::mt19937_64 s_Engine(s_RandomDevice());
static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

UUID::UUID()
    : m_High(s_UniformDistribution(s_Engine)),
      m_Low(s_UniformDistribution(s_Engine))
{
}

UUID::UUID(uint64_t high, uint64_t low)
    : m_High(high), m_Low(low)
{
}

UUID::UUID(const std::string& hexString) {
    if (hexString.length() != 32) {
        m_High = 0; m_Low = 0; return; // Handle invalid strings safely
    }
    m_High = std::stoull(hexString.substr(0, 16), nullptr, 16);
    m_Low = std::stoull(hexString.substr(16, 16), nullptr, 16);
}

std::string UUID::ToString() const {
    std::stringstream ss;
    ss << std::hex << std::setfill('0')
        << std::setw(16) << m_High
        << std::setw(16) << m_Low;
    return ss.str();
}