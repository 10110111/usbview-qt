#pragma once

#include <string>
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <string_view>
#include <QString>

#define DEFINE_EXPLICIT_BOOL(Type)          \
struct Type                                 \
{                                           \
    bool on=true;                           \
    explicit Type()=default;                \
    explicit Type(bool on) : on(on) {}      \
    operator bool() const { return on; }    \
}

inline bool startsWith(std::string_view line, std::string_view beginning)
{
    if(line.size() < beginning.size()) return false;
    return std::string_view(line.data(), beginning.size()) == beginning;
}

inline unsigned getUInt(std::filesystem::path const& filePath, const int base)
{
    std::ifstream file(filePath);
    if(!file)
        throw std::invalid_argument("Failed to open file \""+filePath.string()+"\"");
    switch(base)
    {
    case 16: file >> std::hex; break;
    case 10: file >> std::dec; break;
    case 8:  file >> std::oct; break;
    default: assert(!"Invalid base");
    }
    unsigned value;
    file >> value;
    if(!file)
        throw std::invalid_argument("Failed to parse integer from file \""+filePath.string()+"\"");
    if(file.get()!='\n')
        throw std::invalid_argument("Trailing characters after integer in file \""+filePath.string()+"\"");
    return value;
}

inline unsigned getUInt(std::string const& str, const int base)
{
    assert(base==8 || base==10 || base==16);
    std::size_t pos=0;
    const auto value=std::stoul(str, &pos, base);
    if(pos!=str.size())
        throw std::invalid_argument("Trailing characters after integer \""+str+"\"");
    return value;
}

inline double getDouble(std::filesystem::path const& filePath)
{
    std::ifstream file(filePath);
    if(!file)
        throw std::invalid_argument("Failed to open file \""+filePath.string()+"\"");
    double value;
    file >> value;
    if(!file)
        throw std::invalid_argument("Failed to parse floating-point number from file \""+filePath.string()+"\"");
    if(file.get()!='\n')
        throw std::invalid_argument("Trailing characters after floating-point number in file \""+filePath.string()+"\"");
    return value;
}

inline double getDouble(std::string const& str)
{
    std::size_t pos=0;
    const auto value=std::stod(str, &pos);
    if(pos!=str.size())
        throw std::invalid_argument("Trailing characters after floating-point number \""+str+"\"");
    return value;
}

inline QString getString(std::filesystem::path const& filePath)
{
    char data[4097];
    std::ifstream file(filePath);
    if(!file)
        throw std::invalid_argument("Failed to open file \""+filePath.string()+"\"");
    file.read(data, sizeof data);
    if(!file.gcount())
        throw std::invalid_argument("Failed to read file \""+filePath.string()+"\"");
    auto str=QLatin1String(data, file.gcount());
    if(str.size() && str.back()=='\n')
        str.chop(1);
    return str;
}

inline std::vector<uint8_t> getFileData(std::filesystem::path const& filePath)
{
    char data[4097];
    std::ifstream file(filePath);
    if(!file)
        throw std::invalid_argument("Failed to open file \""+filePath.string()+"\"");
    file.read(data, sizeof data);
    if(!file.gcount() && !file.eof())
        throw std::invalid_argument("Failed to read file \""+filePath.string()+"\"");
    return std::vector<uint8_t>(data, data+file.gcount());
}
