#pragma once

#include "../Plugin/AnlPluginModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Results
    {
    public:
        using Marker = std::tuple<double, double, std::string>;
        using Point = std::tuple<double, double, std::optional<float>>;
        using Column = std::tuple<double, double, std::vector<float>>;

        using Markers = std::vector<Marker>;
        using Points = std::vector<Point>;
        using Columns = std::vector<Column>;

        using SharedMarkers = std::shared_ptr<const std::vector<Markers>>;
        using SharedPoints = std::shared_ptr<const std::vector<Points>>;
        using SharedColumns = std::shared_ptr<const std::vector<Columns>>;

        Results() = default;

        static Results create(std::variant<SharedMarkers, SharedPoints, SharedColumns> results);

        SharedMarkers const getMarkers() const noexcept;
        SharedPoints const getPoints() const noexcept;
        SharedColumns const getColumns() const noexcept;

        size_t const getNumBins() const noexcept;
        Zoom::Range const& getValueRange() const noexcept;
        bool isEmpty() const noexcept;
        bool operator==(Results const& rhd) const noexcept;
        bool operator!=(Results const& rhd) const noexcept;

        static Markers::const_iterator findFirstAt(Markers const& results, Zoom::Range const& globalRange, double time);
        static Points::const_iterator findFirstAt(Points const& results, Zoom::Range const& globalRange, double time);
        static Columns::const_iterator findFirstAt(Columns const& results, Zoom::Range const& globalRange, double time);

        static std::optional<std::string> getValue(SharedMarkers results, size_t channel, Zoom::Range const& globalRange, double time);
        static std::optional<float> getValue(SharedPoints results, size_t channel, Zoom::Range const& globalRange, double time);
        static std::optional<float> getValue(SharedColumns results, size_t channel, Zoom::Range const& globalRange, double time, size_t bin);

    private:
        explicit Results(std::variant<SharedMarkers, SharedPoints, SharedColumns> results, size_t const numBins, Zoom::Range const& valueRange);

        std::variant<SharedMarkers, SharedPoints, SharedColumns> mResults{SharedPoints(nullptr)};
        size_t mNumBins{0_z};
        Zoom::Range mValueRange{};
    };
} // namespace Track

ANALYSE_FILE_END
