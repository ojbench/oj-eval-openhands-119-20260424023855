
#ifndef LINEARSCAN_HPP
#define LINEARSCAN_HPP

// don't include other headfiles
#include <string>
#include <vector>
#include <set>

class Location {
public:
    // return a string that represents the location
    virtual std::string show() const = 0;
    virtual int getId() const = 0;
};

class Register : public Location {
private:
    int regId;
public:
    Register(int regId) {
        this->regId = regId;
    }
    virtual std::string show() const {
        return "reg" + std::to_string(regId);
    }
    virtual int getId() const {
        return regId;
    }
};

class StackSlot : public Location {
public:
    StackSlot() {}
    virtual std::string show() const {
        return "stack";
    }
    virtual int getId() const {
        return -1;
    }
};

struct LiveInterval {
    int startpoint;
    int endpoint;
    Location* location = nullptr;
};

class LinearScanRegisterAllocator {
private:
    int regNum;
    std::set<LiveInterval*> active;
    std::set<int> freeRegisters;
    
    // Comparator for sorting active intervals by endpoint
    struct IntervalComparator {
        bool operator()(LiveInterval* a, LiveInterval* b) const {
            return a->endpoint < b->endpoint;
        }
    };

    void expireOldIntervals(LiveInterval& i) {
        auto it = active.begin();
        while (it != active.end()) {
            LiveInterval* current = *it;
            if (current->endpoint >= i.startpoint) {
                break;
            }
            // Free the register
            if (current->location) {
                int regId = current->location->getId();
                if (regId != -1) {
                    freeRegisters.insert(regId);
                }
            }
            it = active.erase(it);
        }
    }
    
    void spillAtInterval(LiveInterval& i) {
        // Find the interval with the latest end point
        auto it = active.end();
        it--;
        LiveInterval* spill = *it;
        
        // If the current interval ends earlier, spill the other interval
        if (i.endpoint < spill->endpoint) {
            // Assign the spilled interval's register to current interval
            i.location = spill->location;
            spill->location = new StackSlot();
            
            // Remove spilled interval from active and add current
            active.erase(it);
            active.insert(&i);
        } else {
            // Spill the current interval
            i.location = new StackSlot();
        }
    }
    
public:
    LinearScanRegisterAllocator(int regNum) {
        this->regNum = regNum;
        // Initialize all registers as free, with smallest ID at the top
        for (int i = regNum - 1; i >= 0; i--) {
            freeRegisters.insert(i);
        }
    }
    
    void linearScanRegisterAllocate(std::vector<LiveInterval>& intervalList) {
        active.clear();
        freeRegisters.clear();
        // Reinitialize free registers
        for (int i = regNum - 1; i >= 0; i--) {
            freeRegisters.insert(i);
        }
        
        for (auto& interval : intervalList) {
            expireOldIntervals(interval);
            
            if (!freeRegisters.empty()) {
                // Allocate a free register
                int regId = *freeRegisters.begin();
                freeRegisters.erase(freeRegisters.begin());
                interval.location = new Register(regId);
                active.insert(&interval);
            } else {
                // No free registers, need to spill
                spillAtInterval(interval);
            }
        }
    }
};

#endif
