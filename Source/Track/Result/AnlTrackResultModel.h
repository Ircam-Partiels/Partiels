#pragma once

#include "../../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    namespace Result
    {
        class Data;

        class Access
        {
        public:
            // clang-format off
            enum class Mode
            {
                  read
                , write
            };
            // clang-format on

            Access() = delete;
            Access(Data const& data, Mode mode);
            Access(Access const& access);
            Access(Access&& access);
            ~Access();

            inline operator bool() const noexcept
            {
                return mValid;
            }

        private:
            Data const& mData;
            Mode const mMode;
            bool mValid;
        };

        class Data
        {
        public:
            // clang-format off
            enum Type
            {
                  marker = 1<<0
                , point = 1<<1
                , column = 1<<2
            };

            // clang-format on
            // time, duration, value(s)
            using Marker = std::tuple<double, double, std::string>;
            using Point = std::tuple<double, double, std::optional<float>>;
            using Column = std::tuple<double, double, std::vector<float>>;

            using Markers = std::vector<Marker>;
            using Points = std::vector<Point>;
            using Columns = std::vector<Column>;

            Data();
            Data(Data const& rhs);
            Data(std::vector<Markers>&& markers);
            Data(std::vector<Points>&& points);
            Data(std::vector<Columns>&& columns);
            ~Data();

            Data& operator=(Data const& rhs);

            Access getReadAccess() const;
            Access getWriteAccess() const;

            bool isEmpty() const noexcept;
            std::shared_ptr<std::vector<Markers> const> getMarkers() const noexcept;
            std::shared_ptr<std::vector<Points> const> getPoints() const noexcept;
            std::shared_ptr<std::vector<Columns> const> getColumns() const noexcept;

            std::shared_ptr<std::vector<Markers>> getMarkers() noexcept;
            std::shared_ptr<std::vector<Points>> getPoints() noexcept;
            std::shared_ptr<std::vector<Columns>> getColumns() noexcept;

            std::optional<size_t> getNumChannels() const noexcept;
            std::optional<size_t> getNumBins() const noexcept;
            std::optional<Zoom::Range> getValueRange() const noexcept;

            bool operator==(Data const& rhd) const noexcept;
            bool operator!=(Data const& rhd) const noexcept;

            bool matchWithEpsilon(Data const& other, double timeEpsilon, float valueEpsilon) const;

            static std::optional<std::string> getValue(std::shared_ptr<const std::vector<Markers>> results, size_t channel, double time);
            static std::optional<float> getValue(std::shared_ptr<const std::vector<Points>> results, size_t channel, double time);
            static std::optional<float> getValue(std::shared_ptr<const std::vector<Columns>> results, size_t channel, double time, size_t bin);

        private:
            bool getAccess(bool readOnly) const;
            void releaseAccess(bool readOnly) const;

            class Impl;
            std::unique_ptr<Impl> mImpl;
            friend class Access;
        };

        using ChannelData = std::variant<std::vector<Data::Marker>, std::vector<Data::Point>, std::vector<Data::Column>>;

        template <class T>
        concept is_data = std::is_same_v<T, Data::Marker> || std::is_same_v<T, Data::Point> || std::is_same_v<T, Data::Column>;

        template <typename T>
            requires is_data<T>
        static bool lower_cmp(T const& value, double const time)
        {
            return std::get<0_z>(value) < time;
        }

        template <typename T>
            requires is_data<T>
        static bool upper_cmp(double const time, T const& value)
        {
            return time < std::get<0_z>(value);
        }

        struct File
        {
            juce::File file;
            juce::StringPairArray args; // saved values (mainly for parsing SDIF)
            nlohmann::json extra;       // parsed values (mainly for restoring track attributes from JSON)
            juce::String commit;        // unique identifier that is used for versioning

            File(juce::File const& f = {}, juce::StringPairArray const& a = {}, nlohmann::json const& e = {}, juce::String const& c = {});

            bool operator==(File const& rhd) const noexcept;
            bool operator!=(File const& rhd) const noexcept;
            bool isEmpty() const noexcept;
        };

        void to_json(nlohmann::json& j, File const& file);
        void from_json(nlohmann::json const& j, File& file);
    } // namespace Result
} // namespace Track

namespace XmlParser
{
    template <>
    void toXml<Track::Result::File>(juce::XmlElement& xml, juce::Identifier const& attributeName, Track::Result::File const& value);

    template <>
    auto fromXml<Track::Result::File>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Track::Result::File const& defaultValue)
        -> Track::Result::File;
} // namespace XmlParser

ANALYSE_FILE_END
