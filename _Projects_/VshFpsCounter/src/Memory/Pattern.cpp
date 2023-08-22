#include "Pattern.hpp"
#include <sys/memory.h>

uint32_t ReadHighLow(uint32_t address, uint32_t highAdditive, uint32_t lowAdditive)
{
    uint32_t returnAddr = (((uint16_t)(*(uint32_t*)(address + highAdditive)) << 16) | (uint16_t)(*(uint32_t*)(address + lowAdditive)));
    uint32_t returnFinal = (returnAddr & 0x8000) ? returnAddr - 0x10000 : returnAddr;
    return returnFinal;
}

uint32_t ResolveBranch(uint32_t branchAddress)
{
    uint32_t instruction = *(uint32_t*)(branchAddress);
    int32_t offset = instruction & 0x3FFFFFC;

    if (offset & (1 << 25))
        offset |= ~0x03FFFFFF;

    return branchAddress + offset;
}

bool DataCompare(const uint8_t* pbData, const uint8_t* pbMask, const char* szMask)
{
    for (; *szMask; ++szMask, ++pbData, ++pbMask)
    {
        if (*szMask == 'x' && *pbData != *pbMask)
        {
            return false;
        }
    }
    return (*szMask == NULL);
}

uintptr_t FindPatternVsh(uintptr_t address, uint32_t length, uint8_t* bytes, const char* mask)
{
    for (uint32_t i = 0; i < length; i++)
        if (DataCompare((uint8_t*)(address + i), bytes, mask))
            return (uintptr_t)(address + i);

    return 0;
}

uintptr_t FindPatternVshInTextSegment(uint8_t* bytes, const char* mask)
{
    uintptr_t address = FindPatternVsh(gBaseAddress, gInfo_SizeOfImage, bytes, mask);
    if (address == 0)
        return 0;

    return address;
}

uint32_t FindPatternHypervisor(uint32_t startAddress, uint32_t stopAddress, uint8_t step, const char* sfind, size_t len, const char* mask)
{
    const uint32_t chunk_size = 65536; // 64KB
    uint32_t found_offset = 0;

    sys_addr_t sysmem = NULL;
    if (sys_memory_allocate(chunk_size, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) != SUCCEEDED)
        return found_offset;

    char* mem = (char*)sysmem;
    const uint32_t m = chunk_size - len, gap = (len + 0x10) - (len % 0x10);
    for (; startAddress < stopAddress; startAddress += chunk_size - gap)
    {
        bool retval = PeekChunkLV1((startAddress | 0x8000000000000000ULL), (uint64_t*)mem, chunk_size);
        if (!retval)
            break;

        for (uint32_t offset = 0; offset < m; offset += step)
        {
            if (DataCompare((uint8_t*)(mem + offset), (uint8_t*)sfind, mask))
            {
                found_offset = (startAddress + offset);
                startAddress = stopAddress;
                break;
            }
        }
    }

    sys_memory_free(sysmem);
    return found_offset;
}

uint32_t FindPatternHypervisor(const char* bytes, size_t len, const char* mask)
{
    uint32_t address = FindPatternHypervisor(0x00000000, 0x0FFFFFC, 4, bytes, len, mask);
    if (address == 0)
        return 0;

    return address;
}

bool FindPatternHypervisorWithResult(uint32_t startAddress, uint32_t m, uint32_t chunk_size, char* mem, const char* sfind, const char* mask, uint32_t* found_offset)
{
    for (uint32_t offset = 0; offset < m; offset += 4)
    {
        if (DataCompare((uint8_t*)(mem + offset), (uint8_t*)sfind, mask))
        {
            uint32_t found = (startAddress + offset);
            *found_offset = found;
            return true;
        }
    }

    *found_offset = 0;
    return false;
}

void FindPatternsHypervisorInParallel(std::vector<Pattern>& patterns, std::vector<uint32_t>& foundOffsets)
{
    const uint32_t chunk_size = 65536; // 64KB

    sys_addr_t sysmem = NULL;
    if (sys_memory_allocate(chunk_size, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) != SUCCEEDED)
        return;

    char* mem = (char*)sysmem;
    size_t patternApproximateLen = 20;
    const uint32_t m = chunk_size - patternApproximateLen, gap = (patternApproximateLen + 0x10) - (patternApproximateLen % 0x10);
    uint32_t startAddress = 0x00000000;
    uint32_t stopAddress = 0x0FFFFFC;
    for (; startAddress < stopAddress; startAddress += chunk_size - gap)
    {
        bool retval = PeekChunkLV1((startAddress | 0x8000000000000000ULL), (uint64_t*)mem, chunk_size);
        if (!retval)
            break;

        for (auto& pat : patterns)
        {
            if (pat.found)
                continue;

            uint32_t offset = 0;
            if (FindPatternHypervisorWithResult(startAddress, m, chunk_size, mem, pat.sfind, pat.mask, &offset))
            {
                foundOffsets.push_back(offset);
                pat.found = true;
            }

            if (foundOffsets.size() == 0)
                break;
        }

        // Limit 3 patterns for our specific use case
        if (foundOffsets.size() == 3)
            break;
    }

    sys_memory_free(sysmem);
}

uint64_t FindPatternKernel(uint64_t startAddress, uint64_t stopAddress, uint8_t step, const char* sfind, size_t len, const char* mask)
{
    const uint32_t chunk_size = 65536; // 64KB
    uint64_t found_offset = 0;

    sys_addr_t sysmem = NULL;
    if (sys_memory_allocate(chunk_size, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) != SUCCEEDED)
        return found_offset;

    char* mem = (char*)sysmem;
    const uint32_t m = chunk_size - len, gap = (len + 0x10) - (len % 0x10);
    for (; startAddress < stopAddress; startAddress += chunk_size - gap)
    {
        bool retval = PeekChunkLV2(startAddress, (uint64_t*)mem, chunk_size);
        if (!retval)
            break;

        for (uint32_t offset = 0; offset < m; offset += step)
        {
            if (DataCompare((uint8_t*)(mem + offset), (uint8_t*)sfind, mask))
            {
                found_offset = (startAddress + offset);
                startAddress = stopAddress;
                break;
            }
        }
    }

    sys_memory_free(sysmem);
    return found_offset;
}

uint64_t FindPatternKernel(const char* bytes, size_t len, const char* mask)
{
    uint64_t address = FindPatternKernel(0x8000000000000000, 0x8000000000800000, 4, bytes, len, mask);
    if (address == 0)
        return 0;

    return address;
}