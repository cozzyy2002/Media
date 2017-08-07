#include "stdafx.h"
#include "StateMachine.h"
#include "Event.h"
#include "State.h"

using namespace state_machine;

StateMachine::StateMachine()
{
}


StateMachine::~StateMachine()
{
}

HRESULT state_machine::StateMachine::handleEvent(const Event* e)
{
	// Call State::handleEvent()
	// If event is ignored and the state is sub state, delegate handling event to master state.
	State* pCurrentState = m_currentState.get();
	State* pNextState = nullptr;
	std::shared_ptr<State> nextState;
	HRESULT hr;
	do {
		hr = HR_EXPECT_OK(pCurrentState->handleEvent(e, pCurrentState, &pNextState));
		std::shared_ptr<State>* _nextState = findState(pNextState);
		if(_nextState) {
			// Back to existing master state.
			nextState = *_nextState;
		} else {
			// Exit to newly created state.
			nextState.reset(pNextState);
		}
		if(FAILED(hr)) {
			HR_ASSERT_OK(pCurrentState->handleError(e, hr));
		}
		if(S_EVENT_IGNORED == hr) hr = HR_EXPECT_OK(pCurrentState->handleIgnoredEvent(e));
		pCurrentState = pCurrentState->masterState().get();
	} while(pCurrentState && (S_EVENT_IGNORED == hr));

	if(SUCCEEDED(hr) && pNextState) {
		// State transition occurred.
		if(pNextState->isSubState() && !pNextState->isEntryCalled()) {
			// Transition from master state to sub state.
			// Don't call exit() of master state.
			pNextState->masterState() = m_currentState;
		} else {
			// Transition to other state or master state of current state.
			// Call exit() of current state and master state if any.
			HR_ASSERT_OK(for_each_state([e, pNextState](std::shared_ptr<State>& state)
			{
				if(state.get() != pNextState) {
					HR_ASSERT_OK(state->exit(e, pNextState));
					return S_OK;
				} else {
					return S_FALSE;
				}
			}));
		}
		// Preserve current state as previous state until calling entry() of next stete.
		// Note: Current state and it's master state(which is not next state) will be deleted on out of scope.
		std::shared_ptr<State> previousState(m_currentState);
		m_currentState = nextState;
		if(!m_currentState->isEntryCalled()) {
			m_currentState->setEntryCalled(true);
			HR_ASSERT_OK(m_currentState->entry(e, previousState.get()));
		}
	}
	return hr;
}

std::shared_ptr<State>* state_machine::StateMachine::findState(State* pState)
{
	std::shared_ptr<State>* ret = nullptr;
	for_each_state([this, pState, &ret](std::shared_ptr<State>& state)
	{
		if(pState == state.get()) {
			ret = &state;
			return S_FALSE;
		}
		return S_OK;
	});
	return ret;
}

HRESULT state_machine::StateMachine::for_each_state(std::function<HRESULT(std::shared_ptr<State>& state)> func)
{
	HRESULT hr;
	for(std::shared_ptr<State>* state(&m_currentState); state->get(); state = &(state->get()->masterState())) {
		hr = func(*state);
		switch(hr) {
		case S_OK:
			break;
		case S_FALSE:
		default:
			return hr;
		}
	}
	return hr;
}
