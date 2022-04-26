#include "AnlTrackResultCell.h"

ANALYSE_FILE_BEGIN

Track::Result::CellBase::CellBase(Director& director, Zoom::Accessor& timeZoomAccessor, size_t channel, size_t index)
: mDirector(director)
, mTimeZoomAccessor(timeZoomAccessor)
, mChannel(channel)
, mIndex(index)
{
}

bool Track::Result::CellBase::CellBase::updateAndValidate(size_t channel)
{
    auto const& results = mAccessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        return false;
    }
    auto isResultValid = [&](auto const& resultPtr) -> bool
    {
        if(mChannel < resultPtr->size() && mIndex < resultPtr->at(mChannel).size() && channel == mChannel)
        {
            update();
            return true;
        }
        return false;
    };
    if(auto const markers = results.getMarkers())
    {
        return isResultValid(markers);
    }
    if(auto points = results.getPoints())
    {
        return isResultValid(points);
    }
    return false;
}

Track::Result::CellTime::CellTime(Director& director, Zoom::Accessor& timeZoomAccessor, size_t channel, size_t index)
: Result::CellBase(director, timeZoomAccessor, channel, index)
{
    addAndMakeVisible(mHMSmsField);
    mHMSmsField.onTimeChanged = [this](double time)
    {
        auto& undoManager = mDirector.getUndoManager();
        class Action
        : public Modifier::ActionBase
        {
        public:
            Action(Accessor& accessor, size_t channel, size_t index, double currentTime, double newTime)
            : Modifier::ActionBase(accessor, channel)
            , mIndex(index)
            , mCurrentTime(currentTime)
            , mNewTime(newTime)
            {
            }

            ~Action() override = default;

            bool perform() override
            {
                return Modifier::updateFrame<Data::Type::marker | Data::Type::point>(mAccessor, mChannel, mIndex, mNewCommit, [&](auto& frame)
                                                                                     {
                                                                                         if(std::abs(std::get<0_z>(frame) - mNewTime) > std::numeric_limits<double>::epsilon())
                                                                                         {
                                                                                             std::get<0_z>(frame) = mNewTime;
                                                                                             return true;
                                                                                         }
                                                                                         return false;
                                                                                     });
            }

            bool undo() override
            {
                return Modifier::updateFrame<Data::Type::marker | Data::Type::point>(mAccessor, mChannel, mIndex, mCurrentCommit, [&](auto& frame)
                                                                                     {
                                                                                         if(std::abs(std::get<0_z>(frame) - mCurrentTime) > std::numeric_limits<double>::epsilon())
                                                                                         {
                                                                                             std::get<0_z>(frame) = mCurrentTime;
                                                                                             return true;
                                                                                         }
                                                                                         return false;
                                                                                     });
            }

        private:
            size_t const mIndex;
            double const mCurrentTime;
            double const mNewTime;
        };

        auto action = std::make_unique<Action>(mAccessor, mChannel, mIndex, mCurrentTime, time);
        if(action != nullptr)
        {
            undoManager.beginNewTransaction(juce::translate("Change Frame Time"));
            undoManager.perform(action.release());
        }
    };
}

void Track::Result::CellTime::resized()
{
    mHMSmsField.setFont(juce::Font(static_cast<float>(getHeight()) * 0.7f));
    mHMSmsField.setBounds(getLocalBounds().withTrimmedLeft(2).withWidth(114));
}

void Track::Result::CellTime::lookAndFeelChanged()
{
    resized();
}

void Track::Result::CellTime::update()
{
    auto const& results = mAccessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        return;
    }
    auto getInfo = [&]()
    {
        auto getInfoFor = [&](auto const& resultPtr) -> std::tuple<double, juce::Range<double>>
        {
            if(mChannel >= resultPtr->size())
            {
                return std::make_tuple(0.0, juce::Range<double>{});
            }
            auto const& channel = resultPtr->at(mChannel);
            if(mIndex >= channel.size())
            {
                return std::make_tuple(0.0, juce::Range<double>{});
            }

            auto const timeRange = mTimeZoomAccessor.getAttr<Zoom::AttrType::globalRange>();
            auto const currentTime = std::get<0_z>(channel.at(mIndex));
            auto const previousTime = mIndex > 0_z ? std::get<0_z>(channel.at(mIndex - 1_z)) + std::get<1_z>(channel.at(mIndex - 1_z)) : timeRange.getStart();
            auto const nextTime = mIndex < channel.size() - 1_z ? std::get<0_z>(channel.at(mIndex + 1_z)) : timeRange.getEnd();
            return std::make_tuple(currentTime, juce::Range<double>{previousTime, nextTime});
        };

        if(auto markers = results.getMarkers())
        {
            return getInfoFor(markers);
        }
        if(auto points = results.getPoints())
        {
            return getInfoFor(points);
        }
        return std::make_tuple(0.0, juce::Range<double>{});
    };

    auto const info = getInfo();
    mHMSmsField.setRange(std::get<1_z>(info), juce::NotificationType::dontSendNotification);
    mHMSmsField.setEnabled(!std::get<1_z>(info).isEmpty());
    mHMSmsField.setTime(std::get<0_z>(info), juce::NotificationType::dontSendNotification);
    mCurrentTime = std::get<0_z>(info);
}

Track::Result::CellDuration::CellDuration(Director& director, Zoom::Accessor& timeZoomAccessor, size_t channel, size_t index)
: Result::CellBase(director, timeZoomAccessor, channel, index)
{
    addAndMakeVisible(mHMSmsField);
    mHMSmsField.onTimeChanged = [this](double time)
    {
        auto& undoManager = mDirector.getUndoManager();
        class Action
        : public Modifier::ActionBase
        {
        public:
            Action(Accessor& accessor, size_t channel, size_t index, double currentDuration, double newDuration)
            : Modifier::ActionBase(accessor, channel)
            , mIndex(index)
            , mCurrentDuration(currentDuration)
            , mNewDuration(newDuration)
            {
            }

            ~Action() override = default;

            bool perform() override
            {
                Modifier::updateFrame<Data::Type::marker | Data::Type::point>(mAccessor, mChannel, mIndex, mNewCommit, [&](auto& frame)
                                                                              {
                                                                                  if(std::abs(std::get<1_z>(frame) - mNewDuration) > std::numeric_limits<double>::epsilon())
                                                                                  {
                                                                                      std::get<1_z>(frame) = mNewDuration;
                                                                                      return true;
                                                                                  }
                                                                                  return false;
                                                                              });
                return true;
            }

            bool undo() override
            {
                Modifier::updateFrame<Data::Type::marker | Data::Type::point>(mAccessor, mChannel, mIndex, mCurrentCommit, [&](auto& frame)
                                                                              {
                                                                                  if(std::abs(std::get<1_z>(frame) - mCurrentDuration) > std::numeric_limits<double>::epsilon())
                                                                                  {
                                                                                      std::get<1_z>(frame) = mCurrentDuration;
                                                                                      return true;
                                                                                  }
                                                                                  return false;
                                                                              });
                return true;
            }

        private:
            size_t const mIndex;
            double const mCurrentDuration;
            double const mNewDuration;
        };

        auto action = std::make_unique<Action>(mAccessor, mChannel, mIndex, mCurrentDuration, time);
        if(action != nullptr)
        {
            undoManager.beginNewTransaction(juce::translate("Change Frame Duration"));
            undoManager.perform(action.release());
        }
    };
}

void Track::Result::CellDuration::resized()
{
    mHMSmsField.setFont(juce::Font(static_cast<float>(getHeight()) * 0.7f));
    mHMSmsField.setBounds(getLocalBounds().withTrimmedLeft(2).withWidth(114));
}

void Track::Result::CellDuration::lookAndFeelChanged()
{
    resized();
}

void Track::Result::CellDuration::update()
{
    auto const& results = mAccessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        return;
    }
    auto getInfo = [&]()
    {
        auto getInfoFor = [&](auto const& resultPtr) -> std::tuple<double, juce::Range<double>>
        {
            if(mChannel >= resultPtr->size())
            {
                return std::make_tuple(0.0, juce::Range<double>{});
            }
            auto const& channel = resultPtr->at(mChannel);
            if(mIndex >= channel.size())
            {
                return std::make_tuple(0.0, juce::Range<double>{});
            }

            auto const timeRange = mTimeZoomAccessor.getAttr<Zoom::AttrType::globalRange>();
            auto const currentTime = std::get<1_z>(channel.at(mIndex));
            auto const nextTime = mIndex < channel.size() - 1_z ? std::get<0_z>(channel.at(mIndex + 1_z)) : timeRange.getEnd();
            return std::make_tuple(currentTime, juce::Range<double>{0.0, nextTime - std::get<0_z>(channel.at(mIndex))});
        };

        if(auto markers = results.getMarkers())
        {
            return getInfoFor(markers);
        }
        if(auto points = results.getPoints())
        {
            return getInfoFor(points);
        }
        return std::make_tuple(0.0, juce::Range<double>{});
    };

    auto const info = getInfo();
    mHMSmsField.setRange(std::get<1_z>(info), juce::NotificationType::dontSendNotification);
    mHMSmsField.setEnabled(!std::get<1_z>(info).isEmpty());
    mHMSmsField.setTime(std::get<0_z>(info), juce::NotificationType::dontSendNotification);
    mCurrentDuration = std::get<0_z>(info);
}

Track::Result::CellValue::CellValue(Director& director, Zoom::Accessor& timeZoomAccessor, size_t channel, size_t index)
: Result::CellBase(director, timeZoomAccessor, channel, index)
{
    addAndMakeVisible(mLabel);
    mLabel.setEditable(true);
    mLabel.onTextChange = [this]()
    {
        auto& undoManager = mDirector.getUndoManager();
        class Action
        : public Modifier::ActionBase
        {
        public:
            Action(Accessor& accessor, size_t channel, size_t index, std::string currentLabel, std::string newLabel)
            : Modifier::ActionBase(accessor, channel)
            , mIndex(index)
            , mCurrentLabel(currentLabel)
            , mNewLabel(newLabel)
            {
            }

            ~Action() override = default;

            bool perform() override
            {
                Modifier::updateFrame<Data::Type::marker>(mAccessor, mChannel, mIndex, mNewCommit, [&](auto& frame)
                                                          {
                                                              if(std::get<2_z>(frame) != mNewLabel)
                                                              {
                                                                  std::get<2_z>(frame) = mNewLabel;
                                                                  return true;
                                                              }
                                                              return false;
                                                          });
                return true;
            }

            bool undo() override
            {
                Modifier::updateFrame<Data::Type::marker>(mAccessor, mChannel, mIndex, mCurrentCommit, [&](auto& frame)
                                                          {
                                                              if(std::get<2_z>(frame) != mCurrentLabel)
                                                              {
                                                                  std::get<2_z>(frame) = mCurrentLabel;
                                                                  return true;
                                                              }
                                                              return false;
                                                          });
                return true;
            }

        private:
            size_t const mIndex;
            std::string const mCurrentLabel;
            std::string const mNewLabel;
        };

        auto action = std::make_unique<Action>(mAccessor, mChannel, mIndex, mCurrentLabel, mLabel.getText().toStdString());
        if(action != nullptr)
        {
            undoManager.beginNewTransaction(juce::translate("Change Frame Label"));
            undoManager.perform(action.release());
        }
    };

    addAndMakeVisible(mNumberField);
    mNumberField.setOptionalSupported(true, " - ");
    mNumberField.onValueChanged = [this](double newValue)
    {
        auto& undoManager = mDirector.getUndoManager();
        class Action
        : public Modifier::ActionBase
        {
        public:
            Action(Accessor& accessor, size_t channel, size_t index, std::optional<float> currentValue, std::optional<float> newValue)
            : Modifier::ActionBase(accessor, channel)
            , mIndex(index)
            , mCurrentValue(currentValue)
            , mNewValue(newValue)
            {
            }

            ~Action() override = default;

            bool perform() override
            {
                Modifier::updateFrame<Data::Type::point>(mAccessor, mChannel, mIndex, mNewCommit, [&](auto& frame)
                                                         {
                                                             if(mNewValue.has_value() != std::get<2_z>(frame).has_value() || (mNewValue.has_value() && *mNewValue != *std::get<2_z>(frame)))
                                                             {
                                                                 std::get<2_z>(frame) = mNewValue;
                                                                 return true;
                                                             }
                                                             return false;
                                                         });
                return true;
            }

            bool undo() override
            {
                Modifier::updateFrame<Data::Type::point>(mAccessor, mChannel, mIndex, mCurrentCommit, [&](auto& frame)
                                                         {
                                                             if(mCurrentValue.has_value() != std::get<2_z>(frame).has_value() || (mCurrentValue.has_value() && *mCurrentValue != *std::get<2_z>(frame)))
                                                             {
                                                                 std::get<2_z>(frame) = mCurrentValue;
                                                                 return true;
                                                             }
                                                             return false;
                                                         });
                return true;
            }

        private:
            size_t const mIndex;
            std::optional<float> const mCurrentValue;
            std::optional<float> const mNewValue;
        };

        auto action = std::make_unique<Action>(mAccessor, mChannel, mIndex, mCurrentValue, mNumberField.isOptional() ? std::optional<float>{} : std::optional<float>(static_cast<float>(newValue)));
        if(action != nullptr)
        {
            undoManager.beginNewTransaction(juce::translate("Change Frame Value"));
            undoManager.perform(action.release());
        }
    };
}

void Track::Result::CellValue::resized()
{
    mLabel.setFont(juce::Font(static_cast<float>(getHeight()) * 0.7f));
    mLabel.setBounds(getLocalBounds());
    mNumberField.setFont(juce::Font(static_cast<float>(getHeight()) * 0.7f));
    mNumberField.setBounds(getLocalBounds());
}

void Track::Result::CellValue::lookAndFeelChanged()
{
    resized();
}

void Track::Result::CellValue::update()
{
    auto const& results = mAccessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        return;
    }
    if(auto markers = results.getMarkers())
    {
        mLabel.setVisible(true);
        mNumberField.setVisible(false);
        if(mChannel >= markers->size())
        {
            mLabel.setEnabled(false);
            mLabel.setText("", juce::NotificationType::dontSendNotification);
            return;
        }
        auto const& channel = markers->at(mChannel);
        if(mIndex >= channel.size())
        {
            mLabel.setEnabled(false);
            mLabel.setText("", juce::NotificationType::dontSendNotification);
            return;
        }
        mLabel.setEnabled(true);
        auto const label = std::get<2_z>(channel.at(mIndex));
        mCurrentLabel = label;
        if(label.empty())
        {
            mLabel.setText(" - ", juce::NotificationType::dontSendNotification);
        }
        else
        {
            mLabel.setText(label, juce::NotificationType::dontSendNotification);
        }
        return;
    }
    if(auto points = results.getPoints())
    {
        mLabel.setVisible(false);
        mNumberField.setVisible(true);
        mNumberField.setNumDecimalsDisplayed(4);
        mNumberField.setTextValueSuffix(mAccessor.getAttr<AttrType::description>().output.unit);
        if(mChannel >= points->size())
        {
            mNumberField.setEnabled(false);
            mNumberField.setText("", juce::NotificationType::dontSendNotification);
            return;
        }
        auto const& channel = points->at(mChannel);
        if(mIndex >= channel.size())
        {
            mNumberField.setEnabled(false);
            mNumberField.setText("", juce::NotificationType::dontSendNotification);
            return;
        }
        mCurrentValue = std::get<2_z>(channel.at(mIndex));
        if(!std::get<2_z>(channel.at(mIndex)).has_value())
        {
            mNumberField.setEnabled(true);
            mNumberField.setText(" - ", juce::NotificationType::dontSendNotification);
            return;
        }
        mNumberField.setEnabled(true);
        mNumberField.setValue(*std::get<2_z>(channel.at(mIndex)), juce::NotificationType::dontSendNotification);
        return;
    }
    mLabel.setVisible(false);
    mNumberField.setVisible(false);
}

ANALYSE_FILE_END
