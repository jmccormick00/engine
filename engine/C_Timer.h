#ifndef _CTIMER_
#define _CTIMER_

#include <chrono>

namespace engine {

class C_Timer {
private:
    typedef std::chrono::high_resolution_clock Clock;
    static const long COUNTSPERSECOND = 1e9;

    bool d_bPaused;

    Clock::time_point	d_baseTime,		// The base time of the timer.
                        d_pauseTime,	// The time marker when the timer was paused
                        d_prevTime,		// The time during the last time step
                        d_currentTime;

    std::chrono::nanoseconds d_pauseTotal;	// The total time the timer was paused
    std::chrono::nanoseconds d_deltaTime;	// The delta seconds between ticks

public:
    // Constructor
    C_Timer() : d_bPaused(false), d_pauseTotal(std::chrono::nanoseconds::zero()) {}
    ~C_Timer() {}

    // Gets time in seconds since Reset() was called
    inline double getTime() {
        // If we are paused, dont count the time since we have stopped
        //	
        // -----*----------------*--------------*----------> Time
        //	d_baseTime		d_pauseTime	    d_currentTime
        if (d_bPaused)
            return (double)(d_pauseTime - d_baseTime).count()/COUNTSPERSECOND;

        // The (d_currentTime - d_baseTime) includes any paused time
        // So need to subract that out
        //						 |<---d_pauseTotal-->|
        // -----*----------------*-------------------*----------------*----> Time
        //	d_baseTime		d_pauseTime	           start()        d_currentTime
        else {
            d_currentTime = Clock::now();
            return (double)(d_currentTime - d_baseTime - d_pauseTotal).count()/COUNTSPERSECOND;
        }
    }

    // Gets the time in seconds since last tick()
    inline double getDeltaTime() {
        return (double)d_deltaTime.count()/COUNTSPERSECOND;
    }

    // Called to zero the timer and start the counters
    inline void start() {
        d_currentTime = Clock::now();
        d_baseTime = d_currentTime;
        d_prevTime = d_currentTime;
        d_pauseTotal = std::chrono::nanoseconds::zero();
        d_bPaused = false;
    }

    // Called to pause the timer
    inline void pause() {
        // If its already paused, don't do anything
        if (d_bPaused) return;
        // Save the time we stopped at
        d_pauseTime = Clock::now();
        d_bPaused = true;
    }
    
    inline void unpause() {
        if(!d_bPaused) return;
        // We are resuming from a stopped state
        // accumalate the paused time
        //
        //				|<--d_pauseTotal--->|
        // -------------*-------------------*--------------> Time
        //			d_pauseTime			 start()
        d_currentTime = Clock::now();
        d_pauseTotal += (d_currentTime - d_pauseTime);
        
        // Reset the previous time
        d_prevTime = d_currentTime;
        d_bPaused = false;
    }

    // Called before the scene is updated, at the beginning of the sim loop
    inline void tick() {
        // If paused do nothing
        if (d_bPaused) return;

        // Calculate delta
        //
        //				|<----d_deltaTime----->|
        // -------------*----------------------*--------------> Time
        //			d_iPrevTime			      Tick()
        d_currentTime = Clock::now();
        d_deltaTime = d_currentTime - d_prevTime;
        d_prevTime = d_currentTime;
    }
};

}  // engine namespace

#endif // _CTIME_