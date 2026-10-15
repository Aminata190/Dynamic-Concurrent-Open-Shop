#pragma once

#include <iostream>
#include <map>
#include <string>
#include <cmath>
#include <algorithm>
#include <queue>
#include <set>
#include <functional>

using namespace std;


#include "ValueIteration.hxx"

//------------------------------------------------------------------//
//                  Value Function Evaluation                       //
//                                                                  //
//                        Helper structs                            //
//------------------------------------------------------------------//

struct NextJob {
  vector<Job> Jobs;
  Matrix S;
  double p;
};


struct JobInfo{
    int job_id;
    double rank;
    int q;

    bool operator<(const JobInfo& other) const {
        if (rank != other.rank)
            return rank > other.rank;   // larger rank = higher priority - on top
        return job_id > other.job_id;   // tie-breaker
    }
};




// ******************************************************************//


double getProba(const Task& T, int y);


// Main Functions

double  Investment(Task T, int q, int y);


double  Reward(Task T, int q, int y);

pair<int, int> GetBestJob_Min(vector<Job> &jobs, vector<int> &machineTime);


pair<int,double> GittinsIndex(Task T, int y);

pair<int,double> GittinsIndex_MinQ(Task T, int y);

vector<pair<int, double>> GittinsRanks(Task T, int y);

//------------------------------------------------------------------//
//  Method 1: GIPP per machine (independent of required tasks)      //
//                                                                  //
//  Description:                                                     //
//    Compute the Gittins Index Priority Policy (GIPP) separately    //
//    on each machine, ignoring cross-machine task coupling.         //
//    Each machine selects the task with the highest local rank.     //
//    Then execute according to the order.                          //
//  Notes:                                                           //
//    - No coordination across machines (Concurrent Open Shop).      //
//    - Preempt only at quantum boundaries (q* per task,machine).    //
//    - Good as a baseline; may be suboptimal for job completion.    //
//------------------------------------------------------------------//



pair<double, double> GIPP_M1(vector<Job> &jobs);


//------------------------------------------------------------------//
//  Method 2: GIPP with global job selection (min/max aggregation)  //
//                                                                  //
//  Description:                                                    //
//    - Compute the Gittins Index Priority Policy (GIPP) separately //
//      for each task on each machine.                              //
//    - Select best the per-machine ranks for each job using either //
//      min{R_j,a} or max{R_j,a} across machines.                   //
//    - Get the job with the best rank.                             //
//    - Determine its quantum length q* (from the maximizing task), //
//      and execute *all* of that job’s remaining tasks concurrently//
//      on their machines for q*.                                   //
//                                                                  //
//  Notes:                                                          //
//    - This introduces some coordination across machines: one job  //
//      is chosen globally, but ranks are still computed locally.   //
//    - Preemption occurs only at quantum boundaries (q*).          //
//    - Works as a heuristic/baseline; not guaranteed optimal for   //
//      minimizing weighted completion time in Concurrent Open Shop.//
//------------------------------------------------------------------//
pair<double, double> GIPP_M2(vector<Job> &jobs, bool aggrega) ;


//------------------------------------------------------------------//
//  Method 3: GIPP with global job selection                        //
//                                                                  //
//  Description:   to explain                                       //
//------------------------------------------------------------------//

JobInfo GittinsSingleMachine(vector<Job>& jobs, int b);


pair<ActionsQuantum, double> chooseGIPPAction_M3(vector<Job>& jobs);

pair<double, double> GIPP_M3(vector<Job> &jobs);



//------------------------------------------------------------------//
//                  Value Function Evaluation                       //
//                                                                  //
//                     Functions                                    //
//------------------------------------------------------------------//

//------------------------------------//
//            Memoization             //
// Cache for already-computed states  //
//------------------------------------//

// extern map<string, double> gipp_memories;
// extern map<string, ActionsQuantum> gipp_decisions;

// extern map<string, DecisionValue> gipp_policy;



// JobTasksInPhase(Job& J, int allocation_mode = 0) : see ValueIteration.hxx


double getImmediateCostWithQuantum(const Matrix& S,const vector<Job>& jobs,int quantum);


vector<NextState> expandOneMachine(const vector<NextState>& states,int machine_id,int job_id, 
                            int quantum,const vector<Job>& jobs,int depth);

vector<NextState> generateNextStates(const Matrix& Y0,const Matrix& S0,const vector<Job>& jobs,const Action& A, int quantum,int depth);


vector<NextState> getNewStatePerPhase(const Matrix& Y,const Matrix& S,const Action& A,int quantum,const vector<Job>& jobs,int depth = 0);


//------------------------------------//
//     Gittins in MDP                 //
//------------------------------------//
vector<Job> BuildJobsFromState(const Matrix& Y,const Matrix& S,const vector<Job>& jobs_template);

ActionsQuantum policyGIPP(const Matrix& Y,const Matrix& S,const vector<Job>& jobs_template, int version=3);


// double PolicyQFunction_GIPP(const Matrix& Y,const Matrix& S,const vector<Job>& jobs,const ActionsQuantum& decision, int version=3,int depth=0);
double PolicyQFunction_GIPP(const Matrix& Y,const Matrix& S,const vector<Job>& jobs,const vector<Job>& jobs_determinist,
    const ActionsQuantum& decision,map<string, double>& gipp_memories,map<string, ActionsQuantum>& gipp_decisions,
    map<string, DecisionValue>& gipp_policy, int version=3,int depth=0);


double PolicyValueFunction_GIPP(const Matrix& Y,const Matrix& S,
    const vector<Job>& jobs,const vector<Job>& jobs_determinist,int n_machines,map<string, double>& gipp_memories,
    map<string, ActionsQuantum>& gipp_decisions,map<string, DecisionValue>& gipp_policy, int version=3,int depth=0);

// Single-instance overload (no stochastic/realized split): decision and transition both read `jobs`.
double PolicyValueFunction_GIPP(const Matrix& Y,const Matrix& S,
    const vector<Job>& jobs,int n_machines,map<string, double>& gipp_memories,
    map<string, ActionsQuantum>& gipp_decisions,map<string, DecisionValue>& gipp_policy, int version=3,int depth=0);



void verifyVIvsPE(int n, int m,const vector<Job>& jobs, map<string, DecisionValue>& policy, string filename, string mode, int version);

