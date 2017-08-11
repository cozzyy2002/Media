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

class StateMacineStateUnitTest : public Test
{
public:
	typedef TestStateMachine Testee;

	StateMacineStateUnitTest()
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

TEST_F(StateMacineStateUnitTest, no_transition)
{
	EXPECT_CALL(*currentState, handleEvent(e.get(), currentState, _))
		.WillOnce(Return(S_OK));
	EXPECT_CALL(*currentState, entry(_, _)).Times(0);
	EXPECT_CALL(*currentState, exit(_, _)).Times(0);

	ASSERT_HRESULT_SUCCEEDED(testee.handleEvent(e.get()));

	EXPECT_EQ(currentState, testee.getCurrentState(context.get()));
	EXPECT_FALSE(MockObject::deleted(MockObjectId::CURRENT_STATE));
}

TEST_F(StateMacineStateUnitTest, no_transition_error)
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
}

TEST_F(StateMacineStateUnitTest, transition)
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
}

TEST_F(StateMacineStateUnitTest, transition_error)
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
}

TEST_F(StateMacineStateUnitTest, handleEvent_recursive_call_check)
{
	EXPECT_CALL(*currentState, handleEvent(e.get(), currentState, _))
		.WillOnce(Invoke([this](Event* e, State*, State**)
		{
			EXPECT_TRUE(testee.m_isHandlingState);

			// Call StateMachine::handleEvent() in State::handleEvent().
			EXPECT_EQ(E_ILLEGAL_METHOD_CALL, testee.handleEvent(e));
			return S_OK;
		}));

	ASSERT_HRESULT_SUCCEEDED(testee.handleEvent(e.get()));
	EXPECT_FALSE(testee.m_isHandlingState);

	EXPECT_EQ(currentState, testee.getCurrentState(context.get()));
	EXPECT_FALSE(MockObject::deleted(MockObjectId::CURRENT_STATE));
}

class StateMacineSubStateUnitTest : public StateMacineStateUnitTest
{
public:
	void SetUp() {
		StateMacineStateUnitTest::SetUp();
		masterState1 = new MockState(MockObjectId::MASTER_STATE1);
		masterState2 = new MockState(MockObjectId::MASTER_STATE2);
		currentState->masterState().reset(masterState1);
		masterState1->masterState().reset(masterState2);
		otherState = new MockState(MockObjectId::OTHER_STATE);
	}
	void TearDown() {
		StateMacineStateUnitTest::TearDown();
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
}
