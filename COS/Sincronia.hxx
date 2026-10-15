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


// #include "ValueIteration.hxx"
#include "GitHeur.hxx"



void printSigma(list<int> sigma, int depth);



vector<Job> UnfinishedJobs(const vector<Job>& jobs, const Matrix& Y);


//------------------------------------------------------------------------------//
// Public Method:      getTaskExpectedProcTime replace total_transmission_time  //
//------------------------------------------------------------------------------//

double getTaskExpectedProcTime(const vector<Job>& jobs,const Matrix& Y, int j, int i, int depth);

//----------------------------------------------------------------//
// Public Method:      largest_wpt                                //
//----------------------------------------------------------------//

int largest_wpt(int b,const vector<Job>& jobs, const Matrix& Y, set<int> &S_jobs, set<int> & S_b, map<int, double> & weights , int depth);

//----------------------------------------------------------------//
// Public Method: next_release_time                               //
// Method calculating the total workload (termination time) of    //
// machine i given the set S_jobs of jobs to be executed          //
//----------------------------------------------------------------//

double next_release_time(const vector<Job>& jobs,const Matrix& Y, int i, set<int> & S_jobs, int depth);

//----------------------------------------------------------------//
//Public Method: bottleneck_link                                  //
// Given the set S of jobs, this method calculates             //
// the termination time of all machines and returns the ID        //
// external of the one that terminates last. It is used in        //
// dcoflow.                                                       //
//----------------------------------------------------------------//

int bottleneck_link(const vector<Job>& jobs,const Matrix& Y, set<int> & S_jobs, double & max_end_time, int depth);


//----------------------------------------------------------------//
// Public Method:     update_sincroniaCoflows_weights             //
//----------------------------------------------------------------//

void update_sincroniaCoflows_weights(const vector<Job>& jobs,const Matrix& Y,set<int> S_jobs, map<int, double>  &weights , int b, int IK, int depth);

bool isJobComplete(int j, const Matrix& Y);

list<int> Sincronia(const vector<Job>& jobs, const Matrix& Y , int depth);


ActionsQuantum policySincronia(const vector<Job>& jobs,const Matrix& Y,bool isDynamic, list<int> sigma, int depth);



// double PolicyQFunction_Sincronia(const Matrix& Y, const Matrix& S, const vector<Job>& jobs, const ActionsQuantum& decision,bool isDynamic, list<int> sigma,int depth);
double PolicyQFunction_Sincronia(const Matrix& Y,const Matrix& S,const vector<Job>& jobs,const vector<Job>& jobs_determinist,const ActionsQuantum& decision,
    map<string, double>& sin_memories,map<string, ActionsQuantum>& sin_decisions,
    map<string, DecisionValue>& sin_policy,bool isDynamic, list<int> sigma, int depth);


// double PolicyValueFunction_Sincronia(const Matrix& Y,const Matrix& S, const vector<Job>& jobs,int n_machines,bool isDynamic, list<int> sigma,int depth);
double PolicyValueFunction_Sincronia(const Matrix& Y,const Matrix& S, const vector<Job>& jobs,const vector<Job>& jobs_determinist,int n_machines,
    map<string, double>& sin_memories,map<string, ActionsQuantum>& sin_decisions,
    map<string, DecisionValue>& sin_policy, bool isDynamic, list<int> sigma ,int depth);

// Single-instance overload (no stochastic/realized split): decision and transition both read `jobs`.
double PolicyValueFunction_Sincronia(const Matrix& Y,const Matrix& S, const vector<Job>& jobs,int n_machines,
    map<string, double>& sin_memories,map<string, ActionsQuantum>& sin_decisions,
    map<string, DecisionValue>& sin_policy, bool isDynamic, list<int> sigma ,int depth);

//------------------------------------//
//      Policies comparison           //
//----------------------------------- //

void verifyVIvsPE(int n, int m,const vector<Job>& jobs, map<string, DecisionValue>& policy, string filename, string mode);



//------------------------------------------------------------------//
//                  Sincronia Static                                //
//                                                                  //
//                                                                  //
//------------------------------------------------------------------//

list<int> SincroniaStatic(const vector<Job>& jobs, int depth);

void Schedule(vector<Job>& jobs , int depth);

void greedy_allocation(vector<Job>& jobs,list<int> & runningTasks, map<int,double> & rates, bool *busy,  set<int> & priorities, map<int, list<int> > &priority_map);



double Static_Scheduling(vector<Job>& jobs , int depth);