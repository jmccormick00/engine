#ifndef _TIMER_
#define _TIMER_

#include <chrono>

namespace engine {

class C_Timer {
private:
    typedef std::chrono::high_resolution_clock Clock;
    static const long COUNTSPERSECOND = 1e9;

    bool paused_;

    Clock::time_point	base_time_,		// The base time of the timer.
                        pause_time_,	// The time marker when the timer was paused
                        prev_time_,		// The time during the last time step
                        current_time_;

    std::chrono::nanoseconds pause_total_;	// The total time the timer was paused
    std::chrono::nanoseconds delta_time_;	// The delta seconds between ticks

public:
    // Constructor
    C_Timer() : paused_(false), pause_total_(std::chrono::nanoseconds::zero()) {}
    ~C_Timer() {}

    // Gets time in seconds since Reset() was called
    inline double getTime() {
        // If we are paused, dont count the time since we have stopped
        //	
        // -----*----------------*--------------*----------> Time
        //	d_baseTime		d_pauseTime	    d_currentTime
        if (paused_)
            return (double)(pause_time_ - base_time_).count()/COUNTSPERSECOND;

        // The (d_currentTime - d_baseTime) includes any paused time
        // So need to subract that out
        //						 |<---pause_total--->|
        // -----*----------------*-------------------*----------------*----> Time
        //	base_time		pause_time	           start()       current_time
        else {
            current_time_ = Clock::now();
            return (double)(current_time_ - base_time_ - pause_total_).count()/COUNTSPERSECOND;
        }
    }

    // Gets the time in seconds since last tick()
    inline double getDeltaTime() {
        return (double)delta_time_.count()/COUNTSPERSECOND;
    }

    // Called to zero the timer and start the counters
    inline void start() {
        current_time_ = Clock::now();
        base_time_ = current_time_;
        prev_time_ = current_time_;
        pause_total_ = std::chrono::nanoseconds::zero();
        paused_ = false;
    }

    // Called to pause the timer
    inline void pause() {
        // If its already paused, don't do anything
        if (paused_) return;
        // Save the time we stopped at
        pause_time_ = Clock::now();
        paused_ = true;
    }
    
    inline void unpause() {
        if(!paused_) return;
        // We are resuming from a stopped state
        // accumalate the paused time
        //
        //				|<--d_pauseTotal--->|
        // -------------*-------------------*--------------> Time
        //			d_pauseTime			 start()
        current_time_ = Clock::now();
        pause_total_ += (current_time_ - pause_time_);
        
        // Reset the previous time
        prev_time_ = current_time_;
        paused_ = false;
    }

    // Called before the scene is updated, at the beginning of the sim loop
    inline void tick() {
        // If paused do nothing
        if (paused_) return;

        // Calculate delta
        //
        //				|<----d_deltaTime----->|
        // -------------*----------------------*--------------> Time
        //			d_iPrevTime			      Tick()
        current_time_ = Clock::now();
        delta_time_ = current_time_ - prev_time_;
        prev_time_ = current_time_;
    }
};

}  // engine namespace

#endif // _TIMER_
