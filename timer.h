#pragma once

#include <chrono>
#include <cmath>

// Simple timer
struct Timer {
    Timer() : lastDiff(0.0f), lastTime(std::chrono::high_resolution_clock::now()) {}

    // returns seconds since last sample
    float sample() {
        auto now = std::chrono::high_resolution_clock::now();
        // for some reason, std::seconds doesn't seem to be defined on my sys
        lastDiff = 
            std::chrono::duration_cast<std::chrono::duration<float,std::milli>>(now-lastTime).count()/1000.0f;
        lastTime = now;
        return lastDiff;
    }
    
    // most recent time differential
    float lastDiff;
    // last time we sampled (absolute)
    std::chrono::time_point<std::chrono::high_resolution_clock> lastTime;
};

// timer with Average
struct AvgTimer {
    AvgTimer() : average(0.0f) {}

    float sample() {
        float diff = timer.sample();

        if(average > 0.0f)
            average = 0.95f * average + 0.05f * diff;
        else
            average = diff;

        return average;
    }

    // clear average
    void reset() {
        average = 0.0f;
    }

    float average;
    Timer timer;
};
