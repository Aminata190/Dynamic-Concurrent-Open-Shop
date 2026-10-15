#pragma once

#include <math.h>
#include <algorithm>
#include <iomanip>
#include <bits/stdc++.h>
#include <vector>
#include <cmath>
#include <limits.h>
#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <cstdint>  // for uint64_t size_t serialization
#include <numeric>   // for std::iota


#include "Job.hxx"

namespace fs = filesystem;

extern bool debug_;
// extern bool global_debug_;

//------------------------------------------------------------------//
//                  Value Function Evaluation                       //
//                                                                  //
//                        Helper structs                            //
//------------------------------------------------------------------//

using Matrix = vector<vector<int>>;

using Action = vector<int>;


struct NextState {
  Matrix Y;
  Matrix S;
  double p;
};

struct NextStatePerPhase {
  Matrix S;
  double p;
};

struct ActionsQuantum
{
  Action A;
  int q;
};

struct DecisionValue {
    Action A;
    int q;
    double V;
};


// readJobsFromFile : declared in Job.hxx

int extract_index(const fs::path& p);

//------------------------------------//
//            Memoization             //
// Cache for already-computed states  //
//------------------------------------//
// extern map<string, double> memories;

// extern map<Matrix, Action> decisions; // stores (state, action)


// extern map<string, ActionsQuantum> decisions_S; // stores (string of state and phase , {action,quantum})

// extern map<string, DecisionValue> mdp_policy;


// ***************************Printing ********************************//

string indent(int d) ;

void printState(Matrix Y, int decallage= 0);

void printAction(Action A, int decallage=0);


// ******************************************************************//

void JobsDescription(const vector<Job> jobs);




//------------------------------------//
//              Encoding              //
// Serialize (State, PhaseState) into //
//  a single key                      //
//------------------------------------//
string state_key(const Matrix& Y);

void decode_state_key( const string& key,int n,int m, Matrix& Y);

string states_key(const Matrix& Y, const Matrix& S);

void decode_states_key( const string& key,int n,int m, Matrix& Y,Matrix& S);

//------------------------------------//
//              Records              //
// Saving informations               //
//------------------------------------//

void save_policy(const map<string, DecisionValue>& mdp_policy, const string& filename);

void load_policy(map<string, DecisionValue>& mdp_policy, const string& filename);

void PrintPolicy(int n, int m,const map<string, DecisionValue>& mdp_policy);


//------------------------------------//
//              Decomposition         //
// Jobs decomposition in phases       //
//------------------------------------//

double getBeta(const map<int, double> &dist, int state);

void JobTasksInPhase(Job& J, int allocation_mode = 0);


//------------------------------------//
//              Actions              //
//------------------------------------//

//------------------------------------ ------//
//     Recursives Actions functions         //
void generateActionsRecursive(int machine_idx, Action& current_action, int n_jobs, int n_machines, Matrix& all_actions) ;

Matrix getAllActionsRecursive(int n, int m);


//------------------------------------ --------------------//
//          Order Actions functions                        //
// Generate all permutations of indices [0, 1, ..., N-1].  //
// Each permutation is an order in which to process the N jobs.//
//
Matrix getAllPermutations(int N);

pair<Action, bool> ActionChoosed(int v, const Matrix& Y, Action A0);

Matrix getAllActionsFromOrder(const Matrix& Y, int depth);

set<Action> generateSuccessiveOrderActions(const Matrix& Y, int depth);

//------------------------------------//
//     Actions Control Functions      //
//------------------------------------//

bool actionMakesProgressOnAnyMachine(const Matrix& Y, const Matrix& S, const vector<Job>& jobs, const Action& A);

bool invalidAction(const  Action& action);

bool machineHasUnfinishedTask(const Matrix& Y,const Matrix& S, int machine_id);

bool violatesNoIdleConstraint(const Matrix& Y, const Matrix& S, const Action& A);

bool isUselessAction(const Matrix& Y, const Matrix& S, const Action& A);



Matrix generateInitialState(const vector<Job>& jobs) ;

// Printing Different States
vector<NextState> getNewStatePerPhase(const Matrix& Y,const Matrix& S, const Action& A,int quantum,const vector<Job>& jobs,int depth);



//------------------------------------//
//              Immediat cost        //
//------------------------------------//
double getImmediatCost(const Matrix& S, const vector<Job>& jobs);

double getImmediatCostPerPhase(const Matrix& Y,const Matrix& S,const vector<Job>& jobs, const Action& A);

pair <double, int > getImmediatCostPerSemiPhase(const Matrix& Y,const Matrix& S,const vector<Job>& jobs, const Action& A);

pair<double, int > getCostAndQuantum(const Matrix& Y,const Matrix& S, const vector<Job>& jobs, const Action& A,int allocation_mode);




bool isSameState(const NextState& ns, const Matrix& Y, const Matrix& S);

double solveSelfLoop(double q_partial, double p_self);

void expandGeometricTask(const Matrix& Y, const Matrix& S, double p, int job_id, int machine_id,
                         int quantum, const Task& task, vector<NextState>& out, int depth);

vector<NextState> expandOneMachine(const vector<NextState>& states,int machine_id,int job_id, int quantum,const vector<Job>& jobs, int allocation_mode,int depth);


vector<NextState> generateNextStates(const Matrix& Y0,const Matrix& S0,const vector<Job>& jobs,const Action& A, int quantum, int allocation_mode ,int depth);



// pair<double, int>  QFunction(const  Matrix& Y, const  Matrix& S, const  vector<Job>& jobs, const  Action& A,int allocation_mode, int depth , set<Matrix> phasesStates, int generationType);


// double ValueFunction(const  Matrix& Y,const  Matrix& S, const  vector<Job>& jobs, int n_machines, int allocation_mode, int depth, set<Matrix> phasesStates, int generationType);

pair<double, int>  QFunction(const  Matrix& Y, const  Matrix& S, const  vector<Job>& jobs, const vector<Job>& jobs_determinist, const  Action& A,int allocation_mode,
  map<string, double>& memories,map<Matrix, Action>& decisions,map<string, ActionsQuantum>& decisions_S,
 map<string, DecisionValue>& mdp_policy, int depth , set<Matrix> phasesStates, int generationType);


// double ValueFunction(const  Matrix& Y,const  Matrix& S, const  vector<Job>& jobs, int n_machines, int allocation_mode, int depth, set<Matrix> phasesStates, int generationType);

double ValueFunction(const Matrix& Y, const Matrix& S, const vector<Job>& jobs, const vector<Job>& jobs_determinist, int n_machines,int allocation_mode,
 map<string, double>& memories,map<Matrix, Action>& decisions,map<string, ActionsQuantum>& decisions_S,
 map<string, DecisionValue>& mdp_policy,int depth, set<Matrix> phasesStates, int generationType);

// Single-instance overload (no stochastic/realized split): decision and transition
double ValueFunction(const Matrix& Y, const Matrix& S, const vector<Job>& jobs, int n_machines,int allocation_mode,
 map<string, double>& memories,map<Matrix, Action>& decisions,map<string, ActionsQuantum>& decisions_S,
 map<string, DecisionValue>& mdp_policy,int depth, set<Matrix> phasesStates, int generationType);



set<Matrix> GeneratePhaseStates(const vector<Job>& jobs);





void printDecisionsTrajectoryPerPhase_rec(const Matrix& Y,const Matrix& S,const vector<Job>& jobs, const map<string, ActionsQuantum>& decisions_S,
                                          const set<string>& visited,int depth,double pathProb, map<string, DecisionValue>& mdp_policy_final);

void printDecisionsTrajectoryPerPhase(const vector<Job>& jobs,const map<string, DecisionValue>& mdp_policy, map<string, DecisionValue>& mdp_policy_final);



