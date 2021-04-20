#include "AnlAtomicManager.h"

ANALYSE_FILE_BEGIN

class AtomicManagerUnitTest
: public juce::UnitTest
{
public:
    AtomicManagerUnitTest()
    : juce::UnitTest("AtomicManager", "Tools")
    {
    }

    ~AtomicManagerUnitTest() override = default;

    void runTest() override
    {
        // clang-format off
        enum class State : bool
        {
              Active = false
            , Deleted = true
        };
        // clang-format on

        class Deleter
        {
        public:
            Deleter(State& d)
            : state(d)
            {
                anlStrongAssert(state == State::Active);
            }

            ~Deleter()
            {
                state = State::Deleted;
            }

            State& state;
        };

        AtomicManager<Deleter> mgmt;

        beginTest("initialization");
        {
            expect(mgmt.getInstance() == nullptr);
        }

        beginTest("single access");
        {
            State state(State::Active);

            mgmt.setInstance(std::make_shared<Deleter>(state));
            expect(mgmt.getInstance() != nullptr);
            expect(state == State::Active);

            mgmt.setInstance(nullptr);
            expect(mgmt.getInstance() == nullptr);
            expect(state == State::Deleted);
        }

        beginTest("shared access - single thread");
        {
            State state1(State::Active);
            auto value1 = std::make_shared<Deleter>(state1);

            mgmt.setInstance(value1);
            expect(mgmt.getInstance() == value1);
            expect(value1.use_count() == 2);
            expect(state1 == State::Active);

            // The local shared pointer in reset before the manager so the
            // manager can directly release its internal shared pointer
            value1.reset();
            mgmt.setInstance(nullptr);
            expect(mgmt.getInstance() == nullptr);
            expect(state1 == State::Deleted);

            State state2(State::Active);
            auto value2 = std::make_shared<Deleter>(state2);
            mgmt.setInstance(value2);
            expect(mgmt.getInstance() == value2);
            expect(value2.use_count() == 2);
            expect(state2 == State::Active);

            // The local shared pointer in reset after the manager so the
            // manager waits until is has the full ownership
            mgmt.setInstance(nullptr);
            expect(mgmt.getInstance() == nullptr);
            expect(value2.use_count() == 2);
            expect(state2 == State::Active);
            value2.reset();
            // One message tick is neccessary
            if(juce::MessageManager::getInstance()->runDispatchLoopUntil(100))
            {
                expect(state2 == State::Deleted);
            }
        }

        beginTest("shared access - double thread");
        {
            State state(State::Active);
            mgmt.setInstance(std::make_shared<Deleter>(state));
            expect(mgmt.getInstance() != nullptr);
            expect(state == State::Active);

            std::atomic<bool> run = false;
            std::thread thd([&]()
                            {
                                auto instance = mgmt.getInstance();
                                run = true;
                                expect(instance != nullptr);
                                expect(state == State::Active);
                                while(run)
                                {
                                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                                }
                                juce::ignoreUnused(run);
                            });
            thd.detach();
            while(!run)
            {
            }

            expect(state == State::Active);

            mgmt.setInstance(nullptr);
            expect(mgmt.getInstance() == nullptr);
            expect(state == State::Active);
            run = false;
            if(thd.joinable())
            {
                thd.join();
            }

            if(juce::MessageManager::getInstance()->runDispatchLoopUntil(100))
            {
                expect(state == State::Deleted);
            }
        }
    }
};

static AtomicManagerUnitTest atomicManagerUnitTest;

ANALYSE_FILE_END
