/*=========================================================================
/ James McCormick - Engine.h
/ A class for handling the core components
/==========================================================================*/

#ifndef _ENGINE_
#define _ENGINE_

#include <list>
#include <map>
#include <memory>
#include <vector>
#include "Timer.h"

namespace engine {

//-----------------------------------------------------------------------
// The System interface
//typedef unsigned long SystemID;	// A globally unique system ID number   TODO - ?needed?
class EngineSystem {
protected:
	bool paused_;
	bool visible_;

public:
	EngineSystem() : paused_(false), visible_(true) {}
	virtual ~EngineSystem() {}
	virtual void pause() { paused_ = true; }
	virtual void unPause() { paused_ = false; }
	virtual bool isVisible() const { return visible_; }
	virtual bool isPaused() { return paused_; }
	virtual void setVisibility(bool b) { visible_ = b; }

	virtual void onRender() = 0;
	virtual void onUpdate(const double& elapsedTime) = 0;
};
typedef std::shared_ptr<EngineSystem> EngineSystemPtr;
//-----------------------------------------------------------------------



//-----------------------------------------------------------------------
// The EngineState interface
//typedef unsigned StateID;  // A globally unique state ID number    TODO - ?needed?
class EngineState {
private:
	typedef std::vector<EngineSystemPtr> SystemList;
	SystemList update_list_;
	SystemList render_list_;

protected:

	inline virtual void deleteSystem(const EngineSystemPtr& ptr) {
		if (!render_list_.empty())
			render_list_.erase(std::remove(render_list_.begin(), render_list_.end(), ptr));
		if (!update_list_.empty())
			update_list_.erase(std::remove(render_list_.begin(), render_list_.end(), ptr));
	}

	inline void pushBackUpdate(const EngineSystemPtr& ptr) {
		update_list_.push_back(ptr);
	}

	inline void pushBackRender(const EngineSystemPtr& ptr) {
		render_list_.push_back(ptr);
	}

public:
	EngineState() {
#ifndef MAXSYSTEMS
#define MAXSYSTEMS 5
#endif
		update_list_.reserve(MAXSYSTEMS);
		render_list_.reserve(MAXSYSTEMS);
	}
	virtual ~EngineState() {}

	inline virtual void onUpdate(const double& deltaT) {
		for (auto& sys : update_list_)
			sys->onUpdate(deltaT);
	}

	inline virtual void onRender(const double& deltaT) {
		for (auto& sys : render_list_)
			sys->onUpdate(deltaT);
	}

	virtual void exit() = 0;
	virtual void enter() = 0;
};
typedef std::shared_ptr<EngineState> EngineStatePtr;
//-----------------------------------------------------------------------



//-----------------------------------------------------------------------
// The messaging interface
typedef unsigned int MessageType;
class Message {
private:
	const double time_stamp_;
	const MessageType message_type_;

public:
	Message(const MessageType& type, const double& stamp)
		: message_type_(type), time_stamp_(stamp) {}
	virtual ~Message() {}

	virtual const double& getTimeStamp() const { return time_stamp_; }
	virtual const MessageType& getType() const { return message_type_; }
};
typedef std::shared_ptr<Message> MessagePtr;


class MessageListener {
public:
	virtual ~MessageListener() {}
	virtual bool onMessage(MessagePtr msg) = 0;
};
typedef std::shared_ptr<MessageListener> MsgListenerPtr;
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// Engine
class Engine {
private:
    Engine() : paused_(false), current_msg_queue_(false) {}
	Engine(const Engine&); // Singleton class, so keep the copy constructor private

	engine::C_Timer timer_;
	double current_timestamp_;
    double deltaT_;

	// Engine State
	bool paused_;
	// The current state of the engine is at the front of the list followed by previous states
	std::list<EngineStatePtr> state_;		
	EngineStatePtr queued_state_;			// The state to change to on the next tick()
	//-------------------------------------


	// Message Components
	// one list per message type (stored in the map)
	typedef std::list<MsgListenerPtr> ListenerList;
	// mapping of Message ID to listener list
	typedef std::map<MessageType, ListenerList>	MessageListenerMap;
	// An entry pair for the map
	typedef std::pair<MessageType, ListenerList> MessageListenerMapPair;
	// A map insertion result
	typedef std::pair<MessageListenerMap::iterator, bool> MessageListenerMapIResult;

	// queue of pending- or processing-events - double buffer queue, one takes new messages while one is being processed.
	typedef std::list<MessagePtr> MessageQueue;
	MessageQueue message_queue_[2];								
	MessageListenerMap listener_map_;
	bool current_msg_queue_;								// the active queue 
	ListenerList wildcard_listener_list_;					// The list of the wildcard listeners.

	void dispatchMessages();

public:
	static Engine& instance() {
		// c++11 standard enforces static variables only be instantiated once.
		static Engine *instance = new Engine();
		return *instance;
	}

    ~Engine() {
        clean();
    }

    void clean() {
        if(!state_.empty()) {
			for (auto& s : state_)
				s->exit();
        }
		state_.clear();
        queued_state_ = 0;
        listener_map_.clear();
        wildcard_listener_list_.clear();
    }

	void start() { timer_.start(); current_timestamp_ = 0.0; }
	void pause() { paused_ = true; timer_.pause(); }
	void unPause() { paused_ = false; timer_.unpause(); }

	void tick();
	const double& getTimeStamp() { return current_timestamp_; }
    const double& getDeltaT() { return deltaT_; }


	EngineStatePtr getCurrentState() {
		if (!state_.empty())
			return state_.front();
		else
			return EngineStatePtr(NULL);
	}

	void pushState(EngineStatePtr s) {
		if (!state_.empty()) {
			state_.front()->exit();
		}
		s->enter();
		state_.push_front(s);
	}

	void popState() {
		if (!state_.empty()) {
			state_.front()->exit();
			state_.pop_front();
		}
	}

	void queueStateChange(EngineStatePtr state) {
		queued_state_ = state;
	}

	// Message Interface
	enum MsgStatus {
		NOTCONSUMED,
		CONSUMED,
		NOLISTENER,
		SUCCESS
	};
    
	MsgStatus queueMessage(MessagePtr msg);
	MsgStatus triggerMessage(MessagePtr msg);
	bool addWildCardListener(MsgListenerPtr l);
	void deleteWildCardListener(MsgListenerPtr l) {
		wildcard_listener_list_.remove(l);
	}
	bool addListener(MsgListenerPtr l, const MessageType& type);

	bool deleteListener(MsgListenerPtr l, const MessageType& type) {
		MessageListenerMap::iterator mapItr = listener_map_.find(type);
		if (mapItr != listener_map_.end()) {
			mapItr->second.remove(l);
			return true;
		}
		return false;
	}
}; // class Engine
    
} // namespace engine

#endif // _ENGINE_
