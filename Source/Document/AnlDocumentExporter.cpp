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

size_t Document::Exporter::getNumFilesToExport(Accessor const& accessor, std::set<juce::String> const& identifiers)
{
    return std::accumulate(identifiers.cbegin(), identifiers.cend(), 0_z, [&](auto const total, auto const& id)
                           {
                               return total + (Document::Tools::hasTrackAcsr(accessor, id) ? 1_z : (Document::Tools::hasGroupAcsr(accessor, id) ? Document::Tools::getGroupAcsr(accessor, id).template getAttr<Group::AttrType::layout>().size() : 0_z));
                           });
}

bool Document::Exporter::Options::operator==(Options const& rhs) const noexcept
{
    return format == rhs.format &&
           useAutoSize == rhs.useAutoSize &&
           imageWidth == rhs.imageWidth &&
           imageHeight == rhs.imageHeight &&
           imagePpi == rhs.imagePpi &&
           includeHeaderRow == rhs.includeHeaderRow &&
           applyExtraThresholds == rhs.applyExtraThresholds &&
           columnSeparator == rhs.columnSeparator &&
           labSeparator == rhs.labSeparator &&
           disableLabelEscaping == rhs.disableLabelEscaping &&
           reaperType == rhs.reaperType &&
           includeDescription == rhs.includeDescription &&
           sdifFrameSignature == rhs.sdifFrameSignature &&
           sdifMatrixSignature == rhs.sdifMatrixSignature &&
           sdifColumnName == rhs.sdifColumnName &&
           timePreset == rhs.timePreset &&
           outsideGridJustification == rhs.outsideGridJustification;
}

bool Document::Exporter::Options::operator!=(Options const& rhs) const noexcept
{
    return !(*this == rhs);
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
    if(format == Format::max)
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

char Document::Exporter::Options::getLabSeparatorChar() const
{
    switch(labSeparator)
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
            return ' ';
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
        case Format::max:
            return true;
        case Format::sdif:
            return true;
    }
}

void Document::Exporter::Options::setPlotDimension(juce::String const& identifier)
{
    auto const dimension = getPlotDimension(identifier);
    auto const hasDimension = std::get<0_z>(dimension) > 0 && std::get<1_z>(dimension) > 0;
    if(hasDimension)
    {
        imageWidth = std::get<0_z>(dimension);
        imageHeight = std::get<1_z>(dimension);
        imagePpi = std::get<2_z>(dimension);
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

static std::vector<std::string> getTimePresetName()
{
    // clang-format off
    return
    {
          juce::translate("Global").toStdString()
        , juce::translate("Visible").toStdString()
        , juce::translate("Selection").toStdString()
        , juce::translate("Manual").toStdString()
    };
    // clang-format on
}

static std::vector<std::string> getColumnSeparatorNames()
{
    // clang-format off
    return
    {
          juce::translate("Comma (,)").toStdString()
        , juce::translate("Space ( )").toStdString()
        , juce::translate("Tab (\\t)").toStdString()
        , juce::translate("Pipe (|)").toStdString()
        , juce::translate("Slash (/)").toStdString()
        , juce::translate("Colon (:)").toStdString()
    };
    // clang-format on
}

Document::Exporter::Panel::Panel(Accessor& accessor, bool showTimeRange, bool showAutoSize)
: mAccessor(accessor)
, mShowAutoSize(showAutoSize)
, mPropertyItem(juce::translate("Item"), juce::translate("The item to export"), "", std::vector<std::string>{""}, nullptr)
, mPropertyTimePreset(juce::translate("Time Preset"), juce::translate("The preset of the time range to export"), "", getTimePresetName(), [this](size_t index)
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
, mPropertyFormat(juce::translate("Format"), juce::translate("Select the export format"), "", std::vector<std::string>{"JPEG", "PNG", "CSV", "LAB", "JSON", "CUE", "REAPER", "PUREDATA (text)", "MAX (coll)", "SDIF"}, [this](size_t index)
                  {
                      auto options = mOptions;
                      options.format = magic_enum::enum_value<Document::Exporter::Options::Format>(index);
                      setOptions(options, juce::NotificationType::sendNotificationSync);
                  })
, mPropertySizePreset(juce::translate("Image Size"), juce::translate("Select the size preset of the image"), "", std::vector<std::string>{}, [this](size_t index)
                      {
                          auto options = mOptions;
                          options.useAutoSize = mShowAutoSize && index == 0_z;
                          if(options.useAutoSize)
                          {
                              auto const& identifiers = mSelectedIdentifiers;
                              if(identifiers.size() >= 1)
                              {
                                  options.setPlotDimension(*identifiers.begin());
                              }
                          }
                          if(mShowAutoSize)
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
, mPropertyWidth(juce::translate("Image Width"), juce::translate("Set the width of the image"), juce::translate("pixels"), juce::Range<float>{1.0f, 100000000}, 1.0f, [this](float value)
                 {
                     auto options = mOptions;
                     auto const& identifiers = mSelectedIdentifiers;
                     if(std::exchange(options.useAutoSize, false) && identifiers.size() >= 1)
                     {
                         options.setPlotDimension(*identifiers.begin());
                     }
                     options.imageWidth = std::max(static_cast<int>(std::round(value)), 1);
                     setOptions(options, juce::NotificationType::sendNotificationSync);
                 })
, mPropertyHeight(juce::translate("Image Height"), juce::translate("Set the height of the image"), juce::translate("pixels"), juce::Range<float>{1.0f, 100000000}, 1.0f, [this](float value)
                  {
                      auto options = mOptions;
                      auto const& identifiers = mSelectedIdentifiers;
                      if(std::exchange(options.useAutoSize, false) && identifiers.size() >= 1)
                      {
                          options.setPlotDimension(*identifiers.begin());
                      }
                      options.imageHeight = std::max(static_cast<int>(std::round(value)), 1);
                      setOptions(options, juce::NotificationType::sendNotificationSync);
                  })
, mPropertyPpi(juce::translate("Image PPI"), juce::translate("Set the pixel density of the image"), juce::translate("pixels/inch"), juce::Range<float>{1.0f, 100000000}, 1.0f, [this](float value)
               {
                   auto options = mOptions;
                   auto const& identifiers = mSelectedIdentifiers;
                   if(std::exchange(options.useAutoSize, false) && identifiers.size() >= 1)
                   {
                       options.setPlotDimension(*identifiers.begin());
                   }
                   options.imagePpi = std::max(static_cast<int>(std::round(value)), 1);
                   setOptions(options, juce::NotificationType::sendNotificationSync);
               })
, mPropertyRowHeader(juce::translate("Include Header Row"), juce::translate("Include header row before the data rows"), [this](bool state)
                     {
                         auto options = mOptions;
                         options.includeHeaderRow = state;
                         setOptions(options, juce::NotificationType::sendNotificationSync);
                     })
, mPropertyColumnSeparator(juce::translate("Column Separator"), juce::translate("The separator character between columns"), "", getColumnSeparatorNames(), [this](size_t index)
                           {
                               auto options = mOptions;
                               options.columnSeparator = magic_enum::enum_value<Document::Exporter::Options::ColumnSeparator>(index);
                               setOptions(options, juce::NotificationType::sendNotificationSync);
                           })
, mPropertyLabSeparator(juce::translate("Lab Separator"), juce::translate("The separator character for .lab format"), "", getColumnSeparatorNames(), [this](size_t index)
                        {
                            auto options = mOptions;
                            options.labSeparator = magic_enum::enum_value<Document::Exporter::Options::ColumnSeparator>(index);
                            setOptions(options, juce::NotificationType::sendNotificationSync);
                        })
, mPropertyDisableLabelEscaping(juce::translate("Disable Label Escaping"), juce::translate("Disable escaping of special characters in labels"), [this](bool state)
                                {
                                    auto options = mOptions;
                                    options.disableLabelEscaping = state;
                                    setOptions(options, juce::NotificationType::sendNotificationSync);
                                })
, mPropertyReaperType(juce::translate("Reaper Type"), juce::translate("The Reaper data type"), "", std::vector<std::string>{juce::translate("Marker").toStdString(), juce::translate("Region").toStdString()}, [this](size_t index)
                      {
                          auto options = mOptions;
                          options.reaperType = magic_enum::enum_value<Document::Exporter::Options::ReaperType>(index);
                          setOptions(options, juce::NotificationType::sendNotificationSync);
                      })
, mPropertyIncludeDescription(juce::translate("Include Extra Description"), juce::translate("Include the extra description of the track in the results"), [this](bool state)
                              {
                                  auto options = mOptions;
                                  options.includeDescription = state;
                                  setOptions(options, juce::NotificationType::sendNotificationSync);
                              })
, mPropertySdifFrame(juce::translate("Frame Signature"), juce::translate("Define the frame signature to encode the results in the SDIF file"), [this](juce::String text)
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
, mPropertySdifMatrix(juce::translate("Matrix Signature"), juce::translate("Define the matrix signature to encode the results in the SDIF file"), [this](juce::String text)
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
, mPropertySdifColName(juce::translate("Column Name"), juce::translate("Define the name of the column to encode the results in the SDIF file"), [this](juce::String text)
                       {
                           auto options = mOptions;
                           options.sdifColumnName = text;
                           setOptions(options, juce::NotificationType::sendNotificationSync);
                       })
, mPropertyApplyExtraThresholds(juce::translate("Apply Extra Thresholds"), juce::translate("Apply extra threshold filters to exclude values below thresholds when exporting"), [this](bool state)
                                {
                                    auto options = mOptions;
                                    options.applyExtraThresholds = state;
                                    setOptions(options, juce::NotificationType::sendNotificationSync);
                                })
, mPropertyOutsideGridJustification(juce::translate("Outside Grid"), juce::translate("Draw grid ticks and labels outside the frame bounds"), "", {}, nullptr)
, mDocumentLayoutNotifier(typeid(*this).name(), mAccessor, [this]()
                          {
                              setSelectedIdentifiers(mSelectedIdentifiers, juce::NotificationType::sendNotificationSync);
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
    addAndMakeVisible(mPropertySizePreset);
    addAndMakeVisible(mPropertyWidth);
    addAndMakeVisible(mPropertyHeight);
    addAndMakeVisible(mPropertyPpi);
    addChildComponent(mPropertyRowHeader);
    addChildComponent(mPropertyColumnSeparator);
    addChildComponent(mPropertyLabSeparator);
    addChildComponent(mPropertyDisableLabelEscaping);
    addChildComponent(mPropertyReaperType);
    addChildComponent(mPropertyIncludeDescription);
    addChildComponent(mPropertySdifFrame);
    addChildComponent(mPropertySdifMatrix);
    addChildComponent(mPropertySdifColName);
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

    mPropertyItem.entry.onShowPopup = [this]()
    {
        showItemPopup();
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
                                                                                         setSelectedIdentifiers(mSelectedIdentifiers, juce::NotificationType::sendNotificationSync);
                                                                                     }
                                                                                 });
                mTrackListeners.emplace(mTrackListeners.begin() + static_cast<long>(index), std::move(listener));
                anlWeakAssert(mTrackListeners.size() <= acsr.getNumAcsrs<AcsrType::tracks>());
            }
            break;
            case AcsrType::groups:
            {
                auto listener = std::make_unique<Group::Accessor::SmartListener>(typeid(*this).name(), mAccessor.getAcsr<AcsrType::groups>(index), [this]([[maybe_unused]] Group::Accessor const& groupAcsr, Group::AttrType groupAttribute)
                                                                                 {
                                                                                     if(groupAttribute == Group::AttrType::name || groupAttribute == Group::AttrType::layout)
                                                                                     {
                                                                                         setSelectedIdentifiers(mSelectedIdentifiers, juce::NotificationType::sendNotificationSync);
                                                                                     }
                                                                                 });
                mGroupListeners.emplace(mGroupListeners.begin() + static_cast<long>(index), std::move(listener));
                anlWeakAssert(mGroupListeners.size() <= acsr.getNumAcsrs<AcsrType::groups>());
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
                setSelectedIdentifiers(mSelectedIdentifiers, juce::NotificationType::sendNotificationSync);
            }
            break;
            case AcsrType::groups:
            {
                mGroupListeners.erase(mGroupListeners.begin() + static_cast<long>(index));
                anlWeakAssert(mGroupListeners.size() == acsr.getNumAcsrs<AcsrType::groups>());
                setSelectedIdentifiers(mSelectedIdentifiers, juce::NotificationType::sendNotificationSync);
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
    setBounds(mPropertySizePreset);
    setBounds(mPropertyWidth);
    setBounds(mPropertyHeight);
    setBounds(mPropertyPpi);
    setBounds(mPropertyRowHeader);
    setBounds(mPropertyColumnSeparator);
    setBounds(mPropertyLabSeparator);
    setBounds(mPropertyDisableLabelEscaping);
    setBounds(mPropertyReaperType);
    setBounds(mPropertyIncludeDescription);
    setBounds(mPropertySdifFrame);
    setBounds(mPropertySdifMatrix);
    setBounds(mPropertySdifColName);
    setBounds(mPropertyApplyExtraThresholds);
    setBounds(mPropertyOutsideGridJustification);
    setSize(bounds.getWidth(), bounds.getY() + 2);
}

std::set<juce::String> Document::Exporter::Panel::getSelectedIdentifiers() const
{
    return mSelectedIdentifiers;
}

void Document::Exporter::Panel::setSelectedIdentifiers(std::set<juce::String> const& identifiers, juce::NotificationType notification)
{
    auto const validateIdentifier = [this](juce::String const& identifier)
    {
        if(!Tools::hasItem(mAccessor, identifier))
        {
            return false;
        }
        if(Tools::hasGroupAcsr(mAccessor, identifier))
        {
            return mOptions.useImageFormat();
        }
        if(Tools::hasTrackAcsr(mAccessor, identifier))
        {
            auto const frameType = Track::Tools::getFrameType(Document::Tools::getTrackAcsr(mAccessor, identifier));
            return frameType.has_value() && mOptions.isCompatible(frameType.value());
        }
        MiscWeakAssert(false);
        return true;
    };
    auto selectedIdentifiers = copy_with_erased_if(identifiers, [=](auto const& identifier)
                                                   {
                                                       return !validateIdentifier(identifier);
                                                   });
    if(selectedIdentifiers == mSelectedIdentifiers)
    {
        notification = juce::NotificationType::dontSendNotification;
    }

    mSelectedIdentifiers = std::move(selectedIdentifiers);
    auto const numSelected = mSelectedIdentifiers.size();
    if(numSelected == 0_z)
    {
        mPropertyItem.entry.setText(juce::translate("None"), juce::NotificationType::dontSendNotification);
    }
    else if(numSelected == 1_z)
    {
        auto const& identifier = *mSelectedIdentifiers.begin();
        if(Document::Tools::hasTrackAcsr(mAccessor, identifier))
        {
            auto const& trackAcsr = Document::Tools::getTrackAcsr(mAccessor, identifier);
            auto const& groupAcsr = Document::Tools::getGroupAcsrForTrack(mAccessor, identifier);
            mPropertyItem.entry.setText(juce::translate("GROUPNAME: TRACKNAME (Track)")
                                            .replace("GROUPNAME", groupAcsr.getAttr<Group::AttrType::name>())
                                            .replace("TRACKNAME", trackAcsr.getAttr<Track::AttrType::name>()),
                                        juce::NotificationType::dontSendNotification);
        }
        else if(Document::Tools::hasGroupAcsr(mAccessor, identifier))
        {
            auto const& groupAcsr = Document::Tools::getGroupAcsr(mAccessor, identifier);
            mPropertyItem.entry.setText(juce::translate("GROUPNAME (Group Overlay)")
                                            .replace("GROUPNAME", groupAcsr.getAttr<Group::AttrType::name>()),
                                        juce::NotificationType::dontSendNotification);
        }
    }
    else if(isAllDocumentSelected())
    {
        mPropertyItem.entry.setText(juce::translate("All Document"), juce::NotificationType::dontSendNotification);
    }
    else
    {
        auto allGroupFound = false;
        auto const documentLayout = Tools::getEffectiveGroupIdentifiers(mAccessor);
        for(auto const& groupId : documentLayout)
        {
            std::set<juce::String> groupAndTracks;
            if(mOptions.useImageFormat())
            {
                groupAndTracks.insert(groupId);
            }
            for(auto const& trackId : Tools::getEffectiveTrackIdentifiers(mAccessor, groupId))
            {
                groupAndTracks.insert(trackId);
            }

            if(groupAndTracks == mSelectedIdentifiers)
            {
                auto const& groupAcsr = Document::Tools::getGroupAcsr(mAccessor, groupId);
                mPropertyItem.entry.setText(juce::translate("GROUPNAME (All Group)")
                                                .replace("GROUPNAME", groupAcsr.getAttr<Group::AttrType::name>()),
                                            juce::NotificationType::dontSendNotification);
                allGroupFound = true;
                break;
            }
        }
        if(!allGroupFound)
        {
            mPropertyItem.entry.setText(juce::translate("Multiple Items"), juce::NotificationType::dontSendNotification);
        }
    }

    auto const& tracks = mAccessor.getAcsrs<AcsrType::tracks>();
    auto const hasLabel = std::any_of(tracks.cbegin(), tracks.cend(), [](auto const& trackAcsr)
                                      {
                                          return Track::Tools::getFrameType(trackAcsr.get()) == Track::FrameType::label;
                                      });
    mPropertyFormat.entry.setItemEnabled(mPropertyFormat.entry.getItemId(static_cast<int>(Options::Format::cue)), hasLabel);
    mPropertyFormat.entry.setItemEnabled(mPropertyFormat.entry.getItemId(static_cast<int>(Options::Format::reaper)), hasLabel);
    sanitizeProperties(false);

    if(notification == juce::NotificationType::dontSendNotification)
    {
        return;
    }
    else if(notification == juce::NotificationType::sendNotificationAsync)
    {
        postCommandMessage(SelectionChangedId);
    }
    else
    {
        handleCommandMessage(SelectionChangedId);
    }
}

std::set<juce::String> Document::Exporter::Panel::getDefaultSelectedIdentifiers() const
{
    std::set<juce::String> selectedIdentifiers;
    auto const documentLayout = Tools::getEffectiveGroupIdentifiers(mAccessor);
    auto const useImageFormat = mOptions.useImageFormat();
    for(auto const& groupId : documentLayout)
    {
        if(useImageFormat)
        {
            selectedIdentifiers.insert(groupId);
        }
        else
        {
            auto const groupLayout = Tools::getEffectiveTrackIdentifiers(mAccessor, groupId);
            for(auto const& trackId : groupLayout)
            {
                auto const frameType = Track::Tools::getFrameType(Document::Tools::getTrackAcsr(mAccessor, trackId));
                // Exclude incompatible tracks by default and vector tracks because most of the time they are not useful to export
                if(frameType.has_value() && mOptions.isCompatible(frameType.value()) && frameType.value() != Track::FrameType::vector)
                {
                    selectedIdentifiers.insert(trackId);
                }
            }
        }
    }
    return selectedIdentifiers;
}

bool Document::Exporter::Panel::isAllDocumentSelected() const
{
    auto const documentLayout = Tools::getEffectiveGroupIdentifiers(mAccessor);
    return std::all_of(documentLayout.cbegin(), documentLayout.cend(), [this](auto const& groupId)
                       {
                           return isAllGroupSelected(groupId);
                       });
}

bool Document::Exporter::Panel::isAllGroupSelected(juce::String const& groupId) const
{
    if(mOptions.useImageFormat() && !mSelectedIdentifiers.contains(groupId))
    {
        return false;
    }
    auto const groupLayout = Tools::getEffectiveTrackIdentifiers(mAccessor, groupId);
    return std::all_of(groupLayout.cbegin(), groupLayout.cend(), [this](auto const& trackId)
                       {
                           return mSelectedIdentifiers.contains(trackId);
                       });
}

void Document::Exporter::Panel::showItemPopup(int groupToShow)
{
    mPropertyItem.entry.hidePopup();
    auto const documentLayout = Tools::getEffectiveGroupIdentifiers(mAccessor);
    if(documentLayout.empty())
    {
        return;
    }

    juce::PopupMenu menu;
    auto const allSelected = isAllDocumentSelected();
    menu.addItem(juce::translate("All (Document)"), true, allSelected, [=, this]()
                 {
                     if(allSelected)
                     {
                         setSelectedIdentifiers({}, juce::NotificationType::sendNotificationSync);
                     }
                     else
                     {
                         auto identifiers = mSelectedIdentifiers;
                         if(mOptions.useImageFormat())
                         {
                             for(auto const& groupId : Tools::getEffectiveGroupIdentifiers(mAccessor))
                             {
                                 identifiers.insert(groupId);
                             }
                         }
                         for(auto const& trackId : Tools::getEffectiveTrackIdentifiers(mAccessor))
                         {
                             auto const frameType = Track::Tools::getFrameType(Document::Tools::getTrackAcsr(mAccessor, trackId));
                             if(frameType.has_value() && mOptions.isCompatible(frameType.value()))
                             {
                                 identifiers.insert(trackId);
                             }
                         }
                         setSelectedIdentifiers(identifiers, juce::NotificationType::sendNotificationSync);
                     }
                     sanitizeProperties(true);
                     showItemPopup();
                 });

    auto groupItemPopupId = 1000;
    for(auto const& groupId : documentLayout)
    {
        juce::PopupMenu groupMenu;
        auto const& groupAcsr = Document::Tools::getGroupAcsr(mAccessor, groupId);
        auto const& groupName = groupAcsr.getAttr<Group::AttrType::name>();
        if(mOptions.useImageFormat())
        {
            auto const groupSelected = mSelectedIdentifiers.count(groupId) > 0_z;
            groupMenu.addItem(juce::translate("Overlay (Group)"), true, groupSelected, [=, this]()
                              {
                                  auto identifiers = mSelectedIdentifiers;
                                  if(identifiers.erase(groupId) == 0_z)
                                  {
                                      identifiers.insert(groupId);
                                  }
                                  setSelectedIdentifiers(identifiers, juce::NotificationType::sendNotificationSync);
                                  sanitizeProperties(true);
                                  showItemPopup(groupItemPopupId);
                              });
        }

        auto const groupLayout = Tools::getEffectiveTrackIdentifiers(mAccessor, groupId);
        for(auto const& trackId : groupLayout)
        {
            auto const& trackAcsr = Document::Tools::getTrackAcsr(mAccessor, trackId);
            auto const& trackName = trackAcsr.getAttr<Track::AttrType::name>();
            auto const trackFrameType = Track::Tools::getFrameType(trackAcsr);
            auto const trackSelected = mSelectedIdentifiers.count(trackId) > 0_z;
            juce::PopupMenu::Item i(juce::translate("TRACKNAME (Track)").replace("TRACKNAME", trackName));
            i.isEnabled = trackFrameType.has_value() && mOptions.isCompatible(trackFrameType.value());
            i.isTicked = trackSelected;
            i.action = [=, this]()
            {
                auto identifiers = mSelectedIdentifiers;
                if(identifiers.erase(trackId) == 0_z)
                {
                    identifiers.insert(trackId);
                }
                setSelectedIdentifiers(identifiers, juce::NotificationType::sendNotificationSync);
                sanitizeProperties(true);
                showItemPopup(groupItemPopupId);
            };
            groupMenu.addItem(std::move(i));
        }

        auto const groupSelected = isAllGroupSelected(groupId);
        juce::PopupMenu::Item i(juce::translate("GROUPNAME (Group)").replace("GROUPNAME", groupName));
        i.itemID = groupItemPopupId;
        i.isTicked = groupSelected;
        i.isEnabled = !groupLayout.empty();
        i.subMenu.reset(new juce::PopupMenu(groupMenu));
        i.action = [=, this]()
        {
            auto identifiers = mSelectedIdentifiers;
            if(groupSelected)
            {
                identifiers.erase(groupId);
                for(auto const& trackId : Tools::getEffectiveTrackIdentifiers(mAccessor, groupId))
                {
                    identifiers.erase(trackId);
                }
            }
            else
            {
                if(mOptions.useImageFormat())
                {
                    identifiers.insert(groupId);
                }
                for(auto const& trackId : Tools::getEffectiveTrackIdentifiers(mAccessor, groupId))
                {
                    identifiers.insert(trackId);
                }
            }
            setSelectedIdentifiers(identifiers, juce::NotificationType::sendNotificationSync);
            sanitizeProperties(true);
            showItemPopup(groupItemPopupId);
        };
        menu.addItem(std::move(i));
        ++groupItemPopupId;
    }

    menu.setLookAndFeel(&getLookAndFeel());
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(mPropertyItem.entry).withMinimumWidth(mPropertyItem.entry.getWidth()).withMaximumNumColumns(1).withDeletionCheck(*this).withVisibleSubMenu(groupToShow));
}

void Document::Exporter::Panel::sanitizeProperties(bool updateModel)
{
    auto options = mOptions;
    if(mShowAutoSize && mOptions.useAutoSize)
    {
        if(mSelectedIdentifiers.size() > 1_z)
        {
            mPropertyWidth.entry.setText(juce::translate("Multiple"), juce::NotificationType::dontSendNotification);
            mPropertyHeight.entry.setText(juce::translate("Multiple"), juce::NotificationType::dontSendNotification);
            mPropertyPpi.entry.setText(juce::translate("Multiple"), juce::NotificationType::dontSendNotification);
        }
        else if(!mSelectedIdentifiers.empty())
        {
            auto const& identifier = *mSelectedIdentifiers.begin();
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
    }
    mPropertyApplyExtraThresholds.setEnabled(mOptions.useTextFormat() && mOptions.format != Options::Format::sdif);
    mPropertyOutsideGridJustification.setEnabled(mOptions.useImageFormat());
    if(updateModel)
    {
        setOptions(options, juce::NotificationType::sendNotificationSync);
    }
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
    mPropertyWidth.entry.setValue(static_cast<double>(options.imageWidth), silent);
    mPropertyHeight.entry.setValue(static_cast<double>(options.imageHeight), silent);
    mPropertyPpi.entry.setValue(static_cast<double>(options.imagePpi), silent);
    if(options.useAutoSize)
    {
        mPropertySizePreset.entry.setSelectedItemIndex(0, silent);
        auto const& identifiers = mSelectedIdentifiers;
        if(identifiers.size() > 1)
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

    mPropertyRowHeader.entry.setToggleState(options.includeHeaderRow, silent);
    mPropertyColumnSeparator.entry.setSelectedItemIndex(static_cast<int>(options.columnSeparator), silent);
    mPropertyLabSeparator.entry.setSelectedItemIndex(static_cast<int>(options.labSeparator), silent);
    mPropertyDisableLabelEscaping.entry.setToggleState(options.disableLabelEscaping, silent);
    mPropertyReaperType.entry.setSelectedItemIndex(static_cast<int>(options.reaperType), silent);
    mPropertyApplyExtraThresholds.entry.setToggleState(options.applyExtraThresholds, silent);
    mPropertyIncludeDescription.entry.setToggleState(options.includeDescription, silent);
    mPropertySdifFrame.entry.setText(options.sdifFrameSignature, silent);
    mPropertySdifMatrix.entry.setText(options.sdifMatrixSignature, silent);
    mPropertySdifColName.entry.setText(options.sdifColumnName, silent);

    mPropertySizePreset.setVisible(options.useImageFormat());
    mPropertyWidth.setVisible(options.useImageFormat());
    mPropertyHeight.setVisible(options.useImageFormat());
    mPropertyPpi.setVisible(options.useImageFormat());
    mPropertyRowHeader.setVisible(options.format == Document::Exporter::Options::Format::csv);
    mPropertyColumnSeparator.setVisible(options.format == Document::Exporter::Options::Format::csv);
    mPropertyLabSeparator.setVisible(options.format == Document::Exporter::Options::Format::lab);
    mPropertyDisableLabelEscaping.setVisible(options.format == Document::Exporter::Options::Format::csv || options.format == Document::Exporter::Options::Format::lab);
    mPropertyReaperType.setVisible(options.format == Document::Exporter::Options::Format::reaper);
    mPropertyIncludeDescription.setVisible(options.format == Document::Exporter::Options::Format::json);
    mPropertySdifFrame.setVisible(options.format == Document::Exporter::Options::Format::sdif);
    mPropertySdifMatrix.setVisible(options.format == Document::Exporter::Options::Format::sdif);
    mPropertySdifColName.setVisible(options.format == Document::Exporter::Options::Format::sdif);
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
    setSelectedIdentifiers(mSelectedIdentifiers, notification);
    resized();

    if(notification == juce::NotificationType::dontSendNotification)
    {
        return;
    }
    else if(notification == juce::NotificationType::sendNotificationAsync)
    {
        postCommandMessage(OptionsChangedId);
    }
    else
    {
        handleCommandMessage(OptionsChangedId);
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

void Document::Exporter::Panel::handleCommandMessage(int commandId)
{
    switch(commandId)
    {
        case OptionsChangedId:
        {
            if(onOptionsChanged != nullptr)
            {
                onOptionsChanged();
            }
            break;
        }
        case SelectionChangedId:
        {
            if(onSelectionChanged != nullptr)
            {
                onSelectionChanged();
            }
            break;
        }
        default:
            break;
    }
}

juce::Result Document::Exporter::exportTo(Accessor& accessor, juce::File const directory, juce::Range<double> const& timeRange, std::set<size_t> const& channels, juce::String const filePrefix, std::set<juce::String> const& identifiers, Options const& options, std::atomic<bool> const& shouldAbort)
{
    MiscWeakAssert(identifiers.size() > 0_z);
    if(identifiers.empty())
    {
        MiscDebug("Exporter", "No identifiers provided");
        return juce::Result::fail(juce::translate("No identifiers provided"));
    }
    for(auto const& identifier : identifiers)
    {
        auto const result = exportTo(accessor, directory, timeRange, channels, filePrefix, identifier, options, shouldAbort);
        if(result.failed())
        {
            return result;
        }
    }
    return juce::Result::ok();
}

juce::Result Document::Exporter::exportTo(Accessor& accessor, juce::File const file, juce::Range<double> const& timeRange, std::set<size_t> const& channels, juce::String const filePrefix, juce::String const& identifier, Options const& options, std::atomic<bool> const& shouldAbort)
{
    if(file == juce::File())
    {
        MiscDebug("Exporter", "Invalid file");
        return juce::Result::fail(juce::translate("Invalid file"));
    }
    if(!options.isValid())
    {
        MiscDebug("Exporter", "Invalid options");
        return juce::Result::fail(juce::translate("Invalid options"));
    }
    if(accessor.getNumAcsrs<AcsrType::tracks>() == 0_z)
    {
        MiscDebug("Exporter", "Empty project");
        return juce::Result::fail(juce::translate("Empty project"));
    }

    juce::MessageManager::Lock lock;
    if(!lock.tryEnter())
    {
        MiscDebug("Exporter", "threaded access");
        return juce::Result::fail(juce::translate("Invalid threaded access"));
    }

    auto const getEffectiveFile = [&](juce::String const& groupName, juce::String const& trackName)
    {
        if(!file.isDirectory())
        {
            return file.getSiblingFile(filePrefix + file.getFileName());
        }
        if(!trackName.isEmpty())
        {
            return file.getNonexistentChildFile(juce::File::createLegalFileName(groupName) + "_" + filePrefix + juce::File::createLegalFileName(trackName), "." + options.getFormatExtension());
        }
        return file.getNonexistentChildFile(filePrefix + juce::File::createLegalFileName(groupName), "." + options.getFormatExtension());
    };

    if(options.useImageFormat())
    {
        auto const getSizes = [&]()
        {
            if(options.useAutoSize)
            {
                auto const autoBounds = getPlotBounds(identifier);
                auto const outsideBorder = Track::Renderer::getOutsideGridBorder(options.outsideGridJustification);
                auto const bounds = outsideBorder.addedTo(autoBounds);
                auto const scaledBounds = juce::Desktop::getInstance().getDisplays().logicalToPhysical(bounds);
                return std::make_tuple(bounds.getWidth(), bounds.getHeight(), scaledBounds.getWidth(), scaledBounds.getHeight());
            }
            return std::make_tuple(options.imageWidth, options.imageHeight, static_cast<int>(std::round(static_cast<double>(options.imageWidth) * static_cast<double>(options.imagePpi) / 72.0)), static_cast<int>(std::round(static_cast<double>(options.imageHeight) * static_cast<double>(options.imagePpi) / 72.0)));
        };

        if(Tools::hasTrackAcsr(accessor, identifier))
        {
            auto const sizes = getSizes();
            auto& trackAcsr = Tools::getTrackAcsr(accessor, identifier);
            Zoom::Accessor timeZoomAcsr;
            timeZoomAcsr.copyFrom(accessor.getAcsr<AcsrType::timeZoom>(), NotificationType::synchronous);
            if(!timeRange.isEmpty())
            {
                timeZoomAcsr.setAttr<Zoom::AttrType::visibleRange>(timeRange, NotificationType::synchronous);
            }
            auto const& groupAcsr = Tools::getGroupAcsrForTrack(accessor, identifier);
            auto const fileUsed = getEffectiveFile(groupAcsr.getAttr<Group::AttrType::name>(), trackAcsr.getAttr<Track::AttrType::name>());
            lock.exit();

            return Track::Exporter::toImage(trackAcsr, timeZoomAcsr, channels, fileUsed, std::get<0>(sizes), std::get<1>(sizes), std::get<2>(sizes), std::get<3>(sizes), options.outsideGridJustification, shouldAbort);
        }
        else if(Tools::hasGroupAcsr(accessor, identifier))
        {
            auto const sizes = getSizes();
            auto& groupAcsr = Tools::getGroupAcsr(accessor, identifier);
            Zoom::Accessor timeZoomAcsr;
            timeZoomAcsr.copyFrom(accessor.getAcsr<AcsrType::timeZoom>(), NotificationType::synchronous);
            if(!timeRange.isEmpty())
            {
                timeZoomAcsr.setAttr<Zoom::AttrType::visibleRange>(timeRange, NotificationType::synchronous);
            }
            auto const fileUsed = getEffectiveFile(groupAcsr.getAttr<Group::AttrType::name>(), {});
            lock.exit();
            return Group::Exporter::toImage(groupAcsr, timeZoomAcsr, channels, fileUsed, std::get<0>(sizes), std::get<1>(sizes), std::get<2>(sizes), std::get<3>(sizes), options.outsideGridJustification, shouldAbort);
        }
        MiscDebug("Exporter", "Invalid identifier");
        return juce::Result::fail(juce::translate("Invalid identifier"));
    }
    else
    {
        anlStrongAssert(options.useTextFormat());
        if(!options.useTextFormat())
        {
            MiscDebug("Exporter", "Invalid format");
            return juce::Result::fail(juce::translate("Invalid format"));
        }
        auto const documentHasTrack = Tools::hasTrackAcsr(accessor, identifier);
        anlStrongAssert(documentHasTrack);
        if(!documentHasTrack)
        {
            MiscDebug("Exporter", "Invalid track");
            return juce::Result::fail(juce::translate("Invalid track"));
        }
        auto& trackAcsr = Tools::getTrackAcsr(accessor, identifier);
        auto const frameType = Track::Tools::getFrameType(trackAcsr);
        if(!frameType.has_value() || !options.isCompatible(frameType.value()))
        {
            MiscDebug("Exporter", "Invalid format for track");
            return juce::Result::fail(juce::translate("Invalid format for track"));
        }
        auto const& groupAcsr = Tools::getGroupAcsrForTrack(accessor, identifier);
        auto const fileUsed = getEffectiveFile(groupAcsr.getAttr<Group::AttrType::name>(), trackAcsr.getAttr<Track::AttrType::name>());
        lock.exit();

        switch(options.format)
        {
            case Options::Format::jpeg:
                MiscDebug("Exporter", "Unsupported format");
                return juce::Result::fail(juce::translate("Unsupported format"));
            case Options::Format::png:
                MiscDebug("Exporter", "Unsupported format");
                return juce::Result::fail(juce::translate("Unsupported format"));
            case Options::Format::csv:
                return Track::Exporter::toCsv(trackAcsr, timeRange, channels, fileUsed, options.includeHeaderRow, options.getSeparatorChar(), false, options.applyExtraThresholds, "\n", options.disableLabelEscaping, false, shouldAbort);
            case Options::Format::lab:
                return Track::Exporter::toCsv(trackAcsr, timeRange, channels, fileUsed, false, options.getLabSeparatorChar(), true, options.applyExtraThresholds, "\n", options.disableLabelEscaping, false, shouldAbort);
            case Options::Format::puredata:
                return Track::Exporter::toCsv(trackAcsr, timeRange, channels, fileUsed, false, ' ', false, options.applyExtraThresholds, ";", true, false, shouldAbort);
            case Options::Format::max:
                return Track::Exporter::toCsv(trackAcsr, timeRange, channels, fileUsed, false, ' ', false, options.applyExtraThresholds, ";\n", false, true, shouldAbort);
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
        MiscDebug("Exporter", "Unsupported format");
        return juce::Result::fail(juce::translate("Unsupported format"));
    }
}

template <>
void XmlParser::toXml<Document::Exporter::Options>(juce::XmlElement& xml, juce::Identifier const& attributeName, Document::Exporter::Options const& value)
{
    auto child = std::make_unique<juce::XmlElement>(attributeName);
    anlWeakAssert(child != nullptr);
    if(child != nullptr)
    {
        toXml(*child, "format", value.format);
        toXml(*child, "useAutoSize", value.useAutoSize);
        toXml(*child, "imageWidth", value.imageWidth);
        toXml(*child, "imageHeight", value.imageHeight);
        toXml(*child, "imagePpi", value.imagePpi);
        toXml(*child, "includeHeaderRow", value.includeHeaderRow);
        toXml(*child, "applyExtraThresholds", value.applyExtraThresholds);
        toXml(*child, "columnSeparator", value.columnSeparator);
        toXml(*child, "labSeparator", value.labSeparator);
        toXml(*child, "disableLabelEscaping", value.disableLabelEscaping);
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
    value.useAutoSize = fromXml(*child, "useAutoSize", defaultValue.useAutoSize);
    value.imageWidth = fromXml(*child, "imageWidth", defaultValue.imageWidth);
    value.imageHeight = fromXml(*child, "imageHeight", defaultValue.imageHeight);
    value.imagePpi = fromXml(*child, "imagePpi", defaultValue.imagePpi);
    if(child->hasAttribute("includeHeaderRaw")) // For backward compatibility
    {
        value.includeHeaderRow = fromXml(*child, "includeHeaderRaw", defaultValue.includeHeaderRow);
    }
    else
    {
        value.includeHeaderRow = fromXml(*child, "includeHeaderRow", defaultValue.includeHeaderRow);
    }
    value.applyExtraThresholds = fromXml(*child, "applyExtraThresholds", defaultValue.applyExtraThresholds);
    value.columnSeparator = fromXml(*child, "columnSeparator", defaultValue.columnSeparator);
    value.labSeparator = fromXml(*child, "labSeparator", defaultValue.labSeparator);
    value.disableLabelEscaping = fromXml(*child, "disableLabelEscaping", defaultValue.disableLabelEscaping);
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
