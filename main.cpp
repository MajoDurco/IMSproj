
#include <iostream>
#include <functional>
#include <simlib.h>

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

Queue* frontQueue();

/** Settler store for settling customers */
Store settlers("Usádzač", NUMBER_OF_SETTLERS);

/**  */
Store waiters("Čašník", NUMBER_OF_WAITERS);

Store tables("Stoly", NUMBER_OF_TABLES);

Store cookers("Kuchári", NUMBER_OF_COOKERS);

Histogram peopleInSystem("Zákazníci v systéme", 0, 1, 10);

class processOrder: public Process {

public:
    void Behavior()
	{
        double preparingTime;
		// take cook to prepare meal for order
		Enter(cookers);
	    preparingTime = Uniform(MINUTES(15), MINUTES(30));
		Log("Going to prepare food for %g ", preparingTime);
        Wait(preparingTime);
		Leave(cookers);
		Log("Meal is ready");
		// TODO somehow make priority to take waiter
    }

private:
};

/** General returning process */
class Returning: public Process {
public:
    
    // Create new instance of returning object and activate instatly
    static Returning* activateInstance(double duration, Store& release_store, function<void()> callback = NULL) {
        Returning* returning = new Returning(duration, release_store, callback);
        returning->Activate();
        return returning;
    }
    
    Returning(double duration, Store& release_store, function<void()> callback):
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


class ClientQueueLeaving: public Process {
public:
    ClientQueueLeaving(double t, Process *p) {
        this->t = t;
        this->p = p;
    }
    
    void Behavior() {
        Wait(t);
        const unsigned int frontLength = settlers.Q->Length();
        Log("[%.2f]: Client leaving because it takes too long (%.2f) (front: %i)", Time, t, frontLength - 1);
        
        // People group leaving system
        if (p) delete p;
        p->Passivate();
    }
    
private:
    double t;
    Process *p;
};


/** Class representing people gruop incoming to restaurant */
class PeopleGroup: public Process {
    
    void Behavior() {
        double tvstup = Time;
		double choosingTime;
        double writingOrder;
        double distance;
        
        const unsigned int frontLength = settlers.Q->Length();
        // People check front length and if it is too long
        if (frontLength > LONG_QUEUE_DECISION_SIZE) {
            // They decide to leave with chance
            double chance = Random();
            if (chance < LONG_QUEUE_DECISION_CHANGE) {
                Log("[%.2f]: Client decided to leave, because front is too large (%i)", Time, frontLength);
                return;
            }
        }
        
        // Setup timer to leave customer
        ClientQueueLeaving* leavingTimer = new ClientQueueLeaving(LONG_QUEUE_DECISION_TIME, this);
        leavingTimer->Activate();
        
        Log("Customer INCOME QUEUE (%i)", settlers.Q->Length() + 1);
        
        // Settler starts settling people group
        // TODO: Does it work correctly?
        if (!tables.Empty()) {
            Enter(settlers);
            Enter(tables);
        } else {
            Enter(tables);
            Enter(settlers);
        }
        
        Log("Customer IS SETTLING (%i)", settlers.Q->Length());
        
        // Cancel customer leaving timer
        delete leavingTimer;
        
        // Generate distance time for taken table
        distance = Uniform(SECONDS(15), SECONDS(30));
        
        // Settler is navigating people to their table
        Wait(distance);

        // Settler returns to his position
        Returning::activateInstance(distance, settlers);
        
        // TODO: People are settled
        Log("Customer SETTLED (tables: %i)", tables.Used());

        //============================================
        // WAITER PHASE 1: Menu
        
        // Waiter decided to go to table
        Enter(waiters);
        // Waiter is going to table
		Wait(distance);
        
        Log("Customer has got MENU");
        
        // Waiter is returning to his place
        Returning::activateInstance(distance, waiters);
        //=============================================

        choosingTime = Uniform(MINUTES(3), MINUTES(15));
        
        // Ordering time
        Wait(choosingTime);
        
        Log("Customer has chosen a meal");
        
        //=============================================
        // WAITER PHASE 2: Ordering
        
        // Waiter decided to go to table
        Enter(waiters);
        // Waiter is going to table
		Wait(distance);
        
        // Generate writing order time
        writingOrder = Uniform(SECONDS(15), SECONDS(60));
        
        // Waiter is writing order
        Wait(writingOrder);
        
        // Waiter is returning to his place
        Returning::activateInstance(distance, waiters, []() {
            // Place new order to the kitchen
            (new processOrder())->Activate();
        });
        //=============================================
        
        Log("Customer is waiting for a meal");

        // TODO: Leave(tables);
        
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
    (new PeopleGenerator(MINUTES(1)))->Activate();
    // Run simulation
    Run();
    
    //################################################
    // Print results
    
    settlers.Output();
    peopleInSystem.Output();
    
    return EXIT_SUCCESS;
}
