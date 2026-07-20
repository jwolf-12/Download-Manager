#pragma once

#include <optional>
#include <atomic>
struct Range{
    long long start=0, end=0;
};

struct downloadOptions{
    std::optional<Range> range;
    bool expectPartial=false;
};

struct Chunk {
    Range range;
    bool failed = false;
    int retries = 0;
    long long downloaded=0;
    bool completed=false;
};