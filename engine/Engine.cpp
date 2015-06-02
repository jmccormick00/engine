/*=========================================================================
/ James McCormick - Engine.cpp
/ A class for driving the core of the program
/==========================================================================*/

#include <algorithm>
#include "Engine.h"
#ifdef CONSOL
#include "Console.h"
#endif

namespace engine {

//========================================================================
// C_Engine implemenation
//========================================================================


//-----------------------------------------------------------------------
// addListener - Public Engine
// Description 
//		Pairs a message type to a message listener.
//
// Arguments:	MsgListenerPtr - A pointer to a message listener
//				MessageType - the message type to pair the listener to.
// Returns:		true if successful, false otherwise
//-----------------------------------------------------------------------
bool Engine::addListener(MsgListenerPtr l, const MessageType& type) {
	// Check to see if one exists first
	MessageListenerMap::iterator listenerMapItr = listener_map_.find(type);
	if(listenerMapItr == listener_map_.end()) {
		MessageListenerMapIResult insertResult = listener_map_.insert(
			MessageListenerMapPair(type, ListenerList()));

		// Pair insertion failed!!
		if(!insertResult.second)
			return false;

		// Somehow created an empty table?
		if(insertResult.first == listener_map_.end())
			return false;
		
		// replace the listenerList iterator so it can be updated
		listenerMapItr = insertResult.first;
	}


	// Loop through the list to make sure no duplicates exist
	ListenerList & lList = (*listenerMapItr).second;
	for(ListenerList::iterator it = lList.begin(), itEnd = lList.end(); it != itEnd; ++it) {
		if(*it == l) 
			return false;
	}

	// Everything checks out, add the listener
	lList.push_back(l);
	return true;
}


//-----------------------------------------------------------------------
// addWildCardListener - Public Engine
// Description 
//		Adds a listener as a wild card.
//
// Arguments:	MsgListenerPtr - A pointer to a message listener
// Returns:		true if successful, false otherwise
//-----------------------------------------------------------------------
bool Engine::addWildCardListener(MsgListenerPtr l) {
	// First check to make sure this doesnt already exist
	ListenerList::iterator i = std::find(wildcard_listener_list_.begin(), wildcard_listener_list_.end(), l);

	// This listener is already in the list
	if (i != wildcard_listener_list_.end())
		return false;

	// Add it to the list.
	wildcard_listener_list_.push_back(l);
	
	return true;
}



//-----------------------------------------------------------------------
// triggerMessage - Public Engine
// Description 
//		Processes the message now.
//
// Arguments:	MessagePtr - a pointer to the message to send now.
// Returns:		an enum value:	NOTCONSUMED - the msg was not consumed
//								CONSUMED - msg was consumed,
//								NOLISTENER - no listener for msg
//-----------------------------------------------------------------------
Engine::MsgStatus Engine::triggerMessage(MessagePtr msg) {
	Engine::MsgStatus status = NOTCONSUMED;

	MessageListenerMap::iterator mapItr = listener_map_.find(msg->getType());
	if(mapItr == listener_map_.end()) 
		status = NOLISTENER;
	else {
        for(auto &itr : mapItr->second)
			if(itr->onMessage(msg))
				status = CONSUMED;
	}

	// Send the message to the wildcard listeners
	for (auto &itr : wildcard_listener_list_)
        if(itr->onMessage(msg))
            status = CONSUMED;

	return status;
}


//-----------------------------------------------------------------------
// queueMessage - Public Engine
// Description 
//		Queue the message to be processed on the next iteration.
//
// Arguments:	MessagePtr - a pointer to the message to queue.
// Returns:		an enum value:	NOTCONSUMED - the msg was not consumed
//								NOLISTENER - no listener for msg
//								SUCCESS - success
//-----------------------------------------------------------------------
Engine::MsgStatus Engine::queueMessage(MessagePtr msg) {
	// Check for a listener, if no listeners then skip the msg.
	MessageListenerMap::iterator mapItr = listener_map_.find(msg->getType());
	if(mapItr == listener_map_.end())
		return NOLISTENER;

	if(mapItr->second.empty())
		return NOLISTENER;

	message_queue_[current_msg_queue_].push_back(msg);
	return SUCCESS;
}



//-----------------------------------------------------------------------
// dispatchMessages - Private Engine
// Description 
//		Send all of the queued messages.
//
// Arguments:	None.
// Returns:		None.
//-----------------------------------------------------------------------
void Engine::dispatchMessages() {
	// flip the current queue so that during message processing more msgs can be sent.
	bool queue_to_process = current_msg_queue_;
	current_msg_queue_ = !current_msg_queue_;
	message_queue_[current_msg_queue_].clear();

	// Dispatch messages  
	while (!message_queue_[queue_to_process].empty()) {
		MessagePtr msg = message_queue_[queue_to_process].front();
		message_queue_[queue_to_process].pop_front();
		if(triggerMessage(msg) == NOTCONSUMED)
			message_queue_[current_msg_queue_].push_back(msg);
	}
}


//-----------------------------------------------------------------------
// tick - Public Engine
// Description 
//		Steps the engine in time.  Calls DispatchMessages, OnUpdate,
//		and OnRender.
//
// Arguments:	none.
// Returns:		None.
//-----------------------------------------------------------------------
void Engine::tick() {
	// First - Update the timer 
	timer_.tick();
	current_timestamp_ = timer_.getTime(); 
	deltaT_ = timer_.getDeltaTime();

#ifdef CONSOLE
	// Process the Console
	G_Console->onUpdate(d_dDeltaT);
#endif
    
	// Second - Send out the messages
	dispatchMessages();

	// Third - Check if the engine state needs updated
	if(queued_state_) {
		pushState(queued_state_);
		queued_state_ = 0;
	}

	static EngineStatePtr current_state = state_.front();

	// Fourth - Perform the updates
    if(current_state)
		current_state->onUpdate(deltaT_);

	// Fifth - Perform the rendering to the screen
	if (current_state)
		current_state->onRender(deltaT_);
    
#ifdef CONSOLE
	if(G_Console->isVisible())
		G_Console->onRender();
#endif

}
    
}  // namespace engine
