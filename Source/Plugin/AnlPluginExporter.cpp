
#include "AnlPluginExporter.h"

ANALYSE_FILE_BEGIN

void Plugin::Exporter::toCsv(juce::FileOutputStream& stream, std::vector<Result> const& results)
{
    auto state = false;
    auto addLine = [&]()
    {
        state = false;
        stream << '\n';
    };
    
    auto addColumn = [&](juce::StringRef const& text)
    {
        if(state)
        {
            stream << ',';
        }
        stream << text;
        state = true;
        return true;
    };
    
    auto toSeconds = [](Vamp::RealTime const& timestamp)
    {
        return static_cast<double>(timestamp.sec) + static_cast<double>(timestamp.msec()) / 1000.0;
    };
    
    addColumn("ROW");
    addColumn("TIME");
    addColumn("DURATION");
    addColumn("LABEL");
    for(size_t i = 0; i < results.front().values.size(); ++i)
    {
        addColumn("BIN " + juce::String(i));
    }
    addLine();
    for(size_t i = 0; i < results.size(); ++i)
    {
        auto const& result = results[i];
        addColumn(juce::String(i));
        addColumn(result.hasTimestamp ? juce::String(toSeconds(result.timestamp)) : "0");
        addColumn(result.hasDuration ? juce::String(toSeconds(result.duration)) : "0");
        addColumn(result.label.empty() ? "\"\"" : juce::String(result.label));
        for(auto const& value : result.values)
        {
            addColumn(juce::String(value));
        }
        addLine();
    }
}

void Plugin::Exporter::toXml(juce::FileOutputStream& stream, std::vector<Result> const& results)
{
    auto toXml = [&](size_t row, Result const& result)
    {
        auto child = std::make_unique<juce::XmlElement>("result");
        if(child != nullptr)
        {
            auto toSeconds = [](Vamp::RealTime const& timestamp)
            {
                return static_cast<double>(timestamp.sec) + static_cast<double>(timestamp.msec()) / 1000.0;
            };
            
            XmlParser::toXml(*child, "row", row);
            XmlParser::toXml(*child, "time", result.hasTimestamp ? toSeconds(result.timestamp) : 0.0);
            XmlParser::toXml(*child, "duration", result.hasDuration ? toSeconds(result.duration) : 0.0);
            XmlParser::toXml(*child, "label", result.label);
            XmlParser::toXml(*child, "values", result.values);
        }
        return child;
    };
    
    auto element = std::make_unique<juce::XmlElement>("brioche");
    if(element != nullptr)
    {
        for(size_t i = 0; i < results.size(); ++i)
        {
            element->addChildElement(toXml(i, results[i]).release());
        }
    }
    element->writeTo(stream);
}

ANALYSE_FILE_END
