#include "stdafx.h"
#include "StateMachine.h"
#include "Event.h"
#include "State.h"
#include "Context.h"

static log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("state_machine.StateMachine"));

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
	virtual HRESULT handleEvent(Event*, State*, State** nextState) {
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
	HR_ASSERT(!context->currentState, E_ILLEGAL_METHOD_CALL);

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
	Context* context = e->getContext();

	if(logger.isEnabledFor(e->getLogLevel())) {
		// Suppress low level log output.
		LOG4CPLUS_INFO(logger, "Handling " << e->toString() << " in " << context->toString());
	}

	// Recursive call check.
	HR_ASSERT(!context->m_isEventHandling, E_ILLEGAL_METHOD_CALL);
	ScopedStore<bool> _recursive_guard(context->m_isEventHandling, false, true);

	// Lock this scope(If necessary)
	std::unique_ptr<std::lock_guard<std::mutex>> _lock(context->geStatetLock());

	// Current state is contained by Context object in the Event.
	std::shared_ptr<State>& currentState(context->currentState);

	// Call State::handleEvent()
	// If event is ignored and the state is sub state, delegate handling event to master state.
	State* pNextState = nullptr;
	std::shared_ptr<State> nextState;
	bool backToMaster = false;
	HRESULT hr;
	for(State* pCurrentState = currentState.get();
		pCurrentState && !e->isHandled;
		pCurrentState = pCurrentState->masterState().get())
	{
		LOG4CPLUS_INFO(logger, "Calling " << pCurrentState->toString() << "::handleEvent()");
		hr = HR_EXPECT_OK(pCurrentState->handleEvent(e, currentState.get(), &pNextState));
		// Set Event::isHandled.
		// If state transition occurs, assume that the event is handled even if S_EVENT_IGNORED was returned.
		// Setting true by State::handleEvent() is prior to above condition.
		e->isHandled = e->isHandled ? e->isHandled : ((hr != S_EVENT_IGNORED) || pNextState);
		if(pNextState) {
			// Note: Object returned to pNextState might be deleted,
			//       if nextState goes out of scope before it is set as current state.
			std::shared_ptr<State>* nextMasterState = findState(currentState, pNextState);
			if(nextMasterState) {
				// Back to existing master state.
				nextState = *nextMasterState;
				backToMaster = true;
			} else {
				// Exit to newly created state.
				nextState.reset(pNextState);
			}
		}
		if(FAILED(hr)) {
			LOG4CPLUS_INFO(logger, "Calling " << pCurrentState->toString() << "::handleError()");
			HR_ASSERT_OK(pCurrentState->handleError(e, hr));
		}
		if(!e->isHandled) {
			LOG4CPLUS_INFO(logger, "Calling " << pCurrentState->toString() << "::handleIgnoredEvent()");
			HR_ASSERT_OK(pCurrentState->handleIgnoredEvent(e));
		}
	}

	if(SUCCEEDED(hr) && pNextState) {
		LOG4CPLUS_INFO(logger, "Next state is " << pNextState->toString() << (pNextState->isSubState() ? "(Sub state)" : ""));
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
					LOG4CPLUS_DEBUG(logger, "Calling " << state->toString() << "::exit()");
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
			LOG4CPLUS_DEBUG(logger, "Calling " << currentState->toString() << "::entry()");
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
		if(hr != S_OK) return hr;
	}
	return hr;
}

#pragma region Used by unit test.
void StateMachine::setCurrentState(Context* context, State* currentState)
{
	context->currentState.reset(currentState);
}

State* StateMachine::getCurrentState(Context* context) const
{
	return context->currentState.get();
}
#pragma endregion
