
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

Facility settling("Usádzač");
Histogram peopleInSystem("Zákazníci v systéme", 0, 1, 10);

/** Class representing people gruop incoming to restaurant */
class PeopleGroup : public Process {
    
    void Behavior() {
        double tvstup = Time;
        double settlingTime;
        
        // Settler starts settling people group
        Seize(settling);
        // Settler is navigating people to their table
        settlingTime = Uniform(SECONDS(15), SECONDS(30));
        Wait(settlingTime);
        
        // TODO: People are settled
        
        // Settler is returning to his position
        Wait(settlingTime);
        // Settler is free for taking another group of people
        Release(settling);
        
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
    (new PeopleGenerator(MINUTES(3)))->Activate();
    // Run simulation
    Run();
    
    //################################################
    // Print results
    
    settling.Output();
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
