#include "AnlTrackResultCell.h"
#include "../AnlTrackTools.h"

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
    auto const isResultValid = [&](auto const& resultPtr) -> bool
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
            Action(std::function<Accessor&()> fn, size_t channel, size_t index, double currentTime, double newTime)
            : Modifier::ActionBase(fn, channel)
            , mIndex(index)
            , mCurrentTime(currentTime)
            , mNewTime(newTime)
            {
            }

            ~Action() override = default;

            bool perform() override
            {
                return Modifier::updateFrame < Data::Type::marker | Data::Type::point > (mGetAccessorFn(), mChannel, mIndex, mNewCommit, [&](auto& frame)
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
                return Modifier::updateFrame < Data::Type::marker | Data::Type::point > (mGetAccessorFn(), mChannel, mIndex, mCurrentCommit, [&](auto& frame)
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

        auto action = std::make_unique<Action>(mDirector.getSafeAccessorFn(), mChannel, mIndex, mCurrentTime, time);
        if(action != nullptr)
        {
            undoManager.beginNewTransaction(juce::translate("Change Frame Time"));
            undoManager.perform(action.release());
        }
    };
}

void Track::Result::CellTime::resized()
{
    mHMSmsField.setFont(juce::Font(juce::FontOptions(static_cast<float>(getHeight()) * 0.7f)));
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
            Action(std::function<Accessor&()> fn, size_t channel, size_t index, double currentDuration, double newDuration)
            : Modifier::ActionBase(fn, channel)
            , mIndex(index)
            , mCurrentDuration(currentDuration)
            , mNewDuration(newDuration)
            {
            }

            ~Action() override = default;

            bool perform() override
            {
                Modifier::updateFrame<Data::Type::marker | Data::Type::point>(mGetAccessorFn(), mChannel, mIndex, mNewCommit, [&](auto& frame)
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
                Modifier::updateFrame<Data::Type::marker | Data::Type::point>(mGetAccessorFn(), mChannel, mIndex, mCurrentCommit, [&](auto& frame)
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

        auto action = std::make_unique<Action>(mDirector.getSafeAccessorFn(), mChannel, mIndex, mCurrentDuration, time);
        if(action != nullptr)
        {
            undoManager.beginNewTransaction(juce::translate("Change Frame Duration"));
            undoManager.perform(action.release());
        }
    };
}

void Track::Result::CellDuration::resized()
{
    mHMSmsField.setFont(juce::Font(juce::FontOptions(static_cast<float>(getHeight()) * 0.7f)));
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
            Action(std::function<Accessor&()> fn, size_t channel, size_t index, std::string currentLabel, std::string newLabel)
            : Modifier::ActionBase(fn, channel)
            , mIndex(index)
            , mCurrentLabel(currentLabel)
            , mNewLabel(newLabel)
            {
            }

            ~Action() override = default;

            bool perform() override
            {
                Modifier::updateFrame<Data::Type::marker>(mGetAccessorFn(), mChannel, mIndex, mNewCommit, [&](auto& frame)
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
                Modifier::updateFrame<Data::Type::marker>(mGetAccessorFn(), mChannel, mIndex, mCurrentCommit, [&](auto& frame)
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

        auto action = std::make_unique<Action>(mDirector.getSafeAccessorFn(), mChannel, mIndex, mCurrentLabel, mLabel.getText().toStdString());
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
            Action(std::function<Accessor&()> fn, size_t channel, size_t index, std::optional<float> currentValue, std::optional<float> newValue)
            : Modifier::ActionBase(fn, channel)
            , mIndex(index)
            , mCurrentValue(currentValue)
            , mNewValue(newValue)
            {
            }

            ~Action() override = default;

            bool perform() override
            {
                Modifier::updateFrame<Data::Type::point>(mGetAccessorFn(), mChannel, mIndex, mNewCommit, [&](auto& frame)
                                                         {
                                                             if(mNewValue.has_value() != std::get<2_z>(frame).has_value() || (mNewValue.has_value() && std::abs(mNewValue.value() - std::get<2_z>(frame).value()) > std::numeric_limits<float>::epsilon()))
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
                Modifier::updateFrame<Data::Type::point>(mGetAccessorFn(), mChannel, mIndex, mCurrentCommit, [&](auto& frame)
                                                         {
                                                             if(mCurrentValue.has_value() != std::get<2_z>(frame).has_value() || (mCurrentValue.has_value() && std::abs(mCurrentValue.value() - std::get<2_z>(frame).value()) > std::numeric_limits<float>::epsilon()))
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

        auto action = std::make_unique<Action>(mDirector.getSafeAccessorFn(), mChannel, mIndex, mCurrentValue, mNumberField.isOptional() ? std::optional<float>{} : std::optional<float>(static_cast<float>(newValue)));
        if(action != nullptr)
        {
            undoManager.beginNewTransaction(juce::translate("Change Frame Value"));
            undoManager.perform(action.release());
        }
    };
}

void Track::Result::CellValue::resized()
{
    mLabel.setFont(juce::Font(juce::FontOptions(static_cast<float>(getHeight()) * 0.7f)));
    mLabel.setBounds(getLocalBounds());
    mNumberField.setFont(juce::Font(juce::FontOptions(static_cast<float>(getHeight()) * 0.7f)));
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
            juce::WeakReference<juce::Component> weakReference(this);
            mLabel.onEditorShow = [=, this]()
            {
                if(weakReference.get() == nullptr)
                {
                    return;
                }
                if(auto* editor = mLabel.getCurrentTextEditor())
                {
                    editor->setText("", false);
                }
            };
            mLabel.setText(" - ", juce::NotificationType::dontSendNotification);
        }
        else
        {
            mLabel.onEditorShow = nullptr;
            mLabel.setText(label, juce::NotificationType::dontSendNotification);
        }
        return;
    }
    if(auto points = results.getPoints())
    {
        mLabel.setVisible(false);
        mNumberField.setVisible(true);
        mNumberField.setNumDecimalsDisplayed(4);
        mNumberField.setTextValueSuffix(Track::Tools::getUnit(mAccessor));
        if(mAccessor.getAttr<AttrType::description>().output.hasKnownExtents)
        {
            auto const& output = mAccessor.getAttr<AttrType::description>().output;
            auto const min = static_cast<double>(output.minValue);
            auto const max = static_cast<double>(output.maxValue);
            auto const interval = output.isQuantized ? static_cast<double>(output.quantizeStep) : 0.0;
            mNumberField.setRange({min, max}, interval, juce::NotificationType::dontSendNotification);
        }
        else
        {
            mNumberField.setRange({0.0, 0.0}, 0.0, juce::NotificationType::dontSendNotification);
        }
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

Track::Result::CellExtra::CellExtra(Director& director, Zoom::Accessor& timeZoomAccessor, size_t channel, size_t index, size_t extraIndex)
: Result::CellBase(director, timeZoomAccessor, channel, index)
, mExtraIndex(extraIndex)
{
    addAndMakeVisible(mNumberField);
    mNumberField.onValueChanged = [this](double newValue)
    {
        auto& undoManager = mDirector.getUndoManager();
        class Action
        : public Modifier::ActionBase
        {
        public:
            Action(std::function<Accessor&()> fn, size_t channel, size_t index, size_t extraIndex, float currentValue, float newValue, float maxValue)
            : Modifier::ActionBase(fn, channel)
            , mIndex(index)
            , mExtraIndex(extraIndex)
            , mCurrentValue(currentValue)
            , mNewValue(newValue)
            , mMaxValue(maxValue)
            {
            }

            ~Action() override = default;

            bool perform() override
            {
                apply(mNewValue);
                return true;
            }

            bool undo() override
            {
                apply(mCurrentValue);
                return true;
            }

            bool apply(float value)
            {
                auto const applyValue = [value, index = mExtraIndex, maxValue = mMaxValue](auto& frame)
                {
                    if(index >= std::get<3_z>(frame).size())
                    {
                        std::get<3_z>(frame).resize(index + 1_z, maxValue);
                    }
                    auto& extra = std::get<3_z>(frame)[index];
                    if(std::abs(value - extra) < std::numeric_limits<float>::epsilon())
                    {
                        return false;
                    }
                    extra = value;
                    return true;
                };
                auto const frameType = Tools::getFrameType(mGetAccessorFn());
                if(frameType.has_value())
                {
                    switch(frameType.value())
                    {
                        case FrameType::label:
                            return Modifier::updateFrame<Data::Type::marker>(mGetAccessorFn(), mChannel, mIndex, mNewCommit, applyValue);
                        case FrameType::value:
                            return Modifier::updateFrame<Data::Type::point>(mGetAccessorFn(), mChannel, mIndex, mNewCommit, applyValue);
                        case FrameType::vector:
                            return Modifier::updateFrame<Data::Type::column>(mGetAccessorFn(), mChannel, mIndex, mNewCommit, applyValue);
                    }
                }
                return false;
            }

        private:
            size_t const mIndex;
            size_t const mExtraIndex;
            float const mCurrentValue;
            float const mNewValue;
            float const mMaxValue;
        };

        auto const maxValue = Tools::getExtraRange(mAccessor, mExtraIndex).value_or(juce::Range<double>()).getEnd();
        auto action = std::make_unique<Action>(mDirector.getSafeAccessorFn(), mChannel, mIndex, mExtraIndex, mCurrentValue, newValue, maxValue);
        if(action != nullptr)
        {
            undoManager.beginNewTransaction(juce::translate("Change Frame Extra Value"));
            undoManager.perform(action.release());
        }
    };
}

void Track::Result::CellExtra::resized()
{
    mNumberField.setFont(juce::Font(juce::FontOptions(static_cast<float>(getHeight()) * 0.7f)));
    mNumberField.setBounds(getLocalBounds());
}

void Track::Result::CellExtra::lookAndFeelChanged()
{
    resized();
}

void Track::Result::CellExtra::update()
{
    auto const& results = mAccessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access))
    {
        return;
    }
    auto const updateExtra = [this](auto const& resultPtr)
    {
        mNumberField.setNumDecimalsDisplayed(4);
        auto const& outputs = mAccessor.getAttr<AttrType::description>().extraOutputs;
        mNumberField.setTextValueSuffix(mExtraIndex < outputs.size() ? outputs.at(mExtraIndex).unit : "");
        if(mExtraIndex < outputs.size() && outputs.at(mExtraIndex).hasKnownExtents)
        {
            auto const range = Tools::getExtraRange(mAccessor, mExtraIndex);
            auto const& output = outputs.at(mExtraIndex);
            auto const interval = output.isQuantized ? static_cast<double>(output.quantizeStep) : 0.0;
            mNumberField.setRange(range.value_or(juce::Range<double>(0.0, 1.0)), interval, juce::NotificationType::dontSendNotification);
        }
        else
        {
            mNumberField.setRange({0.0, 0.0}, 0.0, juce::NotificationType::dontSendNotification);
        }
        if(mChannel >= resultPtr->size())
        {
            mNumberField.setEnabled(false);
            mNumberField.setText("", juce::NotificationType::dontSendNotification);
            return;
        }
        auto const& channel = resultPtr->at(mChannel);
        if(mIndex >= channel.size())
        {
            mNumberField.setEnabled(false);
            mNumberField.setText("", juce::NotificationType::dontSendNotification);
            return;
        }
        mNumberField.setEnabled(true);
        auto const& row = channel.at(mIndex);
        if(mExtraIndex >= std::get<3_z>(row).size())
        {
            mNumberField.setText("", juce::NotificationType::dontSendNotification);
            return;
        }
        mCurrentValue = std::get<3_z>(row).at(mExtraIndex);
        mNumberField.setValue(std::get<3_z>(row).at(mExtraIndex), juce::NotificationType::dontSendNotification);
    };
    if(auto markers = results.getMarkers())
    {
        updateExtra(markers);
    }
    else if(auto points = results.getPoints())
    {
        updateExtra(points);
    }
    else if(auto columns = results.getColumns())
    {
        updateExtra(columns);
    }
}

ANALYSE_FILE_END
