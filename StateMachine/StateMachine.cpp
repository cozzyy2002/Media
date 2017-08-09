#include "stdafx.h"
#include "StateMachine.h"
#include "Event.h"
#include "State.h"
#include "Context.h"

using namespace state_machine;

/*
	Inner class to let state machine call userState->entry().
	Used to initialize user context.
*/
class RootState : public State
{
public:
	RootState(State* userState) : userState(userState) {}

	// Returns user state as next state regardless of the event.
	// Then state machine calls entry() of user state and sets user state as current state.
	virtual HRESULT handleEvent(const Event*, const State*, State** nextState) {
		*nextState = userState;
		return S_OK;
	}

protected:
	State* userState;
};

StateMachine::StateMachine()
{
}

StateMachine::~StateMachine()
{
}

/*
	Start event handling using the context.

	User state initialState->entry() will be called.
	The method should ignore Event parameter if userEvent is not specified.
	The method should ignore State parameter.
*/
HRESULT StateMachine::start(Context * context, State * initialState, Event* userEvent /*= nullptr*/)
{
	context->currentState.reset(new RootState(initialState));
	Event* e = userEvent ? userEvent : new Event();
	return context->handleEvent(e);
}

HRESULT StateMachine::stop(Context* context)
{
	context->currentState.reset();
	return S_OK;
}

HRESULT StateMachine::handleEvent(Event* e)
{
	Context* context = e->context;

	// Lock this scope(If necessary)
	std::unique_ptr<std::lock_guard<std::mutex>> _lock(context->geStatetLock());

	// Current state is contained by Context object in the Event.
	std::shared_ptr<State>& currentState(context->currentState);

	// Call State::handleEvent()
	// If event is ignored and the state is sub state, delegate handling event to master state.
	State* pCurrentState = currentState.get();
	State* pNextState = nullptr;
	std::shared_ptr<State> nextState;
	bool backToMaster = false;
	HRESULT hr;
	do {
		hr = HR_EXPECT_OK(pCurrentState->handleEvent(e, pCurrentState, &pNextState));
		if(pNextState) {
			std::shared_ptr<State>* _nextState = findState(currentState, pNextState);
			if(_nextState) {
				// Back to existing master state.
				nextState = *_nextState;
				backToMaster = true;
			} else {
				// Exit to newly created state.
				nextState.reset(pNextState);
			}
		}
		if(FAILED(hr)) {
			HR_ASSERT_OK(pCurrentState->handleError(e, hr));
		}
		if(S_EVENT_IGNORED == hr) {
			hr = HR_EXPECT_OK(pCurrentState->handleIgnoredEvent(e));
			if(FAILED(hr)) return hr;
		}
		pCurrentState = pCurrentState->masterState().get();
	} while(pCurrentState && (S_EVENT_IGNORED == hr));

	if(SUCCEEDED(hr) && pNextState) {
		// State transition occurred.
		if(pNextState->isSubState() && !backToMaster) {
			// Transition from master state to sub state.
			// Don't call exit() of master state.
			pNextState->masterState() = currentState;
		} else {
			// Transition to other state or master state of current state.
			// Call exit() of current state and master state if any.
			HR_ASSERT_OK(for_each_state(currentState, [e, pNextState](std::shared_ptr<State>& state)
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
		// Note: Current state and it's master state(which is not next state) will be deleted when current state is updated.
		std::shared_ptr<State> previousState(currentState);
		currentState = nextState;
		if(!backToMaster) {
			HR_ASSERT_OK(currentState->entry(e, previousState.get()));
		}
	}
	return hr;
}

std::shared_ptr<State>* StateMachine::findState(std::shared_ptr<State>& currentState, State* pState)
{
	std::shared_ptr<State>* ret = nullptr;
	for_each_state(currentState, [this, pState, &ret](std::shared_ptr<State>& state)
	{
		if(pState == state.get()) {
			ret = &state;
			return S_FALSE;
		}
		return S_OK;
	});
	return ret;
}

HRESULT StateMachine::for_each_state(std::shared_ptr<State>& currentState, std::function<HRESULT(std::shared_ptr<State>& state)> func)
{
	HRESULT hr;
	for(std::shared_ptr<State>* state(&currentState); state->get(); state = &(state->get()->masterState())) {
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

#pragma region Used by unit test.
void StateMachine::setCurrentState(Context * context, State * currentState)
{
	context->currentState.reset(currentState);
}

State * StateMachine::getCurrentState(Context * context) const
{
	return context->currentState.get();
}
#pragma endregion
