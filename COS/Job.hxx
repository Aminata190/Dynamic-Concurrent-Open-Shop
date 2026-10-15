#pragma once

#include <iostream>
#include <map>
#include <string>
#include <utility> 
#include <math.h>
#include <vector>
#include <random>


using namespace std;


struct Phase {
    int duration;   // quantum length
    double beta_prob; // proba job or task not finished after processing duration
};


// DISCRETE  : processing time given by distribution (time -> probability)
// GEOMETRIC : P(P = k) = alpha (1-alpha)^(k-1), k >= 1  (memoryless)
enum class DistType { DISCRETE, GEOMETRIC };

//----------------------------------------------------------------//
// Class :      Task                                              //
//                                                                //
//                                                                //
// Description : classe representant une tache et les probalités  //
//                                                                 //
//                                                                //
//----------------------------------------------------------------//


class Task
{

private:
    int                 id_;
    DistType            type_  = DistType::DISCRETE;
    map<int, double>    distribution; // DISCRETE : processing time -> probability
    double              alpha_ = 0.0; // GEOMETRIC : probability to finish at each time unit
    int                 progress;
    bool                completed;
    int                 cct_ ;

    vector<Phase>       phases_;

    int              priority_;

// Friendly methods
public:
  friend   istream   &   operator>>(istream &stream, Task &);
  friend   ostream   &   operator<<( ostream &, Task const & );


public:
    // Constructeur
    Task(){ 
        progress =0; 
        completed= false;
        cct_=0; 
        }


    Task(int id, map<int,double> dist):
        id_(id),
        distribution(dist), 
        progress(0), 
        completed(false),
        cct_(0) 
        {}

    Task(int id, map<int,double> dist, int cct):
        id_(id),
        distribution(dist), 
        progress(0), 
        completed(false),
        cct_(cct)
        {}

    // Geometric task
    Task(int id, double alpha):
        id_(id),
        type_(DistType::GEOMETRIC),
        alpha_(alpha),
        progress(0),
        completed(false),
        cct_(0)
        {}

    // --- Setters ---
    void                        setId( int id) { id_ = id; }
    void                        setCCT( int cct) { cct_ = cct; }
    void                        setDistribution(map<int,double> dist){distribution = dist;}

    void                        setPhases(vector<Phase> phases){ phases_ = phases;}

    void                        setPriority(int p) { priority_ = p; }

    void                        setProgress( int prog){ progress = prog;}
    void                        setComplete( bool comp) { completed = comp; }

    // --- Getters ---
    int                         getId() const { return id_; }
    DistType                    getType() const { return type_; }
    bool                        isGeometric() const { return type_ == DistType::GEOMETRIC; }
    double                      getAlpha() const { return alpha_; }
    map<int, double>            getDistribution() const { return distribution;}
    int                         getProgress() const { return progress; }
    bool                        isCompleted() const { return completed; }
    int                         getCCT() const { return cct_; }

    vector<Phase>               getPhases(){return phases_;}
    const vector<Phase>&        getPhases() const { return phases_; } 

    double                      getTotalProba(int y);
    double                      getTotalProba(int y) const;

    vector<pair<int,double>>    getRemaining(int y) ;
    vector<pair<int,double>>    getRemaining(int y) const;

    int                         getPriority() const { return priority_; }

    // --- Other methods ---
    void                        advance(int q);
    void                        advance(int q, int current_time);

    int                         getMaxProcTime()const;  //  largest processing time (DISCRETE only)

    // --- Distribution (DISCRETE and GEOMETRIC) ---
    double                      expectedRemaining(int y) const; // E[P - y | P > y]
    int                         minQuantum(int y) const;        // smallest remaining processing time
    int                         sample(mt19937& rng) const;     // one realization of P
    double                      getCV() const;                  // coefficient of variation of P


};






//----------------------------------------------------------------//
// Class :      Job                                              //
//                                                                //
//                                                                //
// Description : classe representant un job et les probalités      //
//                                                                 //
//                                                                //
//----------------------------------------------------------------//


class Job
{

private:
    int                 id_;
    double              weight;
    int                 nbTask_;
    Task                *task_;
    bool                completed;
    int                 cct_ ;
// Friendly methods
public:
  friend   istream   &   operator>>(istream &stream, Job &);
  friend   ostream   &   operator<<( ostream &, Job const & );

// methodes de classe 
public:
    // Constructeur
    Job(){
        task_=0;
        weight =1;
        completed=false;
        cct_=0; 
        }

    Job(int id, double w, int nbTask):
        id_(id),
        weight(w),
        nbTask_(nbTask),
        completed(false),
        cct_(0) 
        {
            task_ = new Task[nbTask_];
        }
    Job(const Job& other) :
        id_(other.id_),
        weight(other.weight),
        nbTask_(other.nbTask_),
        completed(other.completed),
        cct_(other.cct_)
    {
        task_ = new Task[nbTask_];
        for (int i = 0; i < nbTask_; i++)
            task_[i] = other.task_[i];
    }   

    Job& operator=(const Job& other) {
        if (this == &other) return *this;
        delete[] task_;
        id_       = other.id_;
        weight    = other.weight;
        nbTask_   = other.nbTask_;
        completed = other.completed;
        cct_      = other.cct_;
        task_ = new Task[nbTask_];
        for (int i = 0; i < nbTask_; i++)
            task_[i] = other.task_[i];
        return *this;
    }   

    ~Job() { delete[] task_; }


    // --- Getters ---
    int                         getId() const { return id_; }
    double                      getWeight()  const { return weight; }
    int                         getNbTask() const { return nbTask_; }
    int                         getTaskId(int t) const { return task_[t].getId(); }

    Task&                       getTask(int t) { return task_[t]; }
    const Task&                 getTask(int t) const { return task_[t]; } 

    bool                        isCompleted() const { return completed; }
    int                         getCCT() const { return cct_; }

    vector<Phase>               getTaskPhases(int t){ return task_[t].getPhases();}

    // double                      getTotalProba(int y);

    // vector<pair<int,double>>    getRemaining(int y) ;


    // --- Setters ---
    void                        setId( int id) { id_ = id; }
    void                        setWeight(double w) { weight = w; }
    void                        setNbTask(int m) { nbTask_ = m; }
    void                        setTask(int t, Task & T) { task_[t] = T; }
    void                        setTaskDistribution(int t , map<int,double> dist)  { task_[t].setDistribution(dist) ;}
    void                        setCCT( int cct) { cct_ = cct; }
    void                        setComplete( bool comp) { completed = comp; }
    void                        updateCCT(double c) { if (cct_ < c) cct_=c; }

    void                        setTaskPhases(int t,vector<Phase> phases){task_[t].setPhases(phases);}

    void                        setTaskPriority(int t, int p) { task_[t].setPriority(p); }
    // void



};



//------------------------------------//
//      Jobs reading                  //
//  Expected format (one job / line): //
//  id weight nbr_Task  id_task  distribution                 //
//    DISCRETE  : v1:p1 v2:p2 ...                             //
//    GEOMETRIC : g:alpha  or  alpha (written with a dot, e.g. 0.524)  //
//  All tasks of an instance must have the same type          //
//------------------------------------//

vector<Job> readJobsFromFile(const string &filename);

bool isGeometricInstance(const vector<Job>& jobs);



struct Quantum {
    int j;         // job id  
    int t;         // task id
    int i;         // i  (1..n_j)
    int y;         // y_{ji}
    int q;         // q_{ji} (argmax)
    double rank;   // R_j(y_{ji})
};

struct Machine {
    int id;        // id
    int j;         // job id  
    int t;         // task id
    
};


