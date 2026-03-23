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