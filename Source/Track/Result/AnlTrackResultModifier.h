#pragma once

#include "../AnlTrackDirector.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    namespace Result
    {
        namespace Modifier
        {
            using CopiedData = std::variant<std::map<size_t, Data::Marker>, std::map<size_t, Data::Point>, std::map<size_t, Data::Column>>;

            std::set<size_t> getIndices(CopiedData const& data);
            juce::Range<double> getTimeRange(CopiedData const& data);
            CopiedData duplicateFrames(CopiedData const& data, double const originalTime, size_t destinationIndex, double destinationTime);

            std::optional<double> getTime(Accessor const& accessor, size_t const channel, size_t index);
            std::optional<size_t> getIndex(Accessor const& accessor, size_t const channel, double time);
            std::set<size_t> getIndices(Accessor const& accessor, size_t const channel, juce::Range<double> const& range);
            CopiedData copyFrames(Accessor const& accessor, size_t const channel, std::set<size_t> const& indices);
            bool insertFrames(Accessor& accessor, size_t const channel, CopiedData const& data, juce::String const& commit);
            bool eraseFrames(Accessor& accessor, size_t const channel, std::set<size_t> const& indices, juce::String const& commit);

            template <int D, typename T>
            bool updateFrame(Accessor& accessor, size_t const channel, size_t const index, juce::String const& commit, T const fn)
            {
                auto const applyChange = [&](auto& resultPtr)
                {
                    auto& results = *resultPtr;
                    if(channel >= results.size())
                    {
                        return false;
                    }
                    auto& channelFrames = results[channel];
                    if(index >= channelFrames.size())
                    {
                        return false;
                    }
                    return fn(channelFrames[index]);
                };

                auto const getAccessAndParse = [&](auto& results)
                {
                    auto const access = results.getWriteAccess();
                    if(!static_cast<bool>(access))
                    {
                        auto const options = juce::MessageBoxOptions()
                                                 .withIconType(juce::AlertWindow::WarningIcon)
                                                 .withTitle(juce::translate("Results cannot be accessed!"))
                                                 .withMessage(juce::translate("The results are already used by another process and cannot be modified. Please, wait for a valid access before modifying results."))
                                                 .withButton(juce::translate("Ok"));
                        juce::AlertWindow::showAsync(options, nullptr);
                        return false;
                    }
                    if constexpr(static_cast<bool>(D & Data::Type::marker))
                    {
                        if(auto markers = results.getMarkers())
                        {
                            return applyChange(markers);
                        }
                    }
                    if constexpr(static_cast<bool>(D & Data::Type::point))
                    {
                        if(auto points = results.getPoints())
                        {
                            return applyChange(points);
                        }
                    }
                    if constexpr(static_cast<bool>(D & Data::Type::column))
                    {
                        if(auto columns = results.getColumns())
                        {
                            return applyChange(columns);
                        }
                    }
                    return false;
                };

                auto results(accessor.getAttr<AttrType::results>());
                if(getAccessAndParse(results))
                {
                    auto file = accessor.getAttr<AttrType::file>();
                    file.commit = commit;
                    accessor.setAttr<AttrType::results>(results, NotificationType::synchronous);
                    accessor.setAttr<AttrType::file>(file, NotificationType::synchronous);
                    return true;
                }
                return false;
            }

            class ActionBase
            : public juce::UndoableAction
            {
            public:
                ActionBase(std::function<Accessor&()> fn, size_t const channel);
                ~ActionBase() override = default;

            protected:
                std::function<Accessor&()> mGetAccessorFn;
                size_t const mChannel;
                juce::String const mCurrentCommit;
                juce::String const mNewCommit;
            };

            class ActionErase
            : public ActionBase
            {
            public:
                ActionErase(std::function<Accessor&()> fn, size_t const channel, std::set<size_t> const& indices);
                ActionErase(std::function<Accessor&()> fn, size_t const channel, juce::Range<double> const& selection);
                ~ActionErase() override = default;

                // juce::UndoableAction
                bool perform() override;
                bool undo() override;

            private:
                Modifier::CopiedData const mSavedData;
            };

            class ActionPaste
            : public ActionBase
            {
            public:
                ActionPaste(std::function<Accessor&()> fn, size_t const channel, double origin, CopiedData const& data, double destination);
                ActionPaste(std::function<Accessor&()> fn, size_t const channel, CopiedData const& data, double destination);
                ~ActionPaste() override = default;

                // juce::UndoableAction
                bool perform() override;
                bool undo() override;

            private:
                Modifier::CopiedData const mSavedData;
                std::optional<size_t> const mCopyIndex;
                Modifier::CopiedData const mCopiedData;
            };

            class FocusRestorer
            : public juce::UndoableAction
            {
            public:
                FocusRestorer(std::function<Accessor&()> fn);
                ~FocusRestorer() override = default;

                // juce::UndoableAction
                bool perform() override;
                bool undo() override;

            protected:
                std::function<Accessor&()> mGetAccessorFn;
                FocusInfo const mFocus;
            };
        } // namespace Modifier
    }     // namespace Result
} // namespace Track

ANALYSE_FILE_END
