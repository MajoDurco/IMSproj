
#include <iostream>
#include <functional>
#include <simlib.h>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "error.hpp"

using namespace std;

// Helpfull macros with time calculations
#define SEC_IN_MIN      60
#define MIN_IN_HOUR     60
#define SECONDS(x)  (x)
#define MINUTES(x)  (x * SEC_IN_MIN)
#define HOURS(x)    (x * MIN_IN_HOUR * SEC_IN_MIN)

//##################################################
#define NUMBER_OF_SETTLERS  1
#define NUMBER_OF_WAITERS   3
#define NUMBER_OF_TABLES    40
#define NUMBER_OF_COOKERS   2

#define LONG_QUEUE_DECISION_SIZE    (20)
#define LONG_QUEUE_DECISION_CHANGE  (0.2)
#define LONG_QUEUE_DECISION_TIME    (MINUTES(20))
//##################################################

/** Converts seconds to human readable format */
string time_to_string(double time) {
    long seconds = round(time);
    long sec = seconds % SEC_IN_MIN;
    long min = (seconds / SEC_IN_MIN) % MIN_IN_HOUR;
    long hour = (seconds / (MIN_IN_HOUR * SEC_IN_MIN));
    
    stringstream ss;
    ss << std::setw(2) << std::setfill('0') << sec;
    std::string s_sec(ss.str());
    ss.str("");
    ss << std::setw(2) << std::setfill('0') << min;
    std::string s_min(ss.str());
    ss.str("");
    ss << std::setw(2) << std::setfill('0') << hour;
    std::string s_hour(ss.str());
    return s_hour + ":" + s_min + ":" + s_sec;
}

/** Prints message with simulation time prefix to stdout */
void LogTime(string fmt, ...) {
#if DEBUG
    va_list list;
    va_start(list, fmt);
    string time = time_to_string(Time);
    fprintf(stderr, "LOG: [%s]: " , time.c_str());
    vfprintf(stderr, fmt.c_str(), list);
    printf("\n");
    va_end(list);
#endif
}

/** Settlers' store */
Store settlers("Usádzač", NUMBER_OF_SETTLERS);
/** Waiters' store */
Store waiters("Čašník", NUMBER_OF_WAITERS);
/** Tables' store */
Store tables("Stoly", NUMBER_OF_TABLES);
/** Cookers' store */
Store cookers("Kuchári", NUMBER_OF_COOKERS);

Histogram peopleInSystem("Zákazníci v systéme", 0, 1, 10);

/** General asynch process */
class AsyncRoutine: public Process {
public:
    
    // Create new instance of returning object and activate instatly
    static AsyncRoutine* activateInstance(function<void(Process*)> func = NULL) {
        AsyncRoutine* store_leaving = new AsyncRoutine(func);
        store_leaving->Activate();
        return store_leaving;
    }
    
    AsyncRoutine(function<void(Process*)> func): func(func) {
        // Empty
    }
    
    void Behavior() {
        // Perform function instantly
        if (func) func(this);
    }

private:
    function<void(Process*)> func;
    
};

/** General store leaving process */
class StoreLeaving: public Process {
public:
    
    // Create new instance of returning object and activate instatly
    static StoreLeaving* activateInstance(double duration, Store& release_store, function<void()> callback = NULL) {
        StoreLeaving* store_leaving = new StoreLeaving(duration, release_store, callback);
        store_leaving->Activate();
        return store_leaving;
    }
    
    StoreLeaving(double duration, Store& release_store, function<void()> callback):
        duration(duration), release_store(release_store), callback(callback) {
        // Empty
    }
    
    void Behavior() {
        Wait(duration);
        // Leave store
        Leave(release_store);
        // Perform callback if exists
        if (callback) callback();
    }
    
private:
    // Duration
    double duration;
    // Store to leave
	Store& release_store;
    // Callback after leave
    function<void()> callback;
    
};

/** General system leaving process */
class SystemLeaving: public Process {
public:
    
    // Create new instance of returning object and activate instatly
    static SystemLeaving* activateInstance(double duration, Process* process, function<void(double)> callback = NULL) {
        SystemLeaving* system_leaving = new SystemLeaving(duration, process, callback);
        system_leaving->Activate();
        return system_leaving;
    }
    
    SystemLeaving(double duration, Process* process, function<void(double)> callback = NULL):
        duration(duration), process(process), callback(callback) {
        // Empty
    }
    
    void Behavior() {
        Wait(duration);
        // People group leaving system
        if (process) {
            delete process;
        } else {
            Warning("SystemLeaving has empty process");
        }
        // Perform callback if exists
        if (callback) callback(duration);
    }
    
private:
    // Duration
    double duration;
    // Process to delete
    Process* process;
    // Callback after system leave
    function<void(double)> callback;
};


/** Class representing people gruop incoming to restaurant */
class PeopleGroup: public Process {
    
    void Behavior() {
        double tvstup = Time;
		double choosingTime;
        double writingOrder;
        double distance;
        
        LogTime("NEW customer %p has come", this);
        
        const unsigned int frontLength = settlers.Q->Length();
        // People check front length and if it is too long
        if (frontLength > LONG_QUEUE_DECISION_SIZE) {
            // They decide to leave with chance
            double chance = Random();
            if (chance < LONG_QUEUE_DECISION_CHANGE) {
                LogTime("[CUSTOMER] Has decided to leave, because front is too large (%i)", frontLength);
                return;
            }
        }
        
        // Setup timer to leave customer
        SystemLeaving* system_leaving = SystemLeaving::activateInstance(LONG_QUEUE_DECISION_TIME, this, [](double duration) {
            const unsigned int frontLength = settlers.Q->Length();
            LogTime("[CUSTOMER] Is leaving because it takes too long (%s) (front: %i)", time_to_string(duration).c_str(), frontLength - 1);
        });
        
        LogTime("[CUSTOMER] wait in queue (Queue: %i)", settlers.Q->Length() + 1);
        
        // Settler starts settling people group
        // TODO: Does it work correctly?
        if (!tables.Empty()) {
            Enter(settlers);
            Enter(tables);
        } else {
            Enter(tables);
            Enter(settlers);
        }
        
        LogTime("[CUSTOMER] IS SETTLING (Queue: %i)", settlers.Q->Length());
        
        // Cancel customer leaving timer
        delete system_leaving;
        
        // Generate distance time for taken table
        distance = Uniform(SECONDS(15), SECONDS(30));
        
        // Settler is navigating people to their table
        Wait(distance);

        // Settler returns to his position
        StoreLeaving::activateInstance(distance, settlers);
        
        LogTime("[CUSTOMER] is settled (Free tables: %i)", tables.Free());

        //============================================
        // WAITER PHASE 1: Menu
        
        // Waiter decided to go to table
        Enter(waiters);
        // Waiter is going to table
		Wait(distance);
        
        // Waiter is returning to his place
        StoreLeaving::activateInstance(distance, waiters);
        //=============================================
        
        choosingTime = Uniform(MINUTES(3), MINUTES(15));
        
        LogTime("[CUSTOMER] Started to choosing food (%s)", time_to_string(choosingTime).c_str());
        
        // Ordering time
        Wait(choosingTime);
        
        //=============================================
        // WAITER PHASE 2: Ordering
        
        LogTime("[CUSTOMER] Is waiting for waiter to order");
        
        // Waiter decided to go to table
        Enter(waiters);
        // Waiter is going to table
		Wait(distance);
        
        // Generate writing order time
        writingOrder = Uniform(SECONDS(15), SECONDS(60));
        
        // Waiter is writing order
        Wait(writingOrder);
        
        PeopleGroup* self = this;
        // Waiter is returning to his place
        StoreLeaving::activateInstance(distance, waiters, [=]() {
            
            // Place new order to the kitchen after return
            AsyncRoutine::activateInstance([=](Process* asynRoutine) {
                double preparingTime;
                
                // Take cook to prepare meal for order
                asynRoutine->Enter(cookers);
                LogTime("[KITCHEN] Cooker started cooking (Free cookers: %i)", cookers.Free());
                
                preparingTime = MINUTES(60) + Uniform(MINUTES(15), MINUTES(30));
                
                LogTime("[KITCHEN] Prepare food for %s (%s)", self->Name(), time_to_string(preparingTime).c_str());
                
                // Preparing order
                asynRoutine->Wait(preparingTime);
                // Leave cookers
                asynRoutine->Leave(cookers);
                
                LogTime("[KITCHEN] Meal is ready for %s", self->Name());
                
                // Set higher priority
                self->Priority = 1;
                // Reactivate customer
                self->Activate();
            });
            
        });
        //=============================================
        
        LogTime("[CUSTOMER] is waiting for a meal");
        
        // Freez process
        Passivate();
        
        //=============================================
        // WAITER PHASE 3: Meal
        
        // Waiter decided to go to table
        Enter(waiters);
        // Waiter is going to table
        Wait(distance);
        // Reset priority
        Priority = 0;
        //=============================================
        
        LogTime("[CUSTOMER] Has got his meal (%s)", Name());
        
        // TODO: Phase 4 PAY
        
        // Leave table
        Leave(tables);
        
        LogTime("[CUSTOMER] %s LEAVING TABLE (Free tables: %i)", Name(), tables.Free());
        
        const double timeInSystem = Time - tvstup;
        peopleInSystem(timeInSystem);
    }
};

/** Generating people group to system */
class PeopleGenerator: public Event {
public:
    PeopleGenerator(): PeopleGenerator(0) {}
    PeopleGenerator(double exp_time): exp_time(exp_time) {}
    
private:
    // Exponential interval time
    double exp_time;
    
    void Behavior() {
        // Wait for seconds
        Activate(Time + Exponential(exp_time));
        // Generate new people group
        (new PeopleGroup)->Activate();
    }
};

/** Main function */
int main(int argc, char* argv[]) {
    
    /* Restaurant
     Observe time:      11:00 - 13:30 (2.5 hours)
        Observing start time:   0 hours
        Observing end time:     2.5 hours
     */
    const double startingTime = 0;      // From 11:00
    const double endTime = HOURS(2.5);  // To   13:30
    
    // Initialize simulation
    Init(startingTime, endTime);
    // Start generator with exponential time interval
    (new PeopleGenerator(MINUTES(100)))->Activate();
    // Run simulation
    Run();
    
    //################################################
    // Print results
    
    settlers.Output();
    peopleInSystem.Output();
    
    return EXIT_SUCCESS;
}
