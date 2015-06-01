#ifndef _CTIMER_
#define _CTIMER_

#include <chrono>

namespace engine {

class C_Timer {
private:
    typedef std::chrono::high_resolution_clock Clock;
    static const long COUNTSPERSECOND = 1e9;

    bool paused_;

    Clock::time_point	baseTime_,		// The base time of the timer.
                        pauseTime_,	// The time marker when the timer was paused
                        prevTime_,		// The time during the last time step
                        currentTime_;

    std::chrono::nanoseconds pauseTotal_;	// The total time the timer was paused
    std::chrono::nanoseconds deltaTime_;	// The delta seconds between ticks

public:
    // Constructor
    C_Timer() : paused_(false), pauseTotal_(std::chrono::nanoseconds::zero()) {}
    ~C_Timer() {}

    // Gets time in seconds since Reset() was called
    inline double getTime() {
        // If we are paused, dont count the time since we have stopped
        //	
        // -----*----------------*--------------*----------> Time
        //	d_baseTime		d_pauseTime	    d_currentTime
        if (paused_)
            return (double)(pauseTime_ - baseTime_).count()/COUNTSPERSECOND;

        // The (d_currentTime - d_baseTime) includes any paused time
        // So need to subract that out
        //						 |<---d_pauseTotal-->|
        // -----*----------------*-------------------*----------------*----> Time
        //	d_baseTime		d_pauseTime	           start()        d_currentTime
        else {
            currentTime_ = Clock::now();
            return (double)(currentTime_ - baseTime_ - pauseTotal_).count()/COUNTSPERSECOND;
        }
    }

    // Gets the time in seconds since last tick()
    inline double getDeltaTime() {
        return (double)deltaTime_.count()/COUNTSPERSECOND;
    }

    // Called to zero the timer and start the counters
    inline void start() {
        currentTime_ = Clock::now();
        baseTime_ = currentTime_;
        prevTime_ = currentTime_;
        pauseTotal_ = std::chrono::nanoseconds::zero();
        paused_ = false;
    }

    // Called to pause the timer
    inline void pause() {
        // If its already paused, don't do anything
        if (paused_) return;
        // Save the time we stopped at
        pauseTime_ = Clock::now();
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
        currentTime_ = Clock::now();
        pauseTotal_ += (currentTime_ - pauseTime_);
        
        // Reset the previous time
        prevTime_ = currentTime_;
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
        currentTime_ = Clock::now();
        deltaTime_ = currentTime_ - prevTime_;
        prevTime_ = currentTime_;
    }
};

}  // engine namespace

#endif // _CTIME_
