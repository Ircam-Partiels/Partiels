#include "AnlDocumentExporter.h"
#include "../Group/AnlGroupExporter.h"
#include "../Group/AnlGroupTools.h"
#include "../Track/AnlTrackExporter.h"
#include "../Track/AnlTrackRenderer.h"
#include "../Track/AnlTrackTools.h"
#include "AnlDocumentTools.h"

ANALYSE_FILE_BEGIN

juce::Rectangle<int> Document::Exporter::getPlotBounds(juce::String const& identifier)
{
    if(auto const* plot = getPlotComponent(identifier))
    {
        return plot->getBounds();
    }
    MiscWeakAssert(false && "Plot component not found for identifier");
    return juce::Rectangle<int>{};
}

std::tuple<int, int, int> Document::Exporter::getPlotDimension(juce::String const& identifier)
{
    auto const bounds = getPlotBounds(identifier);
    auto const* display = juce::Desktop::getInstance().getDisplays().getDisplayForRect(bounds.toNearestInt(), false);
    if(display == nullptr)
    {
        return std::make_tuple(bounds.getWidth(), bounds.getHeight(), 72);
    }
    auto const globalScale = juce::Desktop::getInstance().getGlobalScaleFactor();
    auto const dpi = static_cast<int>(std::round(static_cast<double>(globalScale) * display->scale * display->dpi));
    return std::make_tuple(bounds.getWidth(), bounds.getHeight(), dpi);
}

bool Document::Exporter::Options::operator==(Options const& rhd) const noexcept
{
    return format == rhd.format &&
           useGroupOverview == rhd.useGroupOverview &&
           useAutoSize == rhd.useAutoSize &&
           imageWidth == rhd.imageWidth &&
           imageHeight == rhd.imageHeight &&
           imagePpi == rhd.imagePpi &&
           includeHeaderRaw == rhd.includeHeaderRaw &&
           ignoreGridResults == rhd.ignoreGridResults &&
           applyExtraThresholds == rhd.applyExtraThresholds &&
           columnSeparator == rhd.columnSeparator &&
           reaperType == rhd.reaperType &&
           includeDescription == rhd.includeDescription &&
           sdifFrameSignature == rhd.sdifFrameSignature &&
           sdifMatrixSignature == rhd.sdifMatrixSignature &&
           sdifColumnName == rhd.sdifColumnName &&
           timePreset == rhd.timePreset &&
           outsideGridJustification == rhd.outsideGridJustification;
}

bool Document::Exporter::Options::operator!=(Options const& rhd) const noexcept
{
    return !(*this == rhd);
}

bool Document::Exporter::Options::useImageFormat() const noexcept
{
    return format == Format::jpeg || format == Format::png;
}

bool Document::Exporter::Options::useTextFormat() const noexcept
{
    return !useImageFormat();
}

juce::String Document::Exporter::Options::getFormatName() const
{
    return juce::String(std::string(magic_enum::enum_name(format)));
}

juce::String Document::Exporter::Options::getFormatExtension() const
{
    if(format == Format::reaper)
    {
        return "csv";
    }
    if(format == Format::puredata)
    {
        return "txt";
    }
    return getFormatName().toLowerCase();
}

juce::String Document::Exporter::Options::getFormatWilcard() const
{
    if(format == Format::jpeg)
    {
        return "*.jpeg;*.jpg";
    }
    return "*." + getFormatExtension();
}

char Document::Exporter::Options::getSeparatorChar() const
{
    switch(columnSeparator)
    {
        case ColumnSeparator::comma:
        {
            return ',';
        }
        case ColumnSeparator::space:
        {
            return ' ';
        }
        case ColumnSeparator::tab:
        {
            return '\t';
        }
        case ColumnSeparator::pipe:
        {
            return '|';
        }
        case ColumnSeparator::slash:
        {
            return '/';
        }
        case ColumnSeparator::colon:
        {
            return ':';
        }
        default:
        {
            return ',';
        }
    }
}

bool Document::Exporter::Options::isValid() const
{
    return format != Format::sdif || (sdifFrameSignature.length() == 4 && !sdifFrameSignature.contains("?") && sdifMatrixSignature.length() == 4 && !sdifMatrixSignature.contains("?"));
}

bool Document::Exporter::Options::isCompatible(Track::FrameType frameType) const
{
    switch(format)
    {
        case Format::jpeg:
            return true;
        case Format::png:
            return true;
        case Format::csv:
            return true;
        case Format::lab:
            return true;
        case Format::json:
            return true;
        case Format::cue:
            return frameType == Track::FrameType::label;
        case Format::reaper:
            return frameType == Track::FrameType::label;
        case Format::puredata:
            return true;
        case Format::sdif:
            return true;
    }
}

static std::vector<std::tuple<std::string, int, int>> const& getImageSizePresets()
{
    // clang-format off
    static std::vector<std::tuple<std::string, int, int>> const presets
    {
          {"Wide Ultra Extended Graphics Array", 1920, 1200}
        , {"A3 (300ppi)", 4960, 3508}
        , {"A4 (300ppi)", 3508, 2480}
        , {"A5 (300ppi)", 2480, 1748}
        , {"A6 (300ppi)", 1748, 1240}
        , {"A7 (300ppi)", 1240, 874}
        , {"HD (720p)", 1280, 720}
        , {"Full HD (1080p)", 1920, 1080}
        , {"4K UHD", 3840, 2160}
    };
    // clang-format on
    return presets;
}

Document::Exporter::Panel::Panel(Accessor& accessor, bool showTimeRange, bool showAutoSize)
: mAccessor(accessor)
, mShowAutoSize(showAutoSize)
, mPropertyItem("Item", "The item to export", "", std::vector<std::string>{""}, [this]([[maybe_unused]] size_t index)
                {
                    sanitizeProperties(true);
                })
, mPropertyTimePreset("Time Preset", "The preset of the time range to export", "", std::vector<std::string>{"Global", "Visible", "Selection", "Manual"}, [this](size_t index)
                      {
                          auto options = mOptions;
                          options.timePreset = magic_enum::enum_value<Document::Exporter::Options::TimePreset>(index);
                          setOptions(options, juce::NotificationType::sendNotificationSync);
                      })
, mPropertyTimeStart(juce::translate("Time Start"), juce::translate("The start time of the time range to export"), [this](double time)
                     {
                         setTimeRange(getTimeRange().withStart(time), true, juce::NotificationType::sendNotificationSync);
                     })
, mPropertyTimeEnd(juce::translate("Time End"), juce::translate("The end time of the time range to export"), [this](double time)
                   {
                       setTimeRange(getTimeRange().withEnd(time), true, juce::NotificationType::sendNotificationSync);
                   })
, mPropertyTimeLength(juce::translate("Time Length"), juce::translate("The time length of the time range to export"), [this](double time)
                      {
                          setTimeRange(getTimeRange().withLength(time), true, juce::NotificationType::sendNotificationSync);
                      })
, mPropertyFormat("Format", "Select the export format", "", std::vector<std::string>{"JPEG", "PNG", "CSV", "LAB", "JSON", "CUE", "REAPER", "PUREDATA (text)", "SDIF"}, [this](size_t index)
                  {
                      auto options = mOptions;
                      options.format = magic_enum::enum_value<Document::Exporter::Options::Format>(index);
                      setOptions(options, juce::NotificationType::sendNotificationSync);
                  })
, mPropertyGroupMode("Preserve Group Overlay", "Preserve group overlay of tracks when exporting", [this](bool state)
                     {
                         auto options = mOptions;
                         options.useGroupOverview = state;
                         setOptions(options, juce::NotificationType::sendNotificationSync);
                     })
, mPropertySizePreset("Image Size", "Select the size preset of the image", "", std::vector<std::string>{}, [this](size_t index)
                      {
                          auto options = mOptions;
                          options.useAutoSize = mShowAutoSize && index == 0_z;
                          if(options.useAutoSize)
                          {
                              auto const identifier = getSelectedIdentifier();
                              if(identifier.isNotEmpty())
                              {
                                  auto const dimension = getPlotDimension(identifier);
                                  auto const hasDimension = std::get<0_z>(dimension) > 0 && std::get<1_z>(dimension) > 0;
                                  if(hasDimension)
                                  {
                                      options.imageWidth = std::get<0_z>(dimension);
                                      options.imageHeight = std::get<1_z>(dimension);
                                      options.imagePpi = std::get<2_z>(dimension);
                                  }
                              }
                          }
                          if(!mShowAutoSize && !options.useAutoSize)
                          {
                              index--;
                          }
                          if(!options.useAutoSize && index < getImageSizePresets().size())
                          {
                              auto const& presets = getImageSizePresets();
                              options.imageWidth = std::get<1_z>(presets[index]);
                              options.imageHeight = std::get<2_z>(presets[index]);
                          }
                          setOptions(options, juce::NotificationType::sendNotificationSync);
                      })
, mPropertyWidth("Image Width", "Set the width of the image", "pixels", juce::Range<float>{1.0f, 100000000}, 1.0f, [this](float value)
                 {
                     auto options = mOptions;
                     auto const identifier = getSelectedIdentifier();
                     if(std::exchange(options.useAutoSize, false) && identifier.isNotEmpty())
                     {
                         auto const dimension = getPlotDimension(identifier);
                         auto const hasDimension = std::get<0_z>(dimension) > 0 && std::get<1_z>(dimension) > 0;
                         if(hasDimension)
                         {
                             options.imageWidth = std::get<0_z>(dimension);
                             options.imageHeight = std::get<1_z>(dimension);
                             options.imagePpi = std::get<2_z>(dimension);
                         }
                     }
                     options.imageWidth = std::max(static_cast<int>(std::round(value)), 1);
                     setOptions(options, juce::NotificationType::sendNotificationSync);
                 })
, mPropertyHeight("Image Height", "Set the height of the image", "pixels", juce::Range<float>{1.0f, 100000000}, 1.0f, [this](float value)
                  {
                      auto options = mOptions;
                      auto const identifier = getSelectedIdentifier();
                      if(std::exchange(options.useAutoSize, false) && identifier.isNotEmpty())
                      {
                          auto const dimension = getPlotDimension(identifier);
                          auto const hasDimension = std::get<0_z>(dimension) > 0 && std::get<1_z>(dimension) > 0;
                          if(hasDimension)
                          {
                              options.imageWidth = std::get<0_z>(dimension);
                              options.imageHeight = std::get<1_z>(dimension);
                              options.imagePpi = std::get<2_z>(dimension);
                          }
                      }
                      options.imageHeight = std::max(static_cast<int>(std::round(value)), 1);
                      setOptions(options, juce::NotificationType::sendNotificationSync);
                  })
, mPropertyPpi("Image PPI", "Set the pixel density of the image", "pixels/inch", juce::Range<float>{1.0f, 100000000}, 1.0f, [this](float value)
               {
                   auto options = mOptions;
                   auto const identifier = getSelectedIdentifier();
                   if(std::exchange(options.useAutoSize, false) && identifier.isNotEmpty())
                   {
                       auto const dimension = getPlotDimension(identifier);
                       auto const hasDimension = std::get<0_z>(dimension) > 0 && std::get<1_z>(dimension) > 0;
                       if(hasDimension)
                       {
                           options.imageWidth = std::get<0_z>(dimension);
                           options.imageHeight = std::get<1_z>(dimension);
                           options.imagePpi = std::get<2_z>(dimension);
                       }
                   }
                   options.imagePpi = std::max(static_cast<int>(std::round(value)), 1);
                   setOptions(options, juce::NotificationType::sendNotificationSync);
               })
, mPropertyRowHeader("Include Header Row", "Include header row before the data rows", [this](bool state)
                     {
                         auto options = mOptions;
                         options.includeHeaderRaw = state;
                         setOptions(options, juce::NotificationType::sendNotificationSync);
                     })
, mPropertyColumnSeparator("Column Separator", "The seperatror character between colummns", "", std::vector<std::string>{"Comma", "Space", "Tab", "Pipe", "Slash", "Colon"}, [this](size_t index)
                           {
                               auto options = mOptions;
                               options.columnSeparator = magic_enum::enum_value<Document::Exporter::Options::ColumnSeparator>(index);
                               setOptions(options, juce::NotificationType::sendNotificationSync);
                           })
, mPropertyReaperType("Reaper Type", "The Reaper data type", "", std::vector<std::string>{"Marker", "Region"}, [this](size_t index)
                      {
                          auto options = mOptions;
                          options.reaperType = magic_enum::enum_value<Document::Exporter::Options::ReaperType>(index);
                          setOptions(options, juce::NotificationType::sendNotificationSync);
                      })
, mPropertyIncludeDescription("Include Extra Description", "Include the extra description of the track in the results", [this](bool state)
                              {
                                  auto options = mOptions;
                                  options.includeDescription = state;
                                  setOptions(options, juce::NotificationType::sendNotificationSync);
                              })
, mPropertySdifFrame("Frame Signature", "Define the frame signature to encode the results in the SDIF file", [this](juce::String text)
                     {
                         while(text.length() < 4)
                         {
                             text += "?";
                         }
                         text = text.substring(0, 4).toUpperCase();
                         auto options = mOptions;
                         options.sdifFrameSignature = text;
                         setOptions(options, juce::NotificationType::sendNotificationSync);
                     })
, mPropertySdifMatrix("Matrix Signature", "Define the matrix signature to encode the results in the SDIF file", [this](juce::String text)
                      {
                          while(text.length() < 4)
                          {
                              text += "?";
                          }
                          text = text.substring(0, 4).toUpperCase();
                          auto options = mOptions;
                          options.sdifMatrixSignature = text;
                          setOptions(options, juce::NotificationType::sendNotificationSync);
                      })
, mPropertySdifColName("Column Name", "Define the name of the column to encode the results in the SDIF file", [this](juce::String text)
                       {
                           auto options = mOptions;
                           options.sdifColumnName = text;
                           setOptions(options, juce::NotificationType::sendNotificationSync);
                       })
, mPropertyIgnoreGrids("Ignore Matrix Tracks", "Ignore tracks with matrix results (such as a spectrogram)", [this](bool state)
                       {
                           auto options = mOptions;
                           options.ignoreGridResults = state;
                           setOptions(options, juce::NotificationType::sendNotificationSync);
                       })
, mPropertyApplyExtraThresholds("Apply Extra Thresholds", "Apply extra threshold filters to exclude values below thresholds when exporting", [this](bool state)
                                {
                                    auto options = mOptions;
                                    options.applyExtraThresholds = state;
                                    setOptions(options, juce::NotificationType::sendNotificationSync);
                                })
, mPropertyOutsideGridJustification("Outside Grid", "Draw grid ticks and labels outside the frame bounds", "", {}, nullptr)
, mDocumentLayoutNotifier(typeid(*this).name(), mAccessor, [this]()
                          {
                              updateItems();
                          },
                          {Group::AttrType::identifier}, {Track::AttrType::identifier, Track::AttrType::results, Track::AttrType::description})
{
    addAndMakeVisible(mPropertyItem);
    if(showTimeRange)
    {
        addAndMakeVisible(mPropertyTimePreset);
        addAndMakeVisible(mPropertyTimeStart);
        addAndMakeVisible(mPropertyTimeEnd);
        addAndMakeVisible(mPropertyTimeLength);
    }
    addAndMakeVisible(mPropertyFormat);
    addAndMakeVisible(mPropertyGroupMode);
    addAndMakeVisible(mPropertySizePreset);
    addAndMakeVisible(mPropertyWidth);
    addAndMakeVisible(mPropertyHeight);
    addAndMakeVisible(mPropertyPpi);
    addChildComponent(mPropertyRowHeader);
    addChildComponent(mPropertyColumnSeparator);
    addChildComponent(mPropertyReaperType);
    addChildComponent(mPropertyIncludeDescription);
    addChildComponent(mPropertySdifFrame);
    addChildComponent(mPropertySdifMatrix);
    addChildComponent(mPropertySdifColName);
    addChildComponent(mPropertyIgnoreGrids);
    addChildComponent(mPropertyApplyExtraThresholds);
    addChildComponent(mPropertyOutsideGridJustification);
    setSize(300, 200);

    mPropertySizePreset.entry.clear(juce::NotificationType::dontSendNotification);
    if(mShowAutoSize)
    {
        mPropertySizePreset.entry.addItem(juce::translate("Current Size(s)"), mPropertySizePreset.entry.getNumItems() + 1);
    }
    for(auto const& preset : getImageSizePresets())
    {
        mPropertySizePreset.entry.addItem(juce::translate(std::get<0_z>(preset)), mPropertySizePreset.entry.getNumItems() + 1);
    }
    mPropertySizePreset.entry.addItem(juce::translate("Manual"), mPropertySizePreset.entry.getNumItems() + 1);
    mPropertySizePreset.entry.setItemEnabled(mPropertySizePreset.entry.getNumItems(), false);

    mPropertySdifFrame.entry.onEditorShow = [this]()
    {
        if(auto* textEditor = mPropertySdifFrame.entry.getCurrentTextEditor())
        {
            textEditor->setInputRestrictions(4);
            textEditor->setJustification(juce::Justification::right);
        }
    };
    mPropertySdifFrame.entry.setText("????", juce::NotificationType::dontSendNotification);
    mPropertySdifMatrix.entry.onEditorShow = [this]()
    {
        if(auto* textEditor = mPropertySdifMatrix.entry.getCurrentTextEditor())
        {
            textEditor->setInputRestrictions(4);
            textEditor->setJustification(juce::Justification::right);
        }
    };
    mPropertySdifMatrix.entry.setText("????", juce::NotificationType::dontSendNotification);

    mPropertyOutsideGridJustification.entry.onShowPopup = [this]()
    {
        juce::PopupMenu menu;
        auto justification = mOptions.outsideGridJustification;
        menu.addItem(juce::translate("None"), justification.getFlags() != Zoom::Grid::Justification::none, justification.getFlags() == Zoom::Grid::Justification::none, [this]()
                     {
                         auto options = mOptions;
                         options.outsideGridJustification = Zoom::Grid::Justification(Zoom::Grid::Justification::none);
                         setOptions(options, juce::NotificationType::sendNotificationSync);
                         mPropertyOutsideGridJustification.entry.hidePopup();
                         mPropertyOutsideGridJustification.entry.onShowPopup();
                     });
        auto const addItem = [&, this](auto const& name, auto const bit)
        {
            menu.addItem(name, true, justification.testFlags(bit), [this, bit]()
                         {
                             auto options = mOptions;
                             auto const flags = options.outsideGridJustification.getFlags() ^ bit;
                             options.outsideGridJustification = Zoom::Grid::Justification(flags);
                             setOptions(options, juce::NotificationType::sendNotificationSync);
                             mPropertyOutsideGridJustification.entry.hidePopup();
                             mPropertyOutsideGridJustification.entry.showPopup();
                         });
        };
        addItem(juce::translate("Left"), Zoom::Grid::Justification::left);
        addItem(juce::translate("Right"), Zoom::Grid::Justification::right);
        addItem(juce::translate("Top"), Zoom::Grid::Justification::top);
        addItem(juce::translate("Bottom"), Zoom::Grid::Justification::bottom);
        auto& lf = getLookAndFeel();
        menu.setLookAndFeel(&lf);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(mPropertyOutsideGridJustification.entry).withMinimumWidth(mPropertyOutsideGridJustification.entry.getWidth()).withMaximumNumColumns(1).withDeletionCheck(*this), [this](int)
                           {
                               mPropertyOutsideGridJustification.entry.hidePopup();
                           });
    };

    mListener.onAccessorInserted = [this]([[maybe_unused]] Accessor const& acsr, AcsrType type, size_t index)
    {
        switch(type)
        {
            case AcsrType::tracks:
            {
                auto listener = std::make_unique<Track::Accessor::SmartListener>(typeid(*this).name(), mAccessor.getAcsr<AcsrType::tracks>(index), [this]([[maybe_unused]] Track::Accessor const& trackAcsr, Track::AttrType trackAttribute)
                                                                                 {
                                                                                     if(trackAttribute == Track::AttrType::name)
                                                                                     {
                                                                                         updateItems();
                                                                                     }
                                                                                 });
                mTrackListeners.emplace(mTrackListeners.begin() + static_cast<long>(index), std::move(listener));
                anlWeakAssert(mTrackListeners.size() == acsr.getNumAcsrs<AcsrType::tracks>());
            }
            break;
            case AcsrType::groups:
            {
                auto listener = std::make_unique<Group::Accessor::SmartListener>(typeid(*this).name(), mAccessor.getAcsr<AcsrType::groups>(index), [this]([[maybe_unused]] Group::Accessor const& groupAcsr, Group::AttrType groupAttribute)
                                                                                 {
                                                                                     if(groupAttribute == Group::AttrType::name || groupAttribute == Group::AttrType::layout)
                                                                                     {
                                                                                         updateItems();
                                                                                     }
                                                                                 });
                mGroupListeners.emplace(mGroupListeners.begin() + static_cast<long>(index), std::move(listener));
                anlWeakAssert(mGroupListeners.size() == acsr.getNumAcsrs<AcsrType::groups>());
            }
            break;
            case AcsrType::timeZoom:
            case AcsrType::transport:
                break;
        }
    };

    mListener.onAccessorErased = [this]([[maybe_unused]] Accessor const& acsr, AcsrType type, size_t index)
    {
        switch(type)
        {
            case AcsrType::tracks:
            {
                mTrackListeners.erase(mTrackListeners.begin() + static_cast<long>(index));
                anlWeakAssert(mTrackListeners.size() == acsr.getNumAcsrs<AcsrType::tracks>());
                updateItems();
            }
            break;
            case AcsrType::groups:
            {
                mGroupListeners.erase(mGroupListeners.begin() + static_cast<long>(index));
                anlWeakAssert(mGroupListeners.size() == acsr.getNumAcsrs<AcsrType::groups>());
                updateItems();
            }
            break;
            case AcsrType::timeZoom:
            case AcsrType::transport:
                break;
        }
    };

    mTimeZoomListener.onAttrChanged = [this]([[maybe_unused]] Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        switch(attribute)
        {
            case Zoom::AttrType::globalRange:
            case Zoom::AttrType::visibleRange:
            {
                updateTimePreset(true, juce::NotificationType::dontSendNotification);
            }
            break;
            case Zoom::AttrType::minimumLength:
            case Zoom::AttrType::anchor:
                break;
        }
    };

    mTransportZoomListener.onAttrChanged = [this]([[maybe_unused]] Transport::Accessor const& acsr, Transport::AttrType attribute)
    {
        switch(attribute)
        {
            case Transport::AttrType::playback:
            case Transport::AttrType::startPlayhead:
            case Transport::AttrType::runningPlayhead:
            case Transport::AttrType::looping:
            case Transport::AttrType::loopRange:
            case Transport::AttrType::stopAtLoopEnd:
            case Transport::AttrType::autoScroll:
            case Transport::AttrType::gain:
            case Transport::AttrType::markers:
            case Transport::AttrType::magnetize:
                break;
            case Transport::AttrType::selection:
            {
                updateTimePreset(true, juce::NotificationType::dontSendNotification);
            }
            break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::timeZoom>().addListener(mTimeZoomListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::transport>().addListener(mTransportZoomListener, NotificationType::synchronous);
    mOptions.format = Options::Format::sdif;
    setOptions({}, juce::NotificationType::dontSendNotification);
}

Document::Exporter::Panel::~Panel()
{
    mTrackListeners.clear();
    mAccessor.getAcsr<AcsrType::transport>().removeListener(mTransportZoomListener);
    mAccessor.getAcsr<AcsrType::timeZoom>().removeListener(mTimeZoomListener);
    mAccessor.removeListener(mListener);
}

void Document::Exporter::Panel::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto const setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    setBounds(mPropertyItem);
    setBounds(mPropertyTimePreset);
    setBounds(mPropertyTimeStart);
    setBounds(mPropertyTimeEnd);
    setBounds(mPropertyTimeLength);
    setBounds(mPropertyFormat);
    setBounds(mPropertyGroupMode);
    setBounds(mPropertySizePreset);
    setBounds(mPropertyWidth);
    setBounds(mPropertyHeight);
    setBounds(mPropertyPpi);
    setBounds(mPropertyRowHeader);
    setBounds(mPropertyColumnSeparator);
    setBounds(mPropertyReaperType);
    setBounds(mPropertyIncludeDescription);
    setBounds(mPropertySdifFrame);
    setBounds(mPropertySdifMatrix);
    setBounds(mPropertySdifColName);
    setBounds(mPropertyIgnoreGrids);
    setBounds(mPropertyApplyExtraThresholds);
    setBounds(mPropertyOutsideGridJustification);
    setSize(bounds.getWidth(), bounds.getY() + 2);
}

juce::String Document::Exporter::Panel::getSelectedIdentifier() const
{
    auto const itemId = mPropertyItem.entry.getSelectedId();
    if(itemId % documentItemFactor == 0)
    {
        return {};
    }

    auto const documentLayout = copy_with_erased_if(mAccessor.getAttr<AttrType::layout>(), [&](auto const& groupId)
                                                    {
                                                        return !Document::Tools::hasGroupAcsr(mAccessor, groupId);
                                                    });
    auto const groupIndex = static_cast<size_t>((itemId % documentItemFactor) / groupItemFactor - 1);
    anlWeakAssert(groupIndex < documentLayout.size());
    if(groupIndex >= documentLayout.size())
    {
        return {};
    }
    auto const groupId = documentLayout[groupIndex];
    if(itemId % groupItemFactor == 0)
    {
        return groupId;
    }
    anlWeakAssert(Document::Tools::hasGroupAcsr(mAccessor, groupId));
    if(!Document::Tools::hasGroupAcsr(mAccessor, groupId))
    {
        return {};
    }

    auto const& groupAcsr = Document::Tools::getGroupAcsr(mAccessor, groupId);
    auto const groupLayout = copy_with_erased_if(groupAcsr.getAttr<Group::AttrType::layout>(), [&](auto const& trackId)
                                                 {
                                                     return !Document::Tools::hasTrackAcsr(mAccessor, trackId);
                                                 });
    auto const trackIndex = static_cast<size_t>((itemId % groupItemFactor) - 1);
    anlWeakAssert(trackIndex < groupLayout.size());
    if(trackIndex >= groupLayout.size())
    {
        return {};
    }
    return groupLayout[trackIndex];
}

void Document::Exporter::Panel::updateItems()
{
    auto const selectedId = getSelectedIdentifier();

    auto& entryBox = mPropertyItem.entry;
    entryBox.clear(juce::NotificationType::dontSendNotification);
    entryBox.addItem(juce::translate("All (Document)"), documentItemFactor);
    auto const documentLayout = copy_with_erased_if(mAccessor.getAttr<Document::AttrType::layout>(), [&](auto const& groupId)
                                                    {
                                                        return !Document::Tools::hasGroupAcsr(mAccessor, groupId);
                                                    });

    for(size_t documentLayoutIndex = 0; documentLayoutIndex < documentLayout.size(); ++documentLayoutIndex)
    {
        auto const& groupId = documentLayout[documentLayoutIndex];
        auto const groupItemId = groupItemFactor * static_cast<int>(documentLayoutIndex + 1_z) + documentItemFactor;
        anlWeakAssert(Document::Tools::hasGroupAcsr(mAccessor, groupId));
        if(Document::Tools::hasGroupAcsr(mAccessor, groupId))
        {
            auto const& groupAcsr = Document::Tools::getGroupAcsr(mAccessor, groupId);
            auto const& groupName = groupAcsr.getAttr<Group::AttrType::name>();
            entryBox.addItem(juce::translate("GROUPNAME (Group)").replace("GROUPNAME", groupName), groupItemId);
            if(groupId == selectedId)
            {
                entryBox.setSelectedId(groupItemId, juce::NotificationType::dontSendNotification);
            }

            auto const groupLayout = copy_with_erased_if(groupAcsr.getAttr<Group::AttrType::layout>(), [&](auto const& trackId)
                                                         {
                                                             return !Document::Tools::hasTrackAcsr(mAccessor, trackId);
                                                         });

            for(size_t groupLayoutIndex = 0; groupLayoutIndex < groupLayout.size(); ++groupLayoutIndex)
            {
                auto const& trackId = groupLayout[groupLayoutIndex];
                auto const trackItemId = groupItemId + static_cast<int>(groupLayoutIndex + 1_z);
                anlWeakAssert(Document::Tools::hasTrackAcsr(mAccessor, trackId));
                if(Document::Tools::hasTrackAcsr(mAccessor, trackId))
                {
                    auto const& trackAcsr = Document::Tools::getTrackAcsr(mAccessor, trackId);
                    auto const& trackName = trackAcsr.getAttr<Track::AttrType::name>();
                    entryBox.addItem(juce::translate("GROUPNAME: TRACKNAME (Track)").replace("GROUPNAME", groupName).replace("TRACKNAME", trackName), trackItemId);
                    if(trackId == selectedId)
                    {
                        entryBox.setSelectedId(trackItemId, juce::NotificationType::dontSendNotification);
                    }
                }
            }
        }
    }

    if(entryBox.getSelectedId() == 0)
    {
        entryBox.setSelectedId(documentItemFactor, juce::NotificationType::dontSendNotification);
    }

    auto const& tracks = mAccessor.getAcsrs<AcsrType::tracks>();
    auto const hasLabel = std::any_of(tracks.cbegin(), tracks.cend(), [](auto const& trackAcsr)
                                      {
                                          return Track::Tools::getFrameType(trackAcsr.get()) == Track::FrameType::label;
                                      });
    mPropertyFormat.entry.setItemEnabled(mPropertyFormat.entry.getItemId(static_cast<int>(Options::Format::cue)), hasLabel);
    mPropertyFormat.entry.setItemEnabled(mPropertyFormat.entry.getItemId(static_cast<int>(Options::Format::reaper)), hasLabel);
    sanitizeProperties(false);
}

void Document::Exporter::Panel::sanitizeProperties(bool updateModel)
{
    auto const identifier = getSelectedIdentifier();
    auto const itemIsDocument = identifier.isEmpty();
    auto const itemIsGroup = Document::Tools::hasGroupAcsr(mAccessor, identifier);
    auto const itemIsTrack = Document::Tools::hasTrackAcsr(mAccessor, identifier);
    mPropertyGroupMode.setEnabled(itemIsGroup || itemIsDocument);

    if(itemIsDocument)
    {
        mPropertyWidth.setEnabled(true);
        mPropertyHeight.setEnabled(true);
        mPropertyPpi.setEnabled(true);
    }

    auto options = mOptions;

    if(itemIsTrack)
    {
        mPropertyGroupMode.setEnabled(false);
        options.useGroupOverview = false;
    }

    if(mShowAutoSize && mOptions.useAutoSize && !itemIsDocument)
    {
        auto const dimension = getPlotDimension(identifier);
        auto const hasDimension = std::get<0_z>(dimension) > 0 && std::get<1_z>(dimension) > 0;
        options.useAutoSize = hasDimension;
        if(hasDimension)
        {
            options.imageWidth = std::get<0_z>(dimension);
            options.imageHeight = std::get<1_z>(dimension);
            options.imagePpi = std::get<2_z>(dimension);
        }
        mPropertyWidth.entry.setValue(static_cast<double>(options.imageWidth), juce::NotificationType::dontSendNotification);
        mPropertyHeight.entry.setValue(static_cast<double>(options.imageHeight), juce::NotificationType::dontSendNotification);
        mPropertyPpi.entry.setValue(static_cast<double>(options.imagePpi), juce::NotificationType::dontSendNotification);
    }
    else if(mShowAutoSize && mOptions.useAutoSize && itemIsDocument)
    {
        mPropertyWidth.entry.setText(juce::translate("Multiple"), juce::NotificationType::dontSendNotification);
        mPropertyHeight.entry.setText(juce::translate("Multiple"), juce::NotificationType::dontSendNotification);
        mPropertyPpi.entry.setText(juce::translate("Multiple"), juce::NotificationType::dontSendNotification);
    }

    auto const& tracks = mAccessor.getAcsrs<AcsrType::tracks>();
    auto const hasLabel = std::any_of(tracks.cbegin(), tracks.cend(), [](auto const& trackAcsr)
                                      {
                                          return Track::Tools::getFrameType(trackAcsr.get()) == Track::FrameType::label;
                                      });
    if(!hasLabel && (mPropertyFormat.entry.getSelectedItemIndex() == static_cast<int>(Options::Format::reaper) || mPropertyFormat.entry.getSelectedItemIndex() == static_cast<int>(Options::Format::cue)))
    {
        options.format = Options::Format::csv;
    }
    if(updateModel)
    {
        setOptions(options, juce::NotificationType::sendNotificationSync);
    }

    auto const itemId = mPropertyItem.entry.getSelectedId();
    mPropertyIgnoreGrids.setEnabled(itemId % groupItemFactor == 0 && mOptions.format != Options::Format::cue && mOptions.format != Options::Format::reaper);
    mPropertyApplyExtraThresholds.setEnabled(mOptions.useTextFormat() && mOptions.format != Options::Format::sdif);
    mPropertyOutsideGridJustification.setEnabled(mOptions.useImageFormat());
}

Document::Exporter::Options const& Document::Exporter::Panel::getOptions() const
{
    return mOptions;
}

void Document::Exporter::Panel::setOptions(Options const& options, juce::NotificationType notification)
{
    if(mOptions == options)
    {
        return;
    }
    mOptions = options;
    auto constexpr silent = juce::NotificationType::dontSendNotification;
    mPropertyFormat.entry.setSelectedItemIndex(static_cast<int>(options.format), silent);
    mPropertyGroupMode.entry.setToggleState(options.useGroupOverview, silent);
    mPropertyWidth.entry.setValue(static_cast<double>(options.imageWidth), silent);
    mPropertyHeight.entry.setValue(static_cast<double>(options.imageHeight), silent);
    mPropertyPpi.entry.setValue(static_cast<double>(options.imagePpi), silent);
    if(options.useAutoSize)
    {
        mPropertySizePreset.entry.setSelectedItemIndex(0, silent);
        if(getSelectedIdentifier().isEmpty())
        {
            mPropertyWidth.entry.setText(juce::translate("Multiple"), silent);
            mPropertyHeight.entry.setText(juce::translate("Multiple"), silent);
            mPropertyPpi.entry.setText(juce::translate("Multiple"), silent);
        }
    }
    else
    {
        auto const& presets = getImageSizePresets();
        auto presetIt = std::find_if(presets.cbegin(), presets.cend(), [&](auto const& preset)
                                     {
                                         return options.imageWidth == std::get<1_z>(preset) && options.imageHeight == std::get<2_z>(preset);
                                     });
        if(presetIt != presets.cend())
        {
            auto const index = static_cast<int>(std::distance(presets.cbegin(), presetIt)) + (mShowAutoSize ? 1 : 0);
            mPropertySizePreset.entry.setSelectedItemIndex(index, silent);
        }
        else
        {
            mPropertySizePreset.entry.setSelectedItemIndex(mPropertySizePreset.entry.getNumItems() - 1, silent);
        }
    }

    mPropertyRowHeader.entry.setToggleState(options.includeHeaderRaw, silent);
    mPropertyColumnSeparator.entry.setSelectedItemIndex(static_cast<int>(options.columnSeparator), silent);
    mPropertyReaperType.entry.setSelectedItemIndex(static_cast<int>(options.reaperType), silent);
    mPropertyIgnoreGrids.entry.setToggleState(options.ignoreGridResults, silent);
    mPropertyApplyExtraThresholds.entry.setToggleState(options.applyExtraThresholds, silent);
    mPropertyIncludeDescription.entry.setToggleState(options.includeDescription, silent);
    mPropertySdifFrame.entry.setText(options.sdifFrameSignature, silent);
    mPropertySdifMatrix.entry.setText(options.sdifMatrixSignature, silent);
    mPropertySdifColName.entry.setText(options.sdifColumnName, silent);

    mPropertyGroupMode.setVisible(options.useImageFormat());
    mPropertySizePreset.setVisible(options.useImageFormat());
    mPropertyWidth.setVisible(options.useImageFormat());
    mPropertyHeight.setVisible(options.useImageFormat());
    mPropertyPpi.setVisible(options.useImageFormat());
    mPropertyRowHeader.setVisible(options.format == Document::Exporter::Options::Format::csv);
    mPropertyColumnSeparator.setVisible(options.format == Document::Exporter::Options::Format::csv);
    mPropertyReaperType.setVisible(options.format == Document::Exporter::Options::Format::reaper);
    mPropertyIncludeDescription.setVisible(options.format == Document::Exporter::Options::Format::json);
    mPropertySdifFrame.setVisible(options.format == Document::Exporter::Options::Format::sdif);
    mPropertySdifMatrix.setVisible(options.format == Document::Exporter::Options::Format::sdif);
    mPropertySdifColName.setVisible(options.format == Document::Exporter::Options::Format::sdif);
    mPropertyIgnoreGrids.setVisible(options.useTextFormat());
    mPropertyApplyExtraThresholds.setVisible(options.useTextFormat() && options.format != Options::Format::sdif);
    mPropertyOutsideGridJustification.setVisible(options.useImageFormat());
    juce::StringArray justificationNames;
    if(options.outsideGridJustification.getFlags() == Zoom::Grid::Justification::none)
    {
        justificationNames.add("None");
    }
    if(options.outsideGridJustification.testFlags(Zoom::Grid::Justification::left))
    {
        justificationNames.add(juce::translate("Left"));
    }
    if(options.outsideGridJustification.testFlags(Zoom::Grid::Justification::right))
    {
        justificationNames.add(juce::translate("Right"));
    }
    if(options.outsideGridJustification.testFlags(Zoom::Grid::Justification::top))
    {
        justificationNames.add(juce::translate("Top"));
    }
    if(options.outsideGridJustification.testFlags(Zoom::Grid::Justification::bottom))
    {
        justificationNames.add(juce::translate("Bottom"));
    }
    mPropertyOutsideGridJustification.entry.setText(justificationNames.joinIntoString(", "), juce::NotificationType::dontSendNotification);
    updateTimePreset(false, silent);
    sanitizeProperties(false);
    resized();

    if(notification == juce::NotificationType::dontSendNotification)
    {
        return;
    }
    else if(notification == juce::NotificationType::sendNotificationAsync)
    {
        triggerAsyncUpdate();
    }
    else
    {
        handleAsyncUpdate();
    }
}

void Document::Exporter::Panel::updateTimePreset(bool updateModel, juce::NotificationType notification)
{
    using TimePreset = Document::Exporter::Options::TimePreset;
    auto const& timeAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
    auto const globalTimeRange = timeAcsr.getAttr<Zoom::AttrType::globalRange>();
    auto const visibleTimeRange = timeAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const selectionTimeRange = mAccessor.getAcsr<AcsrType::transport>().getAttr<Transport::AttrType::selection>();

    mPropertyTimeStart.entry.setRange(globalTimeRange, notification);
    mPropertyTimeEnd.entry.setRange(globalTimeRange, notification);
    mPropertyTimeLength.entry.setRange({0.0, globalTimeRange.getLength()}, notification);

    auto& timePresetEntry = mPropertyTimePreset.entry;
    timePresetEntry.setItemEnabled(timePresetEntry.getItemId(static_cast<int>(TimePreset::global)), !globalTimeRange.isEmpty());
    timePresetEntry.setItemEnabled(timePresetEntry.getItemId(static_cast<int>(TimePreset::visible)), !visibleTimeRange.isEmpty() && globalTimeRange != visibleTimeRange);
    timePresetEntry.setItemEnabled(timePresetEntry.getItemId(static_cast<int>(TimePreset::selection)), !selectionTimeRange.isEmpty() && globalTimeRange != selectionTimeRange && visibleTimeRange != selectionTimeRange);
    timePresetEntry.setItemEnabled(timePresetEntry.getItemId(static_cast<int>(TimePreset::manual)), false);
    mPropertyTimePreset.entry.setSelectedItemIndex(static_cast<int>(mOptions.timePreset), notification);

    switch(mOptions.timePreset)
    {
        case Document::Exporter::Options::TimePreset::global:
        {
            setTimeRange(globalTimeRange, updateModel, notification);
        }
        break;
        case Options::TimePreset::visible:
        {
            setTimeRange(visibleTimeRange, updateModel, notification);
        }
        break;
        case Options::TimePreset::selection:
        {
            setTimeRange(selectionTimeRange, updateModel, notification);
        }
        break;
        case Options::TimePreset::manual:
        {
            setTimeRange(getTimeRange(), updateModel, notification);
        }
        break;
    }
}

void Document::Exporter::Panel::setTimeRange(juce::Range<double> const& range, bool updateModel, juce::NotificationType notification)
{
    auto const& timeAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
    auto const globalTimeRange = timeAcsr.getAttr<Zoom::AttrType::globalRange>();
    auto const visibleTimeRange = timeAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const selectionTimeRange = mAccessor.getAcsr<AcsrType::transport>().getAttr<Transport::AttrType::selection>();

    mPropertyTimeStart.entry.setTime(range.getStart(), notification);
    mPropertyTimeEnd.entry.setTime(range.getEnd(), notification);
    mPropertyTimeLength.entry.setTime(range.getLength(), notification);

    auto options = mOptions;
    if(!globalTimeRange.isEmpty() && (range == globalTimeRange || range.isEmpty()))
    {
        options.timePreset = Document::Exporter::Options::TimePreset::global;
    }
    else if(!visibleTimeRange.isEmpty() && range == visibleTimeRange)
    {
        options.timePreset = Document::Exporter::Options::TimePreset::visible;
    }
    else if(!selectionTimeRange.isEmpty() && range == selectionTimeRange)
    {
        options.timePreset = Document::Exporter::Options::TimePreset::selection;
    }
    else
    {
        options.timePreset = Document::Exporter::Options::TimePreset::manual;
    }
    setOptions(options, updateModel ? juce::NotificationType::sendNotificationSync : juce::NotificationType::dontSendNotification);
}

juce::Range<double> Document::Exporter::Panel::getTimeRange() const
{
    return {mPropertyTimeStart.entry.getTime(), mPropertyTimeEnd.entry.getTime()};
}

void Document::Exporter::Panel::handleAsyncUpdate()
{
    if(onOptionsChanged != nullptr)
    {
        onOptionsChanged();
    }
}

juce::Result Document::Exporter::toFile(Accessor& accessor, juce::File const file, juce::Range<double> const& timeRange, std::set<size_t> const& channels, juce::String const filePrefix, juce::String const& identifier, Options const& options, std::atomic<bool> const& shouldAbort)
{
    if(file == juce::File())
    {
        return juce::Result::fail("Invalid file");
    }
    if(!options.isValid())
    {
        return juce::Result::fail("Invalid options");
    }
    if(accessor.getNumAcsrs<AcsrType::tracks>() == 0_z)
    {
        return juce::Result::fail("Empty project");
    }

    auto const getAllTrackAcsrs = [&]() -> std::vector<std::reference_wrapper<Track::Accessor>>
    {
        if(identifier.isEmpty())
        {
            return accessor.getAcsrs<AcsrType::tracks>();
        }
        else if(Tools::hasGroupAcsr(accessor, identifier))
        {
            return Group::Tools::getTrackAcsrs(Tools::getGroupAcsr(accessor, identifier));
        }
        else if(Tools::hasTrackAcsr(accessor, identifier))
        {
            return {Tools::getTrackAcsr(accessor, identifier)};
        }
        return {};
    };

    if(options.format == Options::Format::reaper || options.format == Options::Format::cue)
    {
        auto const& tracks = getAllTrackAcsrs();
        if(std::none_of(tracks.cbegin(), tracks.cend(), [](auto const& trackAcsr)
                        {
                            return Track::Tools::getFrameType(trackAcsr.get()) == Track::FrameType::label;
                        }))
        {
            return juce::Result::fail("Invalid format for items");
        }
    }
    else if(options.useTextFormat() && options.ignoreGridResults)
    {
        auto const& tracks = getAllTrackAcsrs();
        if(tracks.size() > 1_z && std::all_of(tracks.cbegin(), tracks.cend(), [](auto const& trackAcsr)
                                              {
                                                  return Track::Tools::getFrameType(trackAcsr.get()) == Track::FrameType::vector;
                                              }))
        {
            return juce::Result::fail("Invalid format for items");
        }
    }

    if(options.useImageFormat())
    {
        auto const exportTrack = [&](juce::String const& trackIdentifier, juce::File const& trackFile)
        {
            juce::MessageManager::Lock lock;
            if(!lock.tryEnter())
            {
                return juce::Result::fail("Invalid threaded access");
            }
            auto const documentHasTrack = Tools::hasTrackAcsr(accessor, trackIdentifier);
            anlStrongAssert(documentHasTrack);
            if(!documentHasTrack)
            {
                return juce::Result::fail("Track is invalid");
            }

            auto const sizes = std::invoke([&]()
                                           {
                                               if(options.useAutoSize)
                                               {
                                                   auto const autoBounds = getPlotBounds(trackIdentifier);
                                                   auto const outsideBorder = Track::Renderer::getOutsideGridBorder(options.outsideGridJustification);
                                                   auto const bounds = outsideBorder.addedTo(autoBounds);
                                                   auto const scaledBounds = juce::Desktop::getInstance().getDisplays().logicalToPhysical(bounds);
                                                   return std::make_tuple(bounds.getWidth(), bounds.getHeight(), scaledBounds.getWidth(), scaledBounds.getHeight());
                                               }
                                               return std::make_tuple(options.imageWidth, options.imageHeight, static_cast<int>(std::round(static_cast<double>(options.imageWidth) * static_cast<double>(options.imagePpi) / 72.0)), static_cast<int>(std::round(static_cast<double>(options.imageHeight) * static_cast<double>(options.imagePpi) / 72.0)));
                                           });
            auto& trackAcsr = Tools::getTrackAcsr(accessor, trackIdentifier);
            Zoom::Accessor timeZoomAcsr;
            timeZoomAcsr.copyFrom(accessor.getAcsr<AcsrType::timeZoom>(), NotificationType::synchronous);
            if(!timeRange.isEmpty())
            {
                timeZoomAcsr.setAttr<Zoom::AttrType::visibleRange>(timeRange, NotificationType::synchronous);
            }
            auto const fileUsed = trackFile.isDirectory() ? trackFile.getNonexistentChildFile(filePrefix + trackAcsr.getAttr<Track::AttrType::name>(), "." + options.getFormatExtension()) : trackFile.getSiblingFile(filePrefix + trackFile.getFileName());
            lock.exit();

            return Track::Exporter::toImage(trackAcsr, timeZoomAcsr, channels, fileUsed, std::get<0>(sizes), std::get<1>(sizes), std::get<2>(sizes), std::get<3>(sizes), options.outsideGridJustification, shouldAbort);
        };

        auto const exportGroup = [&](juce::String const& groupIdentifier, juce::File const& groupFile)
        {
            juce::MessageManager::Lock lock;
            if(!lock.tryEnter())
            {
                return juce::Result::fail("Invalid threaded access");
            }
            auto const documentHasGroup = Tools::hasGroupAcsr(accessor, groupIdentifier);
            anlStrongAssert(documentHasGroup);
            if(!documentHasGroup)
            {
                return juce::Result::fail("Group is invalid");
            }

            auto const sizes = std::invoke([&]()
                                           {
                                               if(options.useAutoSize)
                                               {
                                                   auto const autoBounds = getPlotBounds(groupIdentifier);
                                                   auto const outsideBorder = Track::Renderer::getOutsideGridBorder(options.outsideGridJustification);
                                                   auto const bounds = outsideBorder.addedTo(autoBounds);
                                                   auto const scaledBounds = juce::Desktop::getInstance().getDisplays().logicalToPhysical(bounds);
                                                   return std::make_tuple(bounds.getWidth(), bounds.getHeight(), scaledBounds.getWidth(), scaledBounds.getHeight());
                                               }
                                               return std::make_tuple(options.imageWidth, options.imageHeight, static_cast<int>(std::round(static_cast<double>(options.imageWidth) * static_cast<double>(options.imagePpi) / 72.0)), static_cast<int>(std::round(static_cast<double>(options.imageHeight) * static_cast<double>(options.imagePpi) / 72.0)));
                                           });

            auto& groupAcsr = Tools::getGroupAcsr(accessor, groupIdentifier);
            Zoom::Accessor timeZoomAcsr;
            timeZoomAcsr.copyFrom(accessor.getAcsr<AcsrType::timeZoom>(), NotificationType::synchronous);
            if(!timeRange.isEmpty())
            {
                timeZoomAcsr.setAttr<Zoom::AttrType::visibleRange>(timeRange, NotificationType::synchronous);
            }
            auto const fileUsed = groupFile.isDirectory() ? groupFile.getNonexistentChildFile(filePrefix + groupAcsr.getAttr<Group::AttrType::name>(), "." + options.getFormatExtension()) : groupFile.getSiblingFile(filePrefix + groupFile.getFileName());
            lock.exit();

            return Group::Exporter::toImage(groupAcsr, timeZoomAcsr, channels, fileUsed, std::get<0>(sizes), std::get<1>(sizes), std::get<2>(sizes), std::get<3>(sizes), options.outsideGridJustification, shouldAbort);
        };

        auto const exportGroupTracks = [&](juce::String const& groupIdentifier, juce::File const& groupFolder)
        {
            juce::MessageManager::Lock lock;
            if(!lock.tryEnter())
            {
                return juce::Result::fail("Invalid threaded access");
            }
            auto const documentHasGroup = Tools::hasGroupAcsr(accessor, groupIdentifier);
            anlStrongAssert(documentHasGroup);
            if(!documentHasGroup)
            {
                return juce::Result::fail("Group is invalid");
            }

            auto const& groupAcsr = Tools::getGroupAcsr(accessor, groupIdentifier);
            auto const groupLayout = copy_with_erased_if(groupAcsr.getAttr<Group::AttrType::layout>(), [&](auto const& trackId)
                                                         {
                                                             return !Tools::hasTrackAcsr(accessor, trackId);
                                                         });
            auto const groupName = juce::File::createLegalFileName(groupAcsr.getAttr<Group::AttrType::name>());
            lock.exit();

            for(auto const& trackIdentifier : groupLayout)
            {
                if(!lock.tryEnter())
                {
                    return juce::Result::fail("Invalid threaded access");
                }
                auto const documentHasTrack = Tools::hasTrackAcsr(accessor, trackIdentifier);
                anlStrongAssert(documentHasTrack);
                if(!documentHasTrack)
                {
                    return juce::Result::fail("Track is invalid");
                }
                auto const& trackAcsr = Tools::getTrackAcsr(accessor, trackIdentifier);
                auto const trackName = juce::File::createLegalFileName(trackAcsr.getAttr<Track::AttrType::name>());
                lock.exit();

                auto const trackFile = groupFolder.getNonexistentChildFile(groupName + "_" + trackName, "." + options.getFormatExtension());
                auto const result = exportTrack(trackIdentifier, trackFile);
                if(result.failed())
                {
                    return result;
                }
            }
            return juce::Result::ok();
        };

        auto const exportDocumentGroups = [&](juce::File const& documentFolder)
        {
            juce::MessageManager::Lock lock;
            if(!lock.tryEnter())
            {
                return juce::Result::fail("Invalid threaded access");
            }
            auto const documentLayout = copy_with_erased_if(accessor.getAttr<AttrType::layout>(), [&](auto const& groupId)
                                                            {
                                                                return !Tools::hasGroupAcsr(accessor, groupId);
                                                            });
            lock.exit();

            for(auto const& groupIdentifier : documentLayout)
            {
                if(!lock.tryEnter())
                {
                    return juce::Result::fail("Invalid threaded access");
                }
                auto const documentHasGroup = Tools::hasGroupAcsr(accessor, groupIdentifier);
                anlStrongAssert(documentHasGroup);
                if(!documentHasGroup)
                {
                    return juce::Result::fail("Group is invalid");
                }

                auto const& groupAcsr = Tools::getGroupAcsr(accessor, groupIdentifier);
                auto const groupName = juce::File::createLegalFileName(groupAcsr.getAttr<Group::AttrType::name>());
                lock.exit();

                auto const groupFile = documentFolder.getNonexistentChildFile(groupName, "." + options.getFormatExtension());
                auto const result = exportGroup(groupIdentifier, documentFolder);
                if(result.failed())
                {
                    return result;
                }
            }
            return juce::Result::ok();
        };

        juce::MessageManager::Lock lock;
        if(!lock.tryEnter())
        {
            return juce::Result::fail("Invalid threaded access");
        }

        if(identifier.isEmpty())
        {
            if(options.useGroupOverview)
            {
                lock.exit();
                return exportDocumentGroups(file);
            }
            else
            {
                auto const documentLayout = copy_with_erased_if(accessor.getAttr<AttrType::layout>(), [&](auto const& groupId)
                                                                {
                                                                    return !Tools::hasGroupAcsr(accessor, groupId);
                                                                });
                lock.exit();

                for(auto const& groupIdentifier : documentLayout)
                {
                    auto const result = exportGroupTracks(groupIdentifier, file);
                    if(result.failed())
                    {
                        return result;
                    }
                }
                return juce::Result::ok();
            }
        }
        else if(Tools::hasGroupAcsr(accessor, identifier))
        {
            lock.exit();
            return options.useGroupOverview ? exportGroup(identifier, file) : exportGroupTracks(identifier, file);
        }
        else if(Tools::hasTrackAcsr(accessor, identifier))
        {
            lock.exit();
            return exportTrack(identifier, file);
        }
        return juce::Result::fail("Invalid identifier");
    }

    anlStrongAssert(options.useTextFormat());
    if(!options.useTextFormat())
    {
        return juce::Result::fail("Invalid format");
    }

    auto const exportTrack = [&](juce::String const& trackIdentifier, juce::File const& trackFile)
    {
        juce::MessageManager::Lock lock;
        if(!lock.tryEnter())
        {
            return juce::Result::fail("Invalid threaded access to model");
        }
        auto const documentHasTrack = Tools::hasTrackAcsr(accessor, trackIdentifier);
        anlStrongAssert(documentHasTrack);
        if(!documentHasTrack)
        {
            return juce::Result::fail("Track is invalid");
        }
        auto& trackAcsr = Tools::getTrackAcsr(accessor, trackIdentifier);
        if(identifier != trackIdentifier && options.ignoreGridResults && Track::Tools::getFrameType(trackAcsr) == Track::FrameType::vector)
        {
            return juce::Result::ok();
        }
        auto const isLabelOnly = options.format == Options::Format::cue || options.format == Options::Format::reaper;
        if(identifier != trackIdentifier && isLabelOnly && Track::Tools::getFrameType(trackAcsr) != Track::FrameType::label)
        {
            return juce::Result::ok();
        }
        auto const fileUsed = trackFile.isDirectory() ? trackFile.getNonexistentChildFile(filePrefix + trackAcsr.getAttr<Track::AttrType::name>(), "." + options.getFormatExtension()) : trackFile.getSiblingFile(filePrefix + trackFile.getFileName());
        lock.exit();

        switch(options.format)
        {
            case Options::Format::jpeg:
                return juce::Result::fail("Unsupported format");
            case Options::Format::png:
                return juce::Result::fail("Unsupported format");
            case Options::Format::csv:
                return Track::Exporter::toCsv(trackAcsr, timeRange, channels, fileUsed, options.includeHeaderRaw, options.getSeparatorChar(), false, options.applyExtraThresholds, "\n", false, false, shouldAbort);
            case Options::Format::lab:
                return Track::Exporter::toCsv(trackAcsr, timeRange, channels, fileUsed, false, '\t', true, options.applyExtraThresholds, "\n", false, false, shouldAbort);
            case Options::Format::puredata:
                return Track::Exporter::toCsv(trackAcsr, timeRange, channels, fileUsed, false, ' ', false, options.applyExtraThresholds, ";", true, false, shouldAbort);
            case Options::Format::json:
                return Track::Exporter::toJson(trackAcsr, timeRange, channels, fileUsed, options.includeDescription, options.applyExtraThresholds, shouldAbort);
            case Options::Format::cue:
                return Track::Exporter::toCue(trackAcsr, timeRange, channels, fileUsed, options.applyExtraThresholds, shouldAbort);
            case Options::Format::reaper:
                return Track::Exporter::toReaper(trackAcsr, timeRange, channels, fileUsed, options.reaperType == Options::ReaperType::marker, options.applyExtraThresholds, shouldAbort);
            case Options::Format::sdif:
            {
                auto const frameId = SdifConverter::getSignature(options.sdifFrameSignature);
                auto const matrixId = SdifConverter::getSignature(options.sdifMatrixSignature);
                auto const columnName = options.sdifColumnName.isEmpty() ? std::optional<juce::String>{} : options.sdifColumnName;
                return Track::Exporter::toSdif(trackAcsr, timeRange, fileUsed, frameId, matrixId, columnName, shouldAbort);
            }
        }
        return juce::Result::fail("Unsupported format");
    };

    auto const exportGroupTracks = [&](juce::String const& groupIdentifier, juce::File const& groupFolder)
    {
        juce::MessageManager::Lock lock;
        if(!lock.tryEnter())
        {
            return juce::Result::fail("Invalid threaded access");
        }
        auto const documentHasGroup = Tools::hasGroupAcsr(accessor, groupIdentifier);
        anlStrongAssert(documentHasGroup);
        if(!documentHasGroup)
        {
            return juce::Result::fail("Group is invalid");
        }
        auto const& groupAcsr = Tools::getGroupAcsr(accessor, groupIdentifier);
        auto const groupLayout = copy_with_erased_if(groupAcsr.getAttr<Group::AttrType::layout>(), [&](auto const& trackId)
                                                     {
                                                         return !Tools::hasTrackAcsr(accessor, trackId);
                                                     });
        auto const groupName = juce::File::createLegalFileName(groupAcsr.getAttr<Group::AttrType::name>());
        lock.exit();

        for(auto const& trackIdentifier : groupLayout)
        {
            if(!lock.tryEnter())
            {
                return juce::Result::fail("Invalid threaded access");
            }
            auto const documentHasTrack = Tools::hasTrackAcsr(accessor, trackIdentifier);
            anlStrongAssert(documentHasTrack);
            if(!documentHasTrack)
            {
                return juce::Result::fail("Track is invalid");
            }
            auto const& trackAcsr = Tools::getTrackAcsr(accessor, trackIdentifier);
            auto const trackName = juce::File::createLegalFileName(trackAcsr.getAttr<Track::AttrType::name>());
            lock.exit();

            auto const trackFile = groupFolder.getNonexistentChildFile(groupName + "_" + trackName, "." + options.getFormatExtension());
            auto const result = exportTrack(trackIdentifier, trackFile);
            if(result.failed())
            {
                return result;
            }
        }
        return juce::Result::ok();
    };

    juce::MessageManager::Lock lock;
    if(!lock.tryEnter())
    {
        return juce::Result::fail("Invalid threaded access");
    }

    if(identifier.isEmpty())
    {
        auto const documentLayout = copy_with_erased_if(accessor.getAttr<AttrType::layout>(), [&](auto const& groupId)
                                                        {
                                                            return !Tools::hasGroupAcsr(accessor, groupId);
                                                        });

        lock.exit();

        for(auto const& groupIdentifier : documentLayout)
        {
            auto const result = exportGroupTracks(groupIdentifier, file);
            if(result.failed())
            {
                return result;
            }
        }
        return juce::Result::ok();
    }
    else if(Tools::hasGroupAcsr(accessor, identifier))
    {
        lock.exit();
        return exportGroupTracks(identifier, file);
    }
    else if(Tools::hasTrackAcsr(accessor, identifier))
    {
        lock.exit();
        return exportTrack(identifier, file);
    }
    return juce::Result::fail("Invalid identifier");
}

template <>
void XmlParser::toXml<Document::Exporter::Options>(juce::XmlElement& xml, juce::Identifier const& attributeName, Document::Exporter::Options const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
        toXml(*child, "format", value.format);
        toXml(*child, "useGroupOverview", value.useGroupOverview);
        toXml(*child, "useAutoSize", value.useAutoSize);
        toXml(*child, "imageWidth", value.imageWidth);
        toXml(*child, "imageHeight", value.imageHeight);
        toXml(*child, "imagePpi", value.imagePpi);
        toXml(*child, "includeHeaderRaw", value.includeHeaderRaw);
        toXml(*child, "ignoreGridResults", value.ignoreGridResults);
        toXml(*child, "applyExtraThresholds", value.applyExtraThresholds);
        toXml(*child, "columnSeparator", value.columnSeparator);
        toXml(*child, "includeDescription", value.includeDescription);
        toXml(*child, "sdifFrameSignature", value.sdifFrameSignature);
        toXml(*child, "sdifMatrixSignature", value.sdifMatrixSignature);
        toXml(*child, "sdifColumnName", value.sdifColumnName);
        toXml(*child, "outsideGridJustification", value.outsideGridJustification.getFlags());
        xml.addChildElement(child.release());
    }
}

template <>
auto XmlParser::fromXml<Document::Exporter::Options>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Document::Exporter::Options const& defaultValue)
    -> Document::Exporter::Options
{
    auto const* child = xml.getChildByName(attributeName);
    anlWeakAssert(child != nullptr);
    if(child == nullptr)
    {
        return defaultValue;
    }
    Document::Exporter::Options value;
    value.format = fromXml(*child, "format", defaultValue.format);
    value.useGroupOverview = fromXml(*child, "useGroupOverview", defaultValue.useGroupOverview);
    value.useAutoSize = fromXml(*child, "useAutoSize", defaultValue.useAutoSize);
    value.imageWidth = fromXml(*child, "imageWidth", defaultValue.imageWidth);
    value.imageHeight = fromXml(*child, "imageHeight", defaultValue.imageHeight);
    value.imagePpi = fromXml(*child, "imagePpi", defaultValue.imagePpi);
    value.includeHeaderRaw = fromXml(*child, "includeHeaderRaw", defaultValue.includeHeaderRaw);
    value.ignoreGridResults = fromXml(*child, "ignoreGridResults", defaultValue.ignoreGridResults);
    value.applyExtraThresholds = fromXml(*child, "applyExtraThresholds", defaultValue.applyExtraThresholds);
    value.columnSeparator = fromXml(*child, "columnSeparator", defaultValue.columnSeparator);
    value.includeDescription = fromXml(*child, "includeDescription", defaultValue.includeDescription);
    value.sdifFrameSignature = fromXml(*child, "sdifFrameSignature", defaultValue.sdifFrameSignature);
    value.sdifMatrixSignature = fromXml(*child, "sdifMatrixSignature", defaultValue.sdifMatrixSignature);
    value.sdifColumnName = fromXml(*child, "sdifColumnName", defaultValue.sdifColumnName);
    value.outsideGridJustification = Zoom::Grid::Justification(fromXml(*child, "outsideGridJustification", defaultValue.outsideGridJustification.getFlags()));
    return value;
}

juce::Result Document::Exporter::clearUnusedAudioFiles(Accessor const& accessor, juce::File directory)
{
    directory = directory.getChildFile("Audio");
    if(!directory.exists() || !directory.isDirectory())
    {
        return juce::Result::ok();
    }

    juce::StringArray errors;
    auto reader = accessor.getAttr<AttrType::reader>();
    auto const childFiles = directory.findChildFiles(juce::File::TypesOfFileToFind::findFilesAndDirectories, true);
    for(auto& childFile : childFiles)
    {
        if(std::none_of(reader.cbegin(), reader.cend(), [&](AudioFileLayout const& audioFileLayout)
                        {
                            return audioFileLayout.file == childFile;
                        }))
        {
            if(!childFile.deleteFile())
            {
                errors.add(childFile.getFullPathName());
            }
        }
    }
    if(errors.isEmpty())
    {
        return juce::Result::ok();
    }
    return juce::Result::fail(juce::translate("Cannot clear unused audio files: LIST").replace("LIST", errors.joinIntoString(", ")));
}

juce::Result Document::Exporter::clearUnusedTrackFiles(Accessor const& accessor, juce::File directory)
{
    directory = directory.getChildFile("Track");
    if(!directory.exists() || !directory.isDirectory())
    {
        return juce::Result::ok();
    }

    juce::StringArray errors;
    auto const trackAcsrs = accessor.getAcsrs<AcsrType::tracks>();
    auto const childFiles = directory.findChildFiles(juce::File::TypesOfFileToFind::findFilesAndDirectories, true);
    for(auto& childFile : childFiles)
    {
        if(childFile.isDirectory() || std::none_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
                                                   {
                                                       return Track::Exporter::getConsolidatedFile(trackAcsr.get(), directory) == childFile;
                                                   }))
        {
            if(!childFile.deleteFile())
            {
                errors.add(childFile.getFullPathName());
            }
        }
    }
    if(errors.isEmpty())
    {
        return juce::Result::ok();
    }
    return juce::Result::fail(juce::translate("Cannot clear unused track files: LIST").replace("LIST", errors.joinIntoString(", ")));
}

juce::Result Document::Exporter::consolidateAudioFiles(Accessor& accessor, juce::File directory)
{
    directory = directory.getChildFile("Audio");
    auto reader = accessor.getAttr<AttrType::reader>();
    std::set<juce::File> audioFiles;
    std::set<juce::File> createdFiles;
    auto result = juce::Result::ok();
    for(auto i = 0_z; i < reader.size() && !result.failed(); ++i)
    {
        auto const newAudioFile = directory.getChildFile(reader[i].file.getFileName());
        if(audioFiles.insert(reader[i].file).second)
        {
            if(reader[i].file.exists() && reader[i].file != newAudioFile)
            {
                if(!directory.createDirectory())
                {
                    result = juce::Result::fail(juce::translate("Cannot create DIRNAME").replace("DIRNAME", directory.getFullPathName()));
                }
                if(reader[i].file.copyFileTo(newAudioFile))
                {
                    createdFiles.insert(newAudioFile);
                }
                else
                {
                    result = juce::Result::fail(juce::translate("Cannot copy from SRCFLNAME to DSTFLNAME").replace("SRCFLNAME", reader[i].file.getFullPathName()).replace("DSTFLNAME", newAudioFile.getFullPathName()));
                }
            }
        }
        reader[i].file = newAudioFile;
    }
    if(result.failed())
    {
        while(!createdFiles.empty())
        {
            createdFiles.begin()->deleteFile();
            createdFiles.erase(createdFiles.begin());
        }
    }
    else
    {
        accessor.setAttr<AttrType::reader>(reader, NotificationType::synchronous);
    }
    return result;
}

juce::Result Document::Exporter::consolidateTrackFiles(Accessor& accessor, juce::File directory)
{
    directory = directory.getChildFile("Track");
    auto trackAcsrs = accessor.getAcsrs<AcsrType::tracks>();
    for(auto& trackAcsr : trackAcsrs)
    {
        auto trackFileInfo = trackAcsr.get().getAttr<Track::AttrType::file>();
        if(trackFileInfo.commit.isNotEmpty())
        {
            auto const trackResult = Track::Exporter::consolidateInDirectory(trackAcsr.get(), directory);
            if(trackResult.failed())
            {
                return trackResult;
            }
            trackFileInfo.file = Track::Exporter::getConsolidatedFile(trackAcsr.get(), directory);
            trackAcsr.get().setAttr<Track::AttrType::file>(trackFileInfo, NotificationType::synchronous);
        }
    }
    return juce::Result::ok();
}

ANALYSE_FILE_END
