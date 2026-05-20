#include<iostream>
#include<fstream>
#include<sstream>
#include<vector>
#include<algorithm>
#include<iomanip>
#include<filesystem>
#include<sys/mman.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<cstring>
#include<functional>

#define DEBUG_INPUT 0

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

struct hashmap
{
    std::array<std::string, UINT16_MAX + 1> m_keys;
    std::array<Record, UINT16_MAX + 1> m_values;
    std::vector<size_t> filledSlots;

    // Look for the slot in which key is stored or will be stored
    // implemented this way because after a key insertion
    // the slot for inserting the next key may change if collision happens
    uint16_t lookupSlot(std::string_view key)
    {
        uint16_t index = std::hash<std::string_view>{}(key);

        while (!m_keys[index].empty())
        {
            if (m_keys[index] == key)
                return index;
            index++;
        }

        return index;
    }

    void insert(std::string_view key, int16_t value)
    {
        uint16_t slot = lookupSlot(key);

        // if slot empty
        // key does not exist
        if (m_keys[slot].empty())
        {
            m_keys[slot] = key;
            m_values[slot] = Record{1, value, value, value};
            filledSlots.push_back(slot);
#if DEBUG_INPUT
            std::cout<<"insert new record "<<value<<" "<<" city: "<<key<<std::endl;
#endif
        }
        else
        {
            //Key already exist
            Record& record = m_values[slot];
            record.count++;
            record.sum += value;
            if (record.maxValue < value)
            {
                record.maxValue = value;
            }
            else if (record.minValue > value)
            {
                record.minValue = value;
            }
#if DEBUG_INPUT
            std::cout<<"update curValue: "<<value<<" c: "<<record.count<<" s: "<<record.sum<<" max: "<<record.maxValue<<" min: "<<record.minValue<<std::endl;
#endif
        }
    }

    void sort_slots()
    {
        auto cmp = [this](size_t left, size_t right) {
            return m_keys[left] < m_keys[right];
        };

        sort(filledSlots.begin(), filledSlots.end(), cmp);
    }
};

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

        cityRecord.insert(city, value);
    }

    return cityRecord;
}

void processOutput(std::ostream& outputStream, hashmap& cityRecords)
{
    cityRecords.sort_slots();
    std::string delim = "";

    outputStream << std::fixed
                 << std::showpoint
                 << std::setprecision(1);

    outputStream << '{';

    for (size_t& slot: cityRecords.filledSlots)
    {
        const Record& metrics = cityRecords.m_values[slot];
        outputStream << std::exchange(delim, ", ") << cityRecords.m_keys[slot] << "="
                    << metrics.minValue / 10.0 << "/" << (static_cast<double>(metrics.sum) / 10.0 )/ metrics.count <<"/"<< metrics.maxValue / 10.0;
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
