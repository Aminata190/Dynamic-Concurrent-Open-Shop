#pragma once


#include "ValueIteration.hxx"
#include "GitHeur.hxx"
#include "Sincronia.hxx"

using namespace std;

namespace fs = filesystem;

struct SAAResult {
    double avg;
    double std_dev;
    double min_val;
    double max_val;
};

struct SAAResultBatch {
    double avg;
    double std_dev;
    double min_val;
    double max_val;
    int    usedInstances;
};

vector<Job> generate_determinist(const vector<Job>& jobs);

double determinstJobsVI(const vector<Job>& jobs_stochastic, const vector<Job>& jobs_determinist, string algo, list<int> staticSigma);

SAAResult computeAverageDeterministicValue(const vector<Job>& jobs, int nbInstances, string algo);

SAAResultBatch computeAverageDeterministicValuePerBatch(const vector<Job>& jobs,int nbInstancesMax,const string& algo,int batchSize = 100,int minInstances = 500,double tol = 1e-4,int requiredStableBatches = 3);