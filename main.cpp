
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

#define SEC_TO_MINUTES(x)   ((x) / (double)SEC_IN_MIN)
#define SEC_TO_HOURS(x)     ((x) / (double)SEC_IN_MIN / (double)MIN_IN_HOUR)

//##################################################
#define SYM_BEGIN_TIME          HOURS(11)   // 11:00
#define SYM_DURATION_TIME       HOURS(3)    // 14:00
#define SYM_HOT_HOUR_BEGIN      HOURS(11)   // 11:00
#define SYM_HOT_HOUR_DURATION   HOURS(3)    // 14:00

static double NUMBER_OF_SETTLERS    = 1;
static double NUMBER_OF_WAITERS     = 3;
static double NUMBER_OF_TABLES      = 24;
static double NUMBER_OF_COOKERS     = 6;

static double LONG_QUEUE_DECISION_SIZE      = 5;
static double LONG_QUEUE_DECISION_CHANCE    = 0.2;
static double REORDER_CHANGE                = 0.5;

static double CUSTOMER_TIME_RATION  = 1.0;
static double PREPARING_TIME_RATION = 1.0;
//##################################################



//##################################################
// Flag if its hot hour
static bool HOT_HOURS = false;

/** Preparing order time */
inline double calculateOrderPreparingTime(bool desert) {
    if (desert) {
        return Uniform(MINUTES(3), MINUTES(5)) * PREPARING_TIME_RATION;
    } else {
        return Uniform(MINUTES(10), MINUTES(20)) * PREPARING_TIME_RATION;
    }
}

/** Customer eating duration */
inline double calculcateEatTime() {
    return Uniform(MINUTES(10), MINUTES(20));
}

/** Waiter's walking time to table */
inline double calculateDistanceTime() {
    return Uniform(SECONDS(8), SECONDS(20));
}

/** Waiter writing order time */
inline double calculateWritingOrderTime() {
    return Uniform(SECONDS(15), SECONDS(60));
}

/** Customer choosing time */
inline double calculateChoosingTime() {
    return Uniform(MINUTES(2), MINUTES(5));
}

/** Customer leave decision time */
inline double calculateDecisionTime() {
    return Uniform(MINUTES(15), MINUTES(20));
}

/** Entering time */
inline double calculateEnterTime() {
    if (HOT_HOURS) {
        //return Exponential(MINUTES(2));
        return Uniform(MINUTES(1), MINUTES(4)) * CUSTOMER_TIME_RATION;
    } else {
        return Uniform(MINUTES(3), MINUTES(12)) * CUSTOMER_TIME_RATION;
    }
}
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

Histogram peopleInSystem("Zákazníci v systéme (min)", 0, 10, 10);
Histogram peopleEnteringSystem("Príchod zákazníkov (hours)", SEC_TO_HOURS(SYM_BEGIN_TIME), 0.25, 12);

Histogram personalWait("Čakacia doba na personál (min)", 0, 1, 10);
Histogram foodWait("Čakacia doba na jedlo (min)", 0, 2, 15);
Histogram queueWait("Čakacia doba vo fronte (min)", 0, 2, 10);

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
public:
    PeopleGroup() {
        uniqueID = ++GLOBAL_COUNT;
        name = to_string(uniqueID);
    }
    
    void Behavior() {
        const double tvstup = Time;
        double t, w;
        double choosingTime;
        double writingOrder;
        double distance;
        double eatingTime;
        int reorderCount = 1;
        double payTime;
        bool reorder = false;
        
        peopleEnteringSystem(SEC_TO_HOURS(Time));
        
        LogTime("[CUSTOMER: %s] Has arrived", getID());
        
        const unsigned int frontLength = settlers.Q->Length();
        // People check front length and if it is too long
        if (frontLength > LONG_QUEUE_DECISION_SIZE) {
            // They decide to leave with chance
            double chance = Random();
            if (chance < LONG_QUEUE_DECISION_CHANCE) {
                LogTime("[CUSTOMER: %s] Has decided to leave, because front is too large (%i)", getID(), frontLength);
                return;
            }
        }
        
        t = Time;
        
        // Setup timer to leave customer
        SystemLeaving* system_leaving = SystemLeaving::activateInstance(calculateDecisionTime(), this, [=](double duration) {
            if (HOT_HOURS) queueWait(SEC_TO_MINUTES(Time - t));
            const unsigned int frontLength = settlers.Q->Length();
            LogTime("[CUSTOMER] Is leaving because it takes too long (%s) (front: %i)", time_to_string(duration).c_str(), frontLength - 1);
        });
        
        LogTime("[CUSTOMER: %s] wait in queue (Queue: %i)", getID(), settlers.Q->Length() + 1);
        
        // Settler starts settling people group
        if (!tables.Empty()) {
            Enter(settlers);
            Enter(tables);
        } else {
            Enter(tables);
            Enter(settlers);
        }
        if (HOT_HOURS) queueWait(SEC_TO_MINUTES(Time - t));
        
        LogTime("[CUSTOMER: %s] Is settling (Queue: %i)", getID(), settlers.Q->Length());
        
        // Cancel customer leaving timer
        delete system_leaving;
        
        // Generate distance time for taken table
        distance = calculateDistanceTime();
        
        // Settler is navigating people to their table
        Wait(distance);
        
        // Settler returns to his position
        StoreLeaving::activateInstance(distance, settlers);
        
        LogTime("[CUSTOMER: %s] is settled (Free tables: %i)", getID(), tables.Free());
        
        //============================================
        // WAITER PHASE 1: Menu
        
        t = Time;
        // Waiter decided to go to table
        Enter(waiters);
        // Waiter is going to table
        Wait(distance);
        if (HOT_HOURS) w += (Time - t);
        
        // Waiter is returning to his place
        StoreLeaving::activateInstance(distance, waiters);
        //=============================================
        
        choosingTime = calculateChoosingTime();
        
        LogTime("[CUSTOMER: %s] Started to choose meal (%s)", getID(), time_to_string(choosingTime).c_str());
        
        // Ordering time
        Wait(choosingTime);
        
        // Here comes customer who wants some dezert at the end
        do {
            //=============================================
            // WAITER PHASE 2: Ordering
            
            LogTime("[CUSTOMER: %s] Is waiting for waiter to order", getID());
            
            t = Time;
            // Waiter decided to go to table
            Enter(waiters);
            // Waiter is going to table
            Wait(distance);
            if (HOT_HOURS) w += (Time - t);
            
            // Generate writing order time
            writingOrder = calculateWritingOrderTime();
            
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
                    LogTime("[COOKER: %s] Has started cooking (Free cookers: %i)", asynRoutine->Name(), cookers.Free());
                    
                    preparingTime = calculateOrderPreparingTime(!reorderCount);
                    
                    LogTime("[COOKER: %s] Prepare food for %s (%s)", asynRoutine->Name(), self->getID(), time_to_string(preparingTime).c_str());
                    
                    // Preparing order
                    asynRoutine->Wait(preparingTime);
                    // Leave cookers
                    asynRoutine->Leave(cookers);
                    
                    LogTime("[COOKER: %s] Meal is ready for %s", asynRoutine->Name(), self->getID());
                    
                    // Set higher priority
                    self->Priority = 1;
                    // Reactivate customer
                    self->Activate();
                });
                
            });
            //=============================================
            
            LogTime("[CUSTOMER: %s] Has ordered and is waiting for a meal",  getID());
            
            t = Time;
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
            if (HOT_HOURS) foodWait(SEC_TO_MINUTES(Time - t));
            //=============================================
            
            LogTime("[CUSTOMER: %s] Has got his meal", getID());
            
            // Waiter is returning to his place
            StoreLeaving::activateInstance(distance, waiters);
            
            // Calculate eating time
            eatingTime = calculcateEatTime();
            // Eating time
            Wait(eatingTime);
            
            // Calculate chance and check if customer wants some desert
            reorder = ((reorderCount--) && Random() < REORDER_CHANGE);
            
        } while (reorder);
        
        //=============================================
        // WAITER PHASE 4: Payment
        
        t = Time;
        // Waiter decided to go to table
        Enter(waiters);
        // Waiter is going to table
        Wait(distance);
        if (HOT_HOURS) w += (Time - t);
        
        // Calculate pay time
        payTime = MINUTES(1);
        // Customer is paying
        Wait(payTime);
        
        LogTime("[CUSTOMER: %s] Has paid", getID(), tables.Free());
        
        // Waiter is returning to his place
        StoreLeaving::activateInstance(distance, waiters);
        
        // Leave table
        Leave(tables);
        
        // Customer leaving restaurant
        Wait(distance);
        
        LogTime("[CUSTOMER: %s] Leaving table (Free tables: %i)", getID(), tables.Free());
        //=============================================
        
        // Caculate overal time in system
        if (HOT_HOURS) {
            personalWait(SEC_TO_MINUTES(w));
            const double timeInSystem = Time - tvstup;
            peopleInSystem(SEC_TO_MINUTES(timeInSystem));
        }
    }
    
    const char* getID() {
        return name.c_str();
    }
    
private:
    // Global customer group counter
    static unsigned int GLOBAL_COUNT;
    // Unique id of customer
    unsigned int uniqueID;
    // Name
    string name;
};

unsigned int PeopleGroup::GLOBAL_COUNT = 0;

/** Generating people group to system */
class PeopleGenerator: public Event {
    
    void Behavior() {
        double interval = calculateEnterTime();
        // Wait for seconds
        Activate(Time + interval);
        // Generate new people group
        (new PeopleGroup)->Activate();
    }
};

/** Generating people group to system */
class HotHour: public Process {
    void Behavior() {
        // Wait for seconds
        Activate(SYM_HOT_HOUR_BEGIN);
        
        HOT_HOURS = true;
        Log("************* HOT HOURS BEGIN *************");
        
        Wait(SYM_HOT_HOUR_DURATION);
        
        HOT_HOURS = false;
        Log("************* HOT HOURS END *************");
    }
};

string usageString = "Usage: ./restaurace [<settlers(1)> <waiters(3)> <tables(24)> <cookers(6)> <ct_ratio(1.0)> pf_ratio(1.0)]";

void usage() {
    printf("%s", usageString.c_str());
    exit(0);
}

/** Main function */
int main(int argc, char* argv[]) {
    
    if (argc > 1) {
        bool success = true;
        const int requiredCount = 6;
        if (argc > requiredCount) {
            
            double d[requiredCount];
            for (int i = 0; i < requiredCount; i++) {
                stringstream ss;
                ss << argv[1 + i];
                if (!(ss >> d[i])) {
                    Warning("Couldn't convert arguments! Ignoring all arguments!");
                    success = false;
                    break;
                }
            }
            if (success) {
                settlers.SetCapacity((int)d[0]);
                waiters.SetCapacity((int)d[1]);
                tables.SetCapacity((int)d[2]);
                cookers.SetCapacity((int)d[3]);
                
                CUSTOMER_TIME_RATION = d[4];
                PREPARING_TIME_RATION = d[5];
            }
        } else {
            Warning("Wrong argument format");
            usage();
        }
    }
    
    const double startingTime = SYM_BEGIN_TIME;
    const double endTime = startingTime + SYM_DURATION_TIME;
    
    // Initialize simulation
    Init(startingTime, endTime);
    (new HotHour)->Activate();
    // Start generator with exponential time interval
    (new PeopleGenerator())->Activate();
    // Run simulation
    Run();
    
    //################################################
    // Print results
    
    settlers.Output();
    waiters.Output();
    cookers.Output();
    tables.Output();
    peopleInSystem.Output();
    peopleEnteringSystem.Output();
    
    personalWait.Output();
    foodWait.Output();
    queueWait.Output();
    
    return EXIT_SUCCESS;
}
