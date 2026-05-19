#include<iostream>
#include<fstream>
#include<sstream>
#include<unordered_map>
#include<vector>
#include<algorithm>
#include<iomanip>
#include<filesystem>
#include<sys/mman.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<charconv>
#include<cstring>

#define DEBUG_INPUT 0
#define DEBUG_OUTPUT 0

class FileDes
{
    int fileFd = -1;
public:
    explicit FileDes(std::filesystem::path& filePath)
    {
        fileFd = open(filePath.c_str(), O_RDONLY);

        if (fileFd < 0)
            throw std::system_error(errno, std::system_category(), "Unable to open File");
    }

    ~ FileDes()
    {
        if (fileFd >= 0)
            close(fileFd);
    }

    const int getFileFd()
    {
        return fileFd;
    }
};

class MemMapFile
{
    FileDes fileDes;
    char* buffer = nullptr;
    std::size_t size = 0;

public:
    explicit MemMapFile(std::filesystem::path filePath) : fileDes(filePath)
    {
        struct stat fs;
        if (fstat(fileDes.getFileFd(), &fs) == 1)
            throw std::system_error(errno, std::system_category(), "Unable to read file stats");

        size = fs.st_size;
        buffer = static_cast<char*>(mmap(NULL, size, PROT_READ, MAP_PRIVATE, fileDes.getFileFd(), 0));

        if (buffer == MAP_FAILED)
        {
            throw std::system_error(errno, std::system_category(), "Unable to map file");
        }
    }

    ~ MemMapFile()
    {
        if (buffer != nullptr)
        {
            munmap(buffer, size);
        }
    }

    const char* getBuffer()
    {
        return buffer;
    }

    std::size_t getSize()
    {
        return size;
    }
};

struct Record
{
    int64_t count = 0;
    int64_t sum = 0;
    int16_t maxValue = 0;
    int16_t minValue = 0;
};

struct StringHash
{
    using is_transparent = void;

    size_t operator()(std::string_view txt) const
    {
        return std::hash<std::string_view>{}(txt);
    }

    size_t operator()(const std::string& txt) const
    {
        return std::hash<std::string>{}(txt);
    }
};

using hashmap = std::unordered_map<std::string, Record, StringHash, std::equal_to<>>;

std::string_view parse_city(const char*& curPointer, const char* bufferEnd)
{
    const char* cityStartPtr = curPointer;

    curPointer = static_cast<const char*>(memchr(curPointer, ';', bufferEnd - curPointer));

    return std::string_view(cityStartPtr, curPointer - cityStartPtr);
}

int16_t parse_value(const char*& curPointer)
{
    bool negative = (*curPointer == '-');
    if (negative)
        curPointer++;

    int16_t value = 0;
    while (*curPointer != '\n')
    {
        if (*curPointer != '.')
        {
            value *= 10;
            value += *curPointer - '0';
        }
        curPointer++;
    }

    if (negative)
        value *= -1;

    return value;
}

hashmap processInput(const char* bufferStart, std::size_t size)
{
    hashmap cityRecord;

    const char* bufferEnd = bufferStart + size;
    const char* curPointer = bufferStart;

    while (curPointer < bufferEnd)
    {
        std::string_view city = parse_city(curPointer, bufferEnd);

        // skip ; character
        curPointer++;

        int16_t value = parse_value(curPointer);
        curPointer++;

        auto itr = cityRecord.find(city);
        if (itr == cityRecord.end())
        {
#if DEBUG_INPUT
            std::cout<<"insert new record "<<value<<" "<<" city: "<<city<<std::endl;
#endif
            cityRecord.emplace(city, Record{1, value, value, value});
        }
        else
        {
            Record& record = itr->second;
            record.count++;
            record.sum += value;
            record.maxValue = std::max(record.maxValue, value);
            record.minValue = std::min(record.minValue, value);
#if DEBUG_INPUT
            std::cout<<"update curValue: "<<value<<" c: "<<record.count<<" s: "<<record.sum<<" max: "<<record.maxValue<<" min: "<<record.minValue<<std::endl;
#endif
        }
    }

    return cityRecord;
}

void processOutput(std::ostream& outputStream, hashmap& cityRecords)
{
    std::vector<std::string> cities;
    cities.reserve(cityRecords.size());

    for (const auto& pair : cityRecords)
    {
        cities.push_back(pair.first);
    }

    std::sort(cities.begin(), cities.end());
    std::string delim = "";

    outputStream << std::fixed
                 << std::showpoint
                 << std::setprecision(1);

    outputStream << '{';
    for (std::string& city: cities)
    {
        Record& metrics = cityRecords[city];
#if DEBUG_OUTPUT
        std::cout<<"rec city: "<<city<<" sum: "<<metrics.sum<<std::endl;
#endif
        outputStream << std::exchange(delim, ", ") << city << "="
                    << metrics.minValue / 10.0 << "/" << (metrics.sum / metrics.count) / 10.0 <<"/"<< metrics.maxValue / 10.0;
    }
    outputStream << "}\n";
}

void improvedSolWithMmapFiles()
{
    MemMapFile memMappedFile(std::filesystem::path("data/measurements.txt"));
    auto cityRecords = processInput(memMappedFile.getBuffer(), memMappedFile.getSize());
    processOutput(std::cout, cityRecords);
}

int main()
{
    // Improved Implementation with Mem Mapped files only
    improvedSolWithMmapFiles();

    return 0;
}
