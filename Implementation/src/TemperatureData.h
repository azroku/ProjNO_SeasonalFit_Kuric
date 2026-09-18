#pragma once

#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

struct TemperatureRecord
{
    double month{};
    double temperature{};
};

inline std::string trim(std::string value)
{
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return {};

    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

inline std::vector<std::string> splitCsvLine(const std::string& line)
{
    std::vector<std::string> fields;
    std::stringstream stream(line);
    std::string field;

    while (std::getline(stream, field, ','))
        fields.push_back(trim(field));

    return fields;
}

inline bool tryParseDouble(const std::string& text, double& value)
{
    const std::string cleaned = trim(text);

    if (cleaned.empty())
        return false;

    try
    {
        std::size_t parsedCharacters = 0;
        value = std::stod(cleaned, &parsedCharacters);
        return parsedCharacters == cleaned.size() && std::isfinite(value);
    }
    catch (const std::exception&)
    {
        return false;
    }
}

inline std::vector<TemperatureRecord> loadTemperatureCsv(const std::string& fileName)
{
    std::ifstream input(fileName);

    if (!input)
        throw std::runtime_error("Cannot open temperature data file: " + fileName);

    std::string headerLine;
    if (!std::getline(input, headerLine))
        throw std::runtime_error("Temperature CSV file is empty: " + fileName);

    const std::vector<std::string> header = splitCsvLine(headerLine);

    if (header.size() < 4 ||
        header[0] != "month" ||
        header[3] != "temperature_celsius")
    {
        throw std::runtime_error(
            "Unexpected CSV format. Expected header: "
            "month,year,month_of_year,temperature_celsius");
    }

    std::vector<TemperatureRecord> records;
    std::string line;

    while (std::getline(input, line))
    {
        if (trim(line).empty())
            continue;

        const std::vector<std::string> fields = splitCsvLine(line);

        if (fields.size() < 4)
            continue;

        double month = 0.0;
        double temperature = 0.0;

        if (!tryParseDouble(fields[0], month) ||
            !tryParseDouble(fields[3], temperature))
        {
            continue;
        }

        records.push_back({ month, temperature });
    }

    if (records.empty())
        throw std::runtime_error("No valid temperature records were found in: " + fileName);

    if (records.size() < 12)
        throw std::runtime_error("At least 12 valid monthly temperature records are required.");

    return records;
}

inline void recordsToVectors(
    const std::vector<TemperatureRecord>& records,
    std::vector<double>& time,
    std::vector<double>& temperature)
{
    if (records.empty())
        throw std::runtime_error("Cannot create vectors from an empty record list.");

    time.clear();
    temperature.clear();
    time.reserve(records.size());
    temperature.reserve(records.size());

    for (const TemperatureRecord& record : records)
    {
        if (!time.empty() && record.month <= time.back())
        {
            throw std::runtime_error(
                "Temperature records must have strictly increasing month indices.");
        }
        time.push_back(record.month);
        temperature.push_back(record.temperature);
    }
}
