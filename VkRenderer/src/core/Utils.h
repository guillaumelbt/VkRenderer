#pragma once
#include <functional>
#include <vector>
#include <string>

class Utils {
public:
    static std::vector<char> ReadFile(const std::string& _filename);
};

template <typename T, typename... Rest>
void HashCombine(std::size_t& seed, const T& v, const Rest&... rest) 
{
	seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
	(HashCombine(seed, rest), ...);
};