#include "AnlDocumentHierarchyManager.h"
#include "../Group/AnlGroupTools.h"
#include "../Track/AnlTrackTools.h"
#include "AnlDocumentDirector.h"
#include "AnlDocumentTools.h"

ANALYSE_FILE_BEGIN

Document::HierarchyManager::HierarchyManager(Accessor& accessor)
: mAccessor(accessor)
{
}

std::vector<Document::HierarchyManager::TrackInfo> Document::HierarchyManager::getAvailableTracksFor(juce::String const& ownerIdentifier, juce::String const& inputIdentifier) const
{
    if(!Tools::hasTrackAcsr(mAccessor, ownerIdentifier))
    {
        return {};
    }
    if(!Track::Tools::supportsInputTrack(Tools::getTrackAcsr(mAccessor, ownerIdentifier), inputIdentifier))
    {
        return {};
    }
    std::vector<TrackInfo> tracks;
    auto const groupLayout = mAccessor.getAttr<AttrType::layout>();
    for(auto const& groupIdentifier : groupLayout)
    {
        if(Tools::hasGroupAcsr(mAccessor, groupIdentifier))
        {
            auto const& groupAcsr = Tools::getGroupAcsr(mAccessor, groupIdentifier);
            auto const trackAcsrs = Group::Tools::getTrackAcsrs(groupAcsr);
            for(auto const& trackAcsr : trackAcsrs)
            {
                auto const identifier = trackAcsr.get().getAttr<Track::AttrType::identifier>();
                if(isTrackValidFor(ownerIdentifier, inputIdentifier, identifier))
                {
                    auto const trackName = trackAcsr.get().getAttr<Track::AttrType::name>();
                    auto const groupName = groupAcsr.getAttr<Group::AttrType::name>();
                    tracks.push_back({identifier, groupName, trackName});
                }
            }
        }
    }
    return tracks;
}

bool Document::HierarchyManager::isTrackValidFor(juce::String const& ownerIdentifier, juce::String const& inputIdentifier, juce::String const& inputTrack) const
{
    if(!Tools::hasTrackAcsr(mAccessor, ownerIdentifier) || !Tools::hasTrackAcsr(mAccessor, inputTrack))
    {
        return false;
    }
    auto const& ownerTrackAcsr = Tools::getTrackAcsr(mAccessor, ownerIdentifier);

    // The inputs of the linked tracks are shared, so the input track must not depend on any of the linked tracks
    std::set<juce::String> ownerIdentifiers{ownerIdentifier};
    auto const isLinked = [](Track::Accessor const& trackAcsr)
    {
        return trackAcsr.getAttr<Track::AttrType::voiceLink>().isNotEmpty() && trackAcsr.getAttr<Track::AttrType::file>().isEmpty();
    };
    if(isLinked(ownerTrackAcsr))
    {
        auto const& voiceLink = ownerTrackAcsr.getAttr<Track::AttrType::voiceLink>();
        for(auto const& trackAcsr : mAccessor.getAcsrs<AcsrType::tracks>())
        {
            if(isLinked(trackAcsr.get()) && trackAcsr.get().getAttr<Track::AttrType::voiceLink>() == voiceLink)
            {
                ownerIdentifiers.insert(trackAcsr.get().getAttr<Track::AttrType::identifier>());
            }
        }
    }

    // The visited tracks prevent infinite recursions if the hierarchy already contains a cycle
    std::set<juce::String> visitedIdentifiers;
    std::function<bool(Track::Accessor const&)> const hasValidInheritance = [&](Track::Accessor const& inputTrackAcsr)
    {
        auto const& trackIdentifier = inputTrackAcsr.getAttr<Track::AttrType::identifier>();
        if(ownerIdentifiers.count(trackIdentifier) > 0_z || !visitedIdentifiers.insert(trackIdentifier).second)
        {
            return false;
        }
        auto const& rootInputs = inputTrackAcsr.getAttr<Track::AttrType::inputs>();
        auto const result = std::none_of(rootInputs.cbegin(), rootInputs.cend(), [&](auto const& rootInput)
                                         {
                                             auto const& prevInputIdentifier = rootInput.second;
                                             return prevInputIdentifier.isNotEmpty() && Tools::hasTrackAcsr(mAccessor, prevInputIdentifier) && !hasValidInheritance(Tools::getTrackAcsr(mAccessor, prevInputIdentifier));
                                         });
        visitedIdentifiers.erase(trackIdentifier);
        return result;
    };

    if(!Track::Tools::supportsInputTrack(ownerTrackAcsr, inputIdentifier))
    {
        return false;
    }
    auto const& inputs = ownerTrackAcsr.getAttr<Track::AttrType::description>().inputs;
    auto const inputIt = std::find_if(inputs.cbegin(), inputs.cend(), [&](auto const input)
                                      {
                                          return juce::String{input.identifier} == inputIdentifier;
                                      });
    if(inputIt == inputs.cend())
    {
        return false;
    }
    auto const expectedFrameType = Track::Tools::getFrameType(*inputIt);
    auto const& inputTrackAcsr = Tools::getTrackAcsr(mAccessor, inputTrack);
    auto const inputFrameType = Track::Tools::getFrameType(inputTrackAcsr);
    if(expectedFrameType.has_value() && (!inputFrameType.has_value() || inputFrameType.value() != expectedFrameType.value()))
    {
        return false;
    }
    return hasValidInheritance(inputTrackAcsr);
}

bool Document::HierarchyManager::hasAccessor(juce::String const& identifier) const
{
    return Tools::hasTrackAcsr(mAccessor, identifier);
}

Track::Accessor const& Document::HierarchyManager::getAccessor(juce::String const& identifier) const
{
    return Tools::getTrackAcsr(mAccessor, identifier);
}

class DocumentHierarchyManagerUnitTest
: public juce::UnitTest
{
public:
    DocumentHierarchyManagerUnitTest()
    : juce::UnitTest("HierarchyManager", "Document")
    {
    }

    ~DocumentHierarchyManagerUnitTest() override = default;

    void runTest() override
    {
        Document::Accessor accessor;
        juce::AudioFormatManager audioFormatManager;
        juce::UndoManager undoManager;
        Document::Director director(accessor, audioFormatManager, undoManager);
        auto const& hierarchyManager = director.getHierarchyManager();

        // The tracks use a description with an input without loading a plugin (no analysis is performed without audio file)
        static auto const inputIdentifier = juce::String("input");
        Plugin::Description description;
        description.inputs.resize(1_z);
        description.inputs.front().identifier = inputIdentifier.toStdString();

        director.startAction();
        auto const groupResult = director.addGroup(0_z, NotificationType::synchronous);
        expect(std::get<0_z>(groupResult).wasOk());
        auto const groupIdentifier = std::get<1_z>(groupResult);

        auto const createTrack = [&](juce::String const& voiceLink)
        {
            auto const trackResult = director.addTrack(groupIdentifier, 0_z, NotificationType::synchronous);
            expect(std::get<0_z>(trackResult).wasOk());
            auto const trackIdentifier = std::get<1_z>(trackResult);
            auto& trackAcsr = Document::Tools::getTrackAcsr(accessor, trackIdentifier);
            trackAcsr.setAttr<Track::AttrType::voiceLink>(voiceLink, NotificationType::synchronous);
            trackAcsr.setAttr<Track::AttrType::description>(description, NotificationType::synchronous);
            trackAcsr.setAttr<Track::AttrType::key>(Plugin::Key{"plugin", "feature"}, NotificationType::synchronous);
            return trackIdentifier;
        };

        auto const setInputTrack = [&](juce::String const& trackIdentifier, juce::String const& inputTrackIdentifier)
        {
            auto& trackAcsr = Document::Tools::getTrackAcsr(accessor, trackIdentifier);
            trackAcsr.setAttr<Track::AttrType::inputs>(std::map<juce::String, juce::String>{{inputIdentifier, inputTrackIdentifier}}, NotificationType::synchronous);
        };

        auto const getAvailableTracks = [&](juce::String const& trackIdentifier)
        {
            std::set<juce::String> identifiers;
            for(auto const& trackInfo : hierarchyManager.getAvailableTracksFor(trackIdentifier, inputIdentifier))
            {
                identifiers.insert(trackInfo.identifier);
            }
            return identifiers;
        };

        auto const linkedTrack1 = createTrack("link");
        auto const linkedTrack2 = createTrack("link");
        auto const dependentTrack = createTrack({});
        auto const independentTrack = createTrack({});
        setInputTrack(dependentTrack, linkedTrack2);

        beginTest("inputs of linked tracks");
        {
            // The inputs are shared by the linked tracks so the input track can't be or depend on any of the linked tracks
            expect(!hierarchyManager.isTrackValidFor(linkedTrack1, inputIdentifier, linkedTrack1));
            expect(!hierarchyManager.isTrackValidFor(linkedTrack1, inputIdentifier, linkedTrack2));
            expect(!hierarchyManager.isTrackValidFor(linkedTrack1, inputIdentifier, dependentTrack));
            expect(hierarchyManager.isTrackValidFor(linkedTrack1, inputIdentifier, independentTrack));
            expect(getAvailableTracks(linkedTrack1) == std::set<juce::String>{independentTrack});
            expect(getAvailableTracks(linkedTrack2) == std::set<juce::String>{independentTrack});
        }

        beginTest("inputs of unlinked tracks");
        {
            expect(hierarchyManager.isTrackValidFor(independentTrack, inputIdentifier, linkedTrack1));
            expect(hierarchyManager.isTrackValidFor(independentTrack, inputIdentifier, dependentTrack));
            expect(!hierarchyManager.isTrackValidFor(linkedTrack2, inputIdentifier, dependentTrack));

            // A track that leaves the link can be used by the other linked tracks
            Document::Tools::getTrackAcsr(accessor, linkedTrack2).setAttr<Track::AttrType::voiceLink>(juce::String{}, NotificationType::synchronous);
            expect(hierarchyManager.isTrackValidFor(linkedTrack1, inputIdentifier, linkedTrack2));
            expect(hierarchyManager.isTrackValidFor(linkedTrack1, inputIdentifier, dependentTrack));
            expect(!hierarchyManager.isTrackValidFor(linkedTrack2, inputIdentifier, dependentTrack));
        }

        beginTest("cyclic dependencies");
        {
            // An existing cycle in the hierarchy doesn't cause an infinite recursion
            auto const cyclicTrack1 = createTrack({});
            auto const cyclicTrack2 = createTrack({});
            setInputTrack(cyclicTrack1, cyclicTrack2);
            setInputTrack(cyclicTrack2, cyclicTrack1);
            expect(!hierarchyManager.isTrackValidFor(linkedTrack1, inputIdentifier, cyclicTrack1));
            expect(getAvailableTracks(linkedTrack1).count(cyclicTrack1) == 0_z);
            expect(getAvailableTracks(linkedTrack1).count(cyclicTrack2) == 0_z);
        }
        director.endAction(ActionState::newTransaction, "Test");
    }
};

static DocumentHierarchyManagerUnitTest documentHierarchyManagerUnitTest;

ANALYSE_FILE_END
