#pragma once

#include "../AnlTrackDirector.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    namespace Result
    {
        namespace Modifier
        {
            juce::Range<double> getTimeRange(ChannelData const& data);
            std::optional<double> getTime(Accessor const& accessor, size_t const channel, size_t index);

            bool isEmpty(ChannelData const& data);
            bool matchFrame(Accessor const& accessor, size_t const channel, double const time);
            bool canBreak(Accessor const& accessor, size_t const channel, double const time);
            bool containFrames(Accessor const& accessor, size_t const channel, juce::Range<double> const& range);
            ChannelData createFrame(Accessor const& accessor, size_t const channel, double const time);
            ChannelData copyFrames(Accessor const& accessor, size_t const channel, juce::Range<double> const& range);
            ChannelData duplicateFrames(ChannelData const& data, double const destinationTime);
            bool eraseFrames(Accessor& accessor, size_t const channel, juce::Range<double> const& range, juce::String const& commit);
            bool insertFrames(Accessor& accessor, size_t const channel, ChannelData const& data, juce::String const& commit);

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
                ActionErase(std::function<Accessor&()> fn, size_t const channel, juce::Range<double> const& selection);
                ~ActionErase() override = default;

                // juce::UndoableAction
                bool perform() override;
                bool undo() override;

            private:
                ChannelData const mSavedData;
            };

            class ActionPaste
            : public ActionBase
            {
            public:
                ActionPaste(std::function<Accessor&()> fn, size_t const channel, ChannelData const& data, double destination);
                ~ActionPaste() override = default;

                // juce::UndoableAction
                bool perform() override;
                bool undo() override;

            private:
                ChannelData const mSavedData;
                ChannelData const mChannelData;
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
