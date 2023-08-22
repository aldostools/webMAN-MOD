#pragma once
#undef vector
#include <vector>
#include <algorithm>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/process.h>
#include "Memory.hpp"

constexpr uintptr_t gBaseAddress = 0x00010200;    // first sub in the eboot in ida
constexpr size_t gInfo_SizeOfImage = 0x008FFFFF;  // open the eboot in ida then go all the way to the end and then added FFFF to the last bytes

struct Pattern
{
    const char* sfind;
    const char* mask;
    bool found;
};

uint32_t ReadHighLow(uint32_t address, uint32_t highAdditive, uint32_t lowAdditive);
uint32_t ResolveBranch(uint32_t branchAddress);
bool DataCompare(const uint8_t* pbData, const uint8_t* pbMask, const char* szMask);
uintptr_t FindPatternVsh(uintptr_t address, uint32_t length, uint8_t* bytes, const char* mask);
uintptr_t FindPatternVshInTextSegment(uint8_t* bytes, const char* mask);


uint32_t FindPatternHypervisor(uint32_t startAddress, uint32_t stopAddress, uint8_t step, const char* sfind, size_t len, const char* mask);
uint32_t FindPatternHypervisor(const char* bytes, size_t len, const char* mask);
bool FindPatternHypervisorWithResult(uint32_t startAddress, uint32_t m, uint32_t chunk_size, char* mem, const char* sfind, const char* mask, uint32_t* found_offset);
void FindPatternsHypervisorInParallel(std::vector<Pattern>& patterns, std::vector<uint32_t>& foundOffsets);


uint64_t FindPatternKernel(uint64_t startAddress, uint64_t stopAddress, uint8_t step, const char* sfind, size_t len, const char* mask);
uint64_t FindPatternKernel(const char* bytes, size_t len, const char* mask);