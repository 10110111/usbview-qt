#pragma once

inline bool startsWith(std::string_view line, std::string_view beginning)
{
    if(line.size() < beginning.size()) return false;
    return std::string_view(line.data(), beginning.size()) == beginning;
}

