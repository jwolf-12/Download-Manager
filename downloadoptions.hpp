#pragma once

#include <optional>
struct Range{
    long long start,end;
};

struct downloadOptions{
    std::optional<Range> range;
};