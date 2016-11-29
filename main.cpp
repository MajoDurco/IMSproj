
#include <iostream>

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




class SettlerReturning: public Process {
public:
    
    SettlerReturning(double t) {
        this->t = t;
    }
    
    void Behavior() {
        Wait(t);
        // Settler is free for taking another group of people
        Leave(settlers);
    }
    
private:
    double t;
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
    }
    
private:
    double t;
    Process *p;
};





/** Class representing people gruop incoming to restaurant */
class PeopleGroup: public Process {
    
    void Behavior() {
        double tvstup = Time;
        double settlingTime;
        
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
        if (!tables.Empty()) {
            Enter(settlers);
            Enter(tables);
        } else {
            Enter(tables);
            Enter(settlers);
        }
        
        Log("Customer IS SETTLING (%i)", settlers.Q->Length());
        
        delete leavingTimer;
        
        // Settler is navigating people to their table
        settlingTime = Uniform(SECONDS(1500), SECONDS(3000));
        Wait(settlingTime);

        // Settler returns to his position
        (new SettlerReturning(settlingTime))->Activate();
        
        // TODO: People are settled
        Log("Customer SETTLED (tables: %i)", tables.Used());
        
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
    (new PeopleGenerator(MINUTES(0.1)))->Activate();
    // Run simulation
    Run();
    
    //################################################
    // Print results
    
    settlers.Output();
    peopleInSystem.Output();
    
    return EXIT_SUCCESS;
}

/* Example of small simulation
 
using namespace simlib3;

Facility Linka("Obsluzna linka");

Stat dobaObsluhy("Doba obsluhy na lince");

Histogram dobaVSystemu("Celkova doba v systemu", 10, 1, 5);

class Transakce : public Process {
    
    void Behavior() {
        
        
        double tvstup = Time;
        double obsluha;
        
        //           O   \------------------------------
        // |exp() -> O -> |seize -> O -> |exp() -> O -> |release -> O
        Seize(Linka);
        obsluha = Exponential(10);
        Wait(obsluha); // Alternative: Activate(Time+obsluha);
        dobaObsluhy(obsluha);
        Release(Linka);
        
        dobaVSystemu(Time - tvstup);
    }
};

class Generator : public Event {
    void Behavior() {
        (new Transakce)->Activate();
        Activate(Time + Exponential(11));
    }
};


int main(int argc, char* argv[]) {
    
    Init(0, 1000);
    (new Generator)->Activate();
    Run();
    dobaObsluhy.Output();
    Linka.Output();
    dobaVSystemu.Output();
    
    return EXIT_SUCCESS;
}
 */
