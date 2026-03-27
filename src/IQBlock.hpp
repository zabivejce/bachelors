#pragma once
#include <vector>
const int CHUNK_SIZE = 1024;
struct IQBlock
{
    std::vector<float> i_data;
    std::vector<float> q_data;
    IQBlock()
    {
        i_data.reserve(CHUNK_SIZE);
        q_data.reserve(CHUNK_SIZE);
    }
};