#include "stdafx.h"
#include "TestUtils/Mocks.h"
#include "TestUtils/TestStateMachine.h"

#include <StateMachine/Context.h>
#include <StateMachine/StateMachine.h>

using namespace state_machine;
using namespace testing;

class TestContext : public Context
{
public:
	TestContext(StateMachine* stateMachine) : Context(stateMachine) {}
};

class StateMacineUnitTest : public Test
{
public:
	typedef TestStateMachine Testee;

	StateMacineUnitTest()
		: context(new TestContext(&testee))
		, e(new MockEvent(context.get())) {}

	void SetUp() {
		currentState = new MockState(MockObjectId::CURRENT_STATE);
		testee.setCurrentState(context.get(), currentState);
	}
	void TearDown() {
		context.reset();
	}

	std::unique_ptr<TestContext> context;
	Testee testee;
	MockState* currentState;
	std::unique_ptr<MockEvent> e;
};

TEST_F(StateMacineUnitTest, no_transition)
{
	EXPECT_CALL(*currentState, handleEvent(e.get(), currentState, _))
		.WillOnce(Return(S_OK));
	EXPECT_CALL(*currentState, entry(_, _)).Times(0);
	EXPECT_CALL(*currentState, exit(_, _)).Times(0);

	ASSERT_HRESULT_SUCCEEDED(testee.handleEvent(e.get()));

	EXPECT_EQ(currentState, testee.getCurrentState(context.get()));
	EXPECT_FALSE(MockObject::deleted(MockObjectId::CURRENT_STATE));
	EXPECT_TRUE(e->isHandled);
}

TEST_F(StateMacineUnitTest, no_transition_error)
{
	EXPECT_CALL(*currentState, handleEvent(e.get(), currentState, _))
		.WillOnce(Return(E_NOTIMPL));
	EXPECT_CALL(*currentState, handleError(e.get(), E_NOTIMPL))
		.WillOnce(Invoke([this](Event* e, HRESULT hr)
		{
			// Call default error handler.
			 return currentState->State::handleError(e, hr);
		}));
	EXPECT_CALL(*currentState, entry(_, _)).Times(0);
	EXPECT_CALL(*currentState, exit(_, _)).Times(0);

	ASSERT_EQ(E_NOTIMPL, testee.handleEvent(e.get()));

	EXPECT_EQ(currentState, testee.getCurrentState(context.get()));
	EXPECT_FALSE(MockObject::deleted(MockObjectId::CURRENT_STATE));
	EXPECT_TRUE(e->isHandled);
}

TEST_F(StateMacineUnitTest, transition)
{
	MockState* nextState = new MockState(MockObjectId::NEXT_STATE);
	EXPECT_CALL(*currentState, handleEvent(e.get(), currentState, _))
		.WillOnce(DoAll(SetArgPointee<2>(nextState), Return(S_OK)));
	EXPECT_CALL(*currentState, entry(_, _)).Times(0);
	EXPECT_CALL(*currentState, exit(e.get(), nextState)).Times(1);
	EXPECT_CALL(*nextState, entry(e.get(), currentState)).Times(1);
	EXPECT_CALL(*nextState, exit(_, _)).Times(0);

	ASSERT_HRESULT_SUCCEEDED(testee.handleEvent(e.get()));

	EXPECT_EQ(nextState, testee.getCurrentState(context.get()));
	EXPECT_TRUE(MockObject::deleted(MockObjectId::CURRENT_STATE));
	EXPECT_FALSE(MockObject::deleted(MockObjectId::NEXT_STATE));
	EXPECT_TRUE(e->isHandled);
}

TEST_F(StateMacineUnitTest, transition_error)
{
	MockState* nextState = new MockState(MockObjectId::NEXT_STATE);
	EXPECT_CALL(*currentState, handleEvent(e.get(), currentState, _))
		.WillOnce(DoAll(SetArgPointee<2>(nextState), Return(E_NOTIMPL)));
	EXPECT_CALL(*currentState, handleError(e.get(), E_NOTIMPL))
		.WillOnce(Invoke([this](Event* e, HRESULT hr)
		{
			// Call default error handler.
			 return currentState->State::handleError(e, hr);
		}));
	EXPECT_CALL(*currentState, entry(_, _)).Times(0);
	EXPECT_CALL(*currentState, exit(e.get(), nextState)).Times(0);
	EXPECT_CALL(*nextState, entry(e.get(), currentState)).Times(0);
	EXPECT_CALL(*nextState, exit(_, _)).Times(0);

	ASSERT_EQ(E_NOTIMPL, testee.handleEvent(e.get()));

	EXPECT_EQ(currentState, testee.getCurrentState(context.get()));
	EXPECT_FALSE(MockObject::deleted(MockObjectId::CURRENT_STATE));
	EXPECT_TRUE(MockObject::deleted(MockObjectId::NEXT_STATE));
	EXPECT_TRUE(e->isHandled);
}

class StateMacineSubStateUnitTest : public StateMacineUnitTest
{
public:
	void SetUp() {
		StateMacineUnitTest::SetUp();
		masterState1 = new MockState(MockObjectId::MASTER_STATE1);
		masterState2 = new MockState(MockObjectId::MASTER_STATE2);
		currentState->masterState().reset(masterState1);
		masterState1->masterState().reset(masterState2);
		otherState = new MockState(MockObjectId::OTHER_STATE);
	}
	void TearDown() {
		StateMacineUnitTest::TearDown();
	}

	MockState* masterState1;	// Master of sub state
	MockState* masterState2;	// Master of master1 state
	MockState* otherState;
};

TEST_F(StateMacineSubStateUnitTest, back_to_parent1)
{
	EXPECT_CALL(*currentState, handleEvent(e.get(), currentState, _))
		.WillOnce(DoAll(SetArgPointee<2>(currentState->backToMaster()), Return(S_OK)));
	EXPECT_CALL(*currentState, entry(_, _)).Times(0);
	EXPECT_CALL(*currentState, exit(_, _)).Times(1);
	EXPECT_CALL(*masterState1, entry(_, _)).Times(0);
	EXPECT_CALL(*masterState1, exit(_, _)).Times(0);
	EXPECT_CALL(*masterState2, entry(_, _)).Times(0);
	EXPECT_CALL(*masterState2, exit(_, _)).Times(0);

	ASSERT_HRESULT_SUCCEEDED(testee.handleEvent(e.get()));

	EXPECT_EQ(masterState1, testee.getCurrentState(context.get()));
	EXPECT_TRUE(MockObject::deleted(MockObjectId::CURRENT_STATE));
	EXPECT_FALSE(MockObject::deleted(MockObjectId::MASTER_STATE1));
	EXPECT_FALSE(MockObject::deleted(MockObjectId::MASTER_STATE2));
	EXPECT_TRUE(e->isHandled);
}

TEST_F(StateMacineSubStateUnitTest, back_to_parent2)
{
	EXPECT_CALL(*currentState, handleEvent(e.get(), currentState, _))
		.WillOnce(DoAll(SetArgPointee<2>(masterState2), Return(S_OK)));
	EXPECT_CALL(*currentState, entry(_, _)).Times(0);
	EXPECT_CALL(*currentState, exit(_, _)).Times(1);
	EXPECT_CALL(*masterState1, entry(_, _)).Times(0);
	EXPECT_CALL(*masterState1, exit(_, _)).Times(1);
	EXPECT_CALL(*masterState2, entry(_, _)).Times(0);
	EXPECT_CALL(*masterState2, exit(_, _)).Times(0);

	ASSERT_HRESULT_SUCCEEDED(testee.handleEvent(e.get()));

	EXPECT_EQ(masterState2, testee.getCurrentState(context.get()));
	EXPECT_TRUE(MockObject::deleted(MockObjectId::CURRENT_STATE));
	EXPECT_TRUE(MockObject::deleted(MockObjectId::MASTER_STATE1));
	EXPECT_FALSE(MockObject::deleted(MockObjectId::MASTER_STATE2));
	EXPECT_TRUE(e->isHandled);
}

TEST_F(StateMacineSubStateUnitTest, event_ignored)
{
	EXPECT_CALL(*currentState, handleEvent(e.get(), currentState, _))
		.WillOnce(Return(currentState->eventIsIgnored()));
	EXPECT_CALL(*currentState, handleIgnoredEvent(e.get()))
		.WillOnce(Return(currentState->eventIsIgnored()));
	EXPECT_CALL(*currentState, entry(_, _)).Times(0);
	EXPECT_CALL(*currentState, exit(_, _)).Times(0);
	EXPECT_CALL(*masterState1, handleEvent(e.get(), currentState, _))
		.WillOnce(Return(masterState1->eventIsIgnored()));
	EXPECT_CALL(*masterState1, handleIgnoredEvent(e.get()))
		.WillOnce(Return(masterState1->eventIsIgnored()));
	EXPECT_CALL(*masterState1, entry(_, _)).Times(0);
	EXPECT_CALL(*masterState1, exit(_, _)).Times(0);
	EXPECT_CALL(*masterState2, handleEvent(e.get(), currentState, _))
		.WillOnce(Return(masterState2->eventIsIgnored()));
	EXPECT_CALL(*masterState2, handleIgnoredEvent(e.get()))
		.WillOnce(Return(masterState2->eventIsIgnored()));
	EXPECT_CALL(*masterState2, entry(_, _)).Times(0);
	EXPECT_CALL(*masterState2, exit(_, _)).Times(0);

	ASSERT_HRESULT_SUCCEEDED(testee.handleEvent(e.get()));

	EXPECT_EQ(currentState, testee.getCurrentState(context.get()));
	EXPECT_FALSE(MockObject::deleted(MockObjectId::CURRENT_STATE));
	EXPECT_FALSE(MockObject::deleted(MockObjectId::MASTER_STATE1));
	EXPECT_FALSE(MockObject::deleted(MockObjectId::MASTER_STATE2));
	EXPECT_FALSE(e->isHandled);
}
