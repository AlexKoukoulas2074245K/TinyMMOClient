///------------------------------------------------------------------------------------------------
///  StringUtilsTest.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 19/09/2023
///------------------------------------------------------------------------------------------------

#include <gtest/gtest.h>
#include <game/events/EventSystem.h>

///------------------------------------------------------------------------------------------------

class TestEvent final
{
public:
    TestEvent(int val) : mVal(val) {}
    int GetVal() const { return mVal; }
    
private:
    int mVal = 0;
};

class TestEvent2 final
{
public:
    TestEvent2(int val) : mVal(val) {}
    int GetVal() const { return mVal; }
    
private:
    int mVal = 0;
};

///------------------------------------------------------------------------------------------------

class TestEventListener final: public events::IListener
{
public:
    void OnTestEvent(const TestEvent& event)
    {
        mVal = event.GetVal();
    }
    int GetVal() const { return mVal; }
private:
    int mVal = 0;
};

///------------------------------------------------------------------------------------------------

TEST(EventSystemTests, TestUnregistrationFromEventFollowedByReRegistrationTriggersCallbackForSubsequentDispatches)
{
    TestEventListener listener;
    events::EventSystem::GetInstance().RegisterForEvent<TestEvent>(&listener, &TestEventListener::OnTestEvent);
    events::EventSystem::GetInstance().DispatchEvent<TestEvent>(1);
    EXPECT_EQ(listener.GetVal(), 1);
    
    events::EventSystem::GetInstance().UnregisterForEvent<TestEvent>(&listener);
    events::EventSystem::GetInstance().RegisterForEvent<TestEvent>(&listener, &TestEventListener::OnTestEvent);
    events::EventSystem::GetInstance().DispatchEvent<TestEvent>(3);
    EXPECT_EQ(listener.GetVal(), 3);
}

///------------------------------------------------------------------------------------------------

TEST(EventSystemTests, TestMultipleEventDispatchesTriggerCallback)
{
    TestEventListener listener;
    events::EventSystem::GetInstance().RegisterForEvent<TestEvent>(&listener, &TestEventListener::OnTestEvent);
    events::EventSystem::GetInstance().DispatchEvent<TestEvent>(1);
    EXPECT_EQ(listener.GetVal(), 1);
    
    events::EventSystem::GetInstance().DispatchEvent<TestEvent>(2);
    EXPECT_EQ(listener.GetVal(), 2);
}

///------------------------------------------------------------------------------------------------

TEST(EventSystemTests, TestUnregistrationFromEventDoesNotTriggerCallbackForSubsequentDispatches)
{
    TestEventListener listener;
    events::EventSystem::GetInstance().RegisterForEvent<TestEvent>(&listener, &TestEventListener::OnTestEvent);
    events::EventSystem::GetInstance().DispatchEvent<TestEvent>(1);
    EXPECT_EQ(listener.GetVal(), 1);
    
    events::EventSystem::GetInstance().UnregisterForEvent<TestEvent>(&listener);
    events::EventSystem::GetInstance().DispatchEvent<TestEvent>(2);
    EXPECT_EQ(listener.GetVal(), 1);
}

///------------------------------------------------------------------------------------------------

TEST(EventSystemTests, TestListenerDeallocationDoesNotTriggerCallbackForSubsequentDispatches)
{
    static int sEventsListenedTo = 0;
    class NotSoLongLivedTestEventListener final: public events::IListener
    {
    public:
        NotSoLongLivedTestEventListener()
        {
            events::EventSystem::GetInstance().RegisterForEvent<TestEvent>(this, &NotSoLongLivedTestEventListener::OnTestEvent);
        }
        
    private:
        void OnTestEvent(const TestEvent&)
        {
            sEventsListenedTo++;
        }
    };
    
    {
        NotSoLongLivedTestEventListener listener;
        
        events::EventSystem::GetInstance().DispatchEvent<TestEvent>(1);
        EXPECT_EQ(sEventsListenedTo, 1);
    }
    
    events::EventSystem::GetInstance().DispatchEvent<TestEvent>(1);
    EXPECT_EQ(sEventsListenedTo, 1);
}

///------------------------------------------------------------------------------------------------

TEST(EventSystemTests, TestListenerDeallocationDoesNotTriggerCallbackForSubsequentDispatchesOfAllRegisteredEvents)
{
    static int sEvents1ListenedTo = 0;
    static int sEvents2ListenedTo = 0;
    
    class NotSoLongLivedTestEventListener final: public events::IListener
    {
    public:
        NotSoLongLivedTestEventListener()
        {
            events::EventSystem::GetInstance().RegisterForEvent<TestEvent>(this, &NotSoLongLivedTestEventListener::OnTestEvent1);
            events::EventSystem::GetInstance().RegisterForEvent<TestEvent2>(this, &NotSoLongLivedTestEventListener::OnTestEvent2);
        }
        
    private:
        void OnTestEvent1(const TestEvent&)
        {
            sEvents1ListenedTo++;
        }
        
        void OnTestEvent2(const TestEvent2&)
        {
            sEvents2ListenedTo++;
        }
    };
    
    {
        NotSoLongLivedTestEventListener listener;
        
        events::EventSystem::GetInstance().DispatchEvent<TestEvent>(1);
        EXPECT_EQ(sEvents1ListenedTo, 1);
        EXPECT_EQ(sEvents2ListenedTo, 0);
        
        events::EventSystem::GetInstance().DispatchEvent<TestEvent2>(1);
        EXPECT_EQ(sEvents1ListenedTo, 1);
        EXPECT_EQ(sEvents2ListenedTo, 1);
    }
    
    events::EventSystem::GetInstance().DispatchEvent<TestEvent>(1);
    events::EventSystem::GetInstance().DispatchEvent<TestEvent2>(1);
    
    EXPECT_EQ(sEvents1ListenedTo, 1);
    EXPECT_EQ(sEvents2ListenedTo, 1);
}

///------------------------------------------------------------------------------------------------

TEST(EventSystemTests, TestEventRegistrationWithLambda)
{
    class NotSoLongLivedEvent {};
    
    class NotSoLongLivedTestEventListenerWithLambda final: public events::IListener
    {
    public:
        void OnTestEvent(const NotSoLongLivedEvent&)
        {
            mEventsListenedTo++;
        }
        
        int mEventsListenedTo = 0;
    };
    
    NotSoLongLivedTestEventListenerWithLambda listener;
    {
        auto listenerHandle = events::EventSystem::GetInstance().RegisterForEvent<NotSoLongLivedEvent>([&](const NotSoLongLivedEvent& e){listener.OnTestEvent(e); });
        events::EventSystem::GetInstance().DispatchEvent<NotSoLongLivedEvent>();
    }
    
    events::EventSystem::GetInstance().DispatchEvent<NotSoLongLivedEvent>();
    EXPECT_EQ(listener.mEventsListenedTo, 1);
}

///------------------------------------------------------------------------------------------------

TEST(EventSystemTests, TestDoubleEventRegistrationIsNoOp)
{
    static int sEventsListenedTo = 0;
    
    class MultiRegistrationEvent {};
    
    class TestEventListenerMultiple final: public events::IListener
    {
    public:
        void OnTestEvent(const MultiRegistrationEvent&)
        {
            sEventsListenedTo++;
        }
    };
    
    TestEventListenerMultiple listener;
    events::EventSystem::GetInstance().RegisterForEvent<MultiRegistrationEvent>(&listener, &TestEventListenerMultiple::OnTestEvent);
    events::EventSystem::GetInstance().RegisterForEvent<MultiRegistrationEvent>(&listener, &TestEventListenerMultiple::OnTestEvent);
    
    events::EventSystem::GetInstance().DispatchEvent<MultiRegistrationEvent>();
    
    EXPECT_EQ(sEventsListenedTo, 1);
}

///------------------------------------------------------------------------------------------------

TEST(EventSystemTests, TestListenerUnregistrationFollowedByReregistrationTriggerCallbackOnce)
{
    static int sEventsListenedTo = 0;
    
    class TestDeallocationFollowedByReregistrationEvent {};
    
    class TestDeallocationFollowedByReregistrationListener final: public events::IListener
    {
    public:
        void OnTestEvent(const TestDeallocationFollowedByReregistrationEvent&)
        {
            sEventsListenedTo++;
        }
    };
    
    TestDeallocationFollowedByReregistrationListener listener;
    events::EventSystem::GetInstance().RegisterForEvent<TestDeallocationFollowedByReregistrationEvent>(&listener, &TestDeallocationFollowedByReregistrationListener::OnTestEvent);
    
    events::EventSystem::GetInstance().UnregisterAllEventsForListener(&listener);
    events::EventSystem::GetInstance().RegisterForEvent<TestDeallocationFollowedByReregistrationEvent>(&listener, &TestDeallocationFollowedByReregistrationListener::OnTestEvent);
    
    events::EventSystem::GetInstance().UnregisterAllEventsForListener(&listener);
    events::EventSystem::GetInstance().RegisterForEvent<TestDeallocationFollowedByReregistrationEvent>(&listener, &TestDeallocationFollowedByReregistrationListener::OnTestEvent);
    
    events::EventSystem::GetInstance().DispatchEvent<TestDeallocationFollowedByReregistrationEvent>();
    
    EXPECT_EQ(sEventsListenedTo, 1);
}

///------------------------------------------------------------------------------------------------

TEST(EventSystemTests, TestDoubleListenerDeathDoesNotYieldAnyEventCallbacksForEither)
{
    static int sEventsListenedToByListenerA = 0;
    static int sEventsListenedToByListenerB = 0;
    
    class TestDeallocationEvent {};
    
    class TestDeallocationListenerA final: public events::IListener
    {
    public:
        void OnTestEvent(const TestDeallocationEvent&)
        {
            sEventsListenedToByListenerA++;
        }
    };
    
    class TestDeallocationListenerB final: public events::IListener
    {
    public:
        void OnTestEvent(const TestDeallocationEvent&)
        {
            sEventsListenedToByListenerB++;
        }
    };
    
    {
        TestDeallocationListenerA listenerA;
        events::EventSystem::GetInstance().RegisterForEvent<TestDeallocationEvent>(&listenerA, &TestDeallocationListenerA::OnTestEvent);
        
        TestDeallocationListenerB listenerB;
        events::EventSystem::GetInstance().RegisterForEvent<TestDeallocationEvent>(&listenerB, &TestDeallocationListenerB::OnTestEvent);
        
        events::EventSystem::GetInstance().DispatchEvent<TestDeallocationEvent>();
        
        EXPECT_EQ(sEventsListenedToByListenerA, 1);
        EXPECT_EQ(sEventsListenedToByListenerB, 1);
    }
    
    events::EventSystem::GetInstance().DispatchEvent<TestDeallocationEvent>();
    
    EXPECT_EQ(sEventsListenedToByListenerA, 1);
    EXPECT_EQ(sEventsListenedToByListenerB, 1);
}

///------------------------------------------------------------------------------------------------

TEST(EventSystemTests, TestDoubleListenerRegistrationUnregistrationRegistration)
{
    static int sEventsListenedToByListenerA = 0;
    static int sEventsListenedToByListenerB = 0;
    
    class TestDeallocationEvent {};
    
    class TestDeallocationListenerA final: public events::IListener
    {
    public:
        void OnTestEvent(const TestDeallocationEvent&)
        {
            sEventsListenedToByListenerA++;
        }
    };
    
    class TestDeallocationListenerB final: public events::IListener
    {
    public:
        void OnTestEvent(const TestDeallocationEvent&)
        {
            sEventsListenedToByListenerB++;
        }
    };
    
    TestDeallocationListenerA listenerA;
    events::EventSystem::GetInstance().RegisterForEvent<TestDeallocationEvent>(&listenerA, &TestDeallocationListenerA::OnTestEvent);
    
    TestDeallocationListenerB listenerB;
    events::EventSystem::GetInstance().RegisterForEvent<TestDeallocationEvent>(&listenerB, &TestDeallocationListenerB::OnTestEvent);
    
    events::EventSystem::GetInstance().DispatchEvent<TestDeallocationEvent>();
    
    EXPECT_EQ(sEventsListenedToByListenerA, 1);
    EXPECT_EQ(sEventsListenedToByListenerB, 1);
    
    events::EventSystem::GetInstance().UnregisterAllEventsForListener(&listenerA);
    events::EventSystem::GetInstance().UnregisterAllEventsForListener(&listenerB);
    
    events::EventSystem::GetInstance().RegisterForEvent<TestDeallocationEvent>(&listenerA, &TestDeallocationListenerA::OnTestEvent);
    events::EventSystem::GetInstance().RegisterForEvent<TestDeallocationEvent>(&listenerB, &TestDeallocationListenerB::OnTestEvent);
    
    events::EventSystem::GetInstance().DispatchEvent<TestDeallocationEvent>();
    
    EXPECT_EQ(sEventsListenedToByListenerA, 2);
    EXPECT_EQ(sEventsListenedToByListenerB, 2);
}

///------------------------------------------------------------------------------------------------
