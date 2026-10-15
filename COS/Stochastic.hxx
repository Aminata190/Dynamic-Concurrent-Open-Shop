#pragma once


#include "ValueIteration.hxx"
#include "GitHeur.hxx"
#include "Sincronia.hxx"

using namespace std;

namespace fs = filesystem;


//------------------------------------------------------------------//
//                  Stochastic Computing                            //
//                                                                  //
//  Expected value of the initial state for one algorithm:          //
//      "o" : VI (optimal)                                          //
//      "g" : GitHeur (GIPP, version 3)                             //
//      "s" : Sincronia (dynamic)                                   //
//      "t" : Static (Sincronia static order)                       //
//------------------------------------------------------------------//

string algoName(const string& algo);

bool isValidAlgo(const string& algo);

// jobs must already be decomposed in phases (JobTasksInPhase)
// policy is filled with the decisions of the evaluated algorithm
double stochasticValue(const vector<Job>& jobs, int m, const string& algo,
    map<string, DecisionValue>& policy, int allocation_mode = 0, int generationType = 1);


//------------------------------------------------------------------//
//                  Policies comparison                             //
//  Compare a saved VI policy with the decisions of algo ("g", "s") //
//------------------------------------------------------------------//

void compareWithVI(int N, int m, const vector<Job>& jobs, const string& algo,
    const fs::path& policy_file, const fs::path& comparison_file);
