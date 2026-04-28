#include<iostream>
#include<fstream>
#include<sstream>
#include<unordered_map>
#include<vector>
#include<algorithm>
#include<iomanip>

struct Record
{
    uint64_t count = 0;
    double sum = 0.0;
    float maxValue = 0.0;
    float minValue = 0.0;
};

std::unordered_map<std::string, Record> processInput(std::ifstream& inputFile)
{
    std::unordered_map<std::string, Record> cityRecord;
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

void processOutput(std::ostream& outputStream, std::unordered_map<std::string, Record>& cityRecords)
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

int main()
{
    bruteForceSol();

    return 0;
}