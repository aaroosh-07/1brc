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
    uint64_t count = 0;
    double sum = 0.0;
    float maxValue = 0.0;
    float minValue = 0.0;
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

float parse_value(const char*& curPointer, const char* bufferEnd)
{
    const char* valueStartPtr = curPointer;

    curPointer = static_cast<const char*>(memchr(curPointer, '\n', bufferEnd - curPointer));

    float value;
    std::from_chars(valueStartPtr, curPointer, value);
    return value;
}

hashmap processInput(const char* bufferStart, std::size_t size)
{
    hashmap cityRecord;

    const char* bufferEnd = bufferStart + size;
    const char* curPointer = bufferStart;

    while (curPointer < bufferEnd)
    {
        // const char* cityStartPtr = curPointer;

        // curPointer = static_cast<const char*>(memchr(curPointer, ';', bufferEnd - curPointer));

        // std::string_view city(cityStartPtr, curPointer - cityStartPtr);

        std::string_view city = parse_city(curPointer, bufferEnd);

        // skip ; character
        curPointer++;

        // const char* valueStartPtr = curPointer;

        // curPointer = static_cast<const char*>(memchr(curPointer, '\n', bufferEnd - curPointer));

        // float value;
        // std::from_chars(valueStartPtr, curPointer, value);
        float value = parse_value(curPointer, bufferEnd);
        curPointer++;

        auto itr = cityRecord.find(city);
        if (itr == cityRecord.end())
        {
            cityRecord.emplace(city, Record{1, value, value, value});
        }
        else
        {
            Record& record = itr->second;
            record.count++;
            record.sum += value;
            record.maxValue = std::max(record.maxValue, value);
            record.minValue = std::min(record.minValue, value);
        }
    }

    return cityRecord;
}

hashmap processInput(std::ifstream& inputFile)
{
    hashmap cityRecord;
    std::string city, value;
    while(std::getline(inputFile, city, ';') && std::getline(inputFile, value, '\n'))
    {
        float data = std::stof(value);
        auto itr = cityRecord.find(city);
        if (itr == cityRecord.end())
        {
            cityRecord.emplace(city, Record{1, data, data, data});
            continue;
        }

        Record& record = itr->second;
        record.sum += data;
        record.maxValue = std::max(record.maxValue, data);
        record.minValue = std::min(record.minValue, data);
        record.count++;
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
        outputStream << std::exchange(delim, ", ") << city << "="
                    << metrics.minValue << "/" << metrics.sum / metrics.count <<"/"<< metrics.maxValue;
    }
    outputStream << "}\n";
}

void bruteForceSol()
{
    std::ifstream file("data/measurements.txt");
    if (!file.is_open())
        std::exit(1);

    auto cityRecords = processInput(file);
    processOutput(std::cout, cityRecords);
}

void improvedSolWithMmapFiles()
{
    MemMapFile memMappedFile(std::filesystem::path("data/measurements.txt"));
    auto cityRecords = processInput(memMappedFile.getBuffer(), memMappedFile.getSize());
    processOutput(std::cout, cityRecords);
}

int main()
{
    // Call this fn for BruteForce sol
    // bruteForceSol();

    // Improved Implementation with Mem Mapped files only
    improvedSolWithMmapFiles();

    return 0;
}
