#include <random>

#include "SAA.hxx"

// Setup random engine 
// mt19937 rng(random_device{}());
// thread_local mt19937 rng(random_device{}());
static thread_local mt19937 rng(123456u);

vector<Job> generate_determinist(const vector<Job>& jobs) {  
    vector<Job> gen_jobs;

    for (const auto& J : jobs) {  
        int m = J.getNbTask();
        Job Jo = Job(J.getId(), J.getWeight(), m);

        for (int t = 0; t < m; t++) {
            const map<int, double>& distribution = J.getTask(t).getDistribution();

            vector<int>    values;
            vector<double> probs;
            for (const auto& [val, prob] : distribution) {
                values.push_back(val);
                probs.push_back(prob);
            }

            // discrete_distribution<int> dist(values.begin(), values.end());
            // pass probs, not values
            discrete_distribution<int> dist(probs.begin(), probs.end());
            int sampledValue = values[dist(rng)];

            
            
            Task T = Task(t, map<int,double>{{sampledValue, 1.0}});
            Jo.setTask(t, T);
        }

        gen_jobs.push_back(Jo);
    }

    return gen_jobs;  
}





double determinstJobsVI(const vector<Job>& jobs_stochastic, const vector<Job>& jobs_determinist, string algo, list<int> staticSigma){
    Matrix  Y = generateInitialState(jobs_determinist);
    Matrix  S = generateInitialState(jobs_determinist);

    set<Matrix>  states = GeneratePhaseStates(jobs_determinist);

    int N = Y.size();
    int m = Y[N-1].size();
    int allocation_mode = 0; // semi-phase
    int generationType = 1;  // order-based

    // Phases (Task::phases_) of the sampled realization: this is the only thing state
    // transitions (expandOneMachine/generateNextStates) read, driving the actual duration of
    // each task. jobs_stochastic is passed through untouched to the decision functions
    // (getTaskExpectedProcTime/getRemaining for Sincronia, BuildJobsFromState/Gittins index for
    // GIPP), which only ever read Task::distribution, never the realized value.
    vector<Job> jobs_realized = jobs_determinist;
    for (auto &J : jobs_realized)
        JobTasksInPhase(J, allocation_mode);

    double vi;
    if (algo == "o"){

        map<string, double> memories;
        map<Matrix, Action> decisions; // stores (state, action)
        map<string, ActionsQuantum> decisions_S; // stores (string of state and phase , {action,quantum})
        map<string, DecisionValue> mdp_policy;

        vi = ValueFunction(Y, S, jobs_stochastic, jobs_realized, m,allocation_mode,memories,decisions, decisions_S, mdp_policy,0, states,generationType);

    }
    else if(algo == "g"){

        map<string, double> gipp_memories;
        map<string, ActionsQuantum> gipp_decisions;
        map<string, DecisionValue> gipp_policy;
        vi = PolicyValueFunction_GIPP(Y, S, jobs_stochastic, jobs_realized, m,gipp_memories, gipp_decisions, gipp_policy, 3,0);
    }
    else{
        map<string, double> sin_memories;
        map<string, ActionsQuantum> sin_decisions;
        map<string, DecisionValue> sin_policy;


        list<int> sigma;
        bool isDynamic;
        if (algo == "s") {
            isDynamic = 1;
        }
        else if (algo == "t") {
            isDynamic = 0;
            sigma = staticSigma;  //SincroniaStatic(jobs, 0);

        }

        vi = PolicyValueFunction_Sincronia(Y, S, jobs_stochastic, jobs_realized, m , sin_memories, sin_decisions, sin_policy, isDynamic, sigma ,0);

    }


    return vi;
}



SAAResult computeAverageDeterministicValue(const vector<Job>& jobs, int nbInstances, string algo){

    double min_val  = numeric_limits<double>::infinity();
    double max_val  = 0.0;
    double somme    = 0.0;
    double somme_sq = 0.0;  

    list<int> staticSigma;

    #pragma omp parallel for reduction(+:somme, somme_sq) reduction(min:min_val) reduction(max:max_val)
    for (int i = 0; i < nbInstances; i++){
        vector<Job> deterministjobs = generate_determinist(jobs);
        double vi = determinstJobsVI(jobs, deterministjobs, algo, staticSigma);



        #pragma omp critical
        cout << "\n N° "<< i << ": Jobs Description " << endl;
        JobsDescription(deterministjobs);
        cout <<"\t vi= " << vi << endl;
        // cout << "Thread " << omp_get_thread_num() 
        //  << " → instance " << i << endl;

        somme    += vi;
        somme_sq += vi * vi;  
        if (vi > max_val) max_val = vi;
        if (vi < min_val) min_val = vi;
    }

    double avg  = somme / nbInstances;
    double var  = (somme_sq / nbInstances) - (avg * avg);  // E[X²] - E[X]²
    double std_dev = sqrt(var);

    cout << "Avg    = " << avg     << endl;
    cout << "StdDev = " << std_dev << endl;
    cout << "Min    = " << min_val << endl;
    cout << "Max    = " << max_val << endl;


    return {avg, std_dev, min_val, max_val};
}

list<int> StaticSigmaComputation(const vector<Job>& jobs){
    return SincroniaStatic(jobs, 0);
}


SAAResultBatch computeAverageDeterministicValuePerBatch(const vector<Job>& jobs,int nbInstancesMax,const string& algo,int batchSize ,int minInstances ,double tol ,int requiredStableBatches )
{
    // debug_ = true; // to drop
    double mean = 0.0;
    double M2 = 0.0;
    double min_val = numeric_limits<double>::infinity();
    double max_val = -numeric_limits<double>::infinity();

    int n = 0;
    int stableCount = 0;
    double prevBatchMean = numeric_limits<double>::infinity(); // +infiny

    const double rtol = tol;    // relative tolerance (fraction of the mean)
    const double atol = 1e-6;   // absolute floor, keeps the test meaningful when mean ~ 0

    list<int> staticSigma;
    if (algo == "t"){
        if (debug_) cerr << "Static sigma computing once and for all" << endl;
        staticSigma = StaticSigmaComputation(jobs);
    }
    
    for (int start = 0; start < nbInstancesMax; start += batchSize) {
        int end = min(start + batchSize, nbInstancesMax);

        for (int i = start; i < end; ++i) {
            vector<Job> deterministjobs = generate_determinist(jobs);
            if (debug_){
                cout << "\n ----------N° " << i << ": Jobs Description\n";
                JobsDescription(deterministjobs);
            } 

            double vi = determinstJobsVI(jobs, deterministjobs, algo,staticSigma);

            ++n;

            double delta = vi - mean;
            mean += delta / n; // incremental mean
            double delta2 = vi - mean;
            M2 += delta * delta2; // second central moment -to calculate the variance

            if (vi < min_val) min_val = vi;
            if (vi > max_val) max_val = vi;

            // cout << "\n ----------N° " << i << ": Jobs Description\n";
            // JobsDescription(deterministjobs);
            // cout << "\tvi = " << vi << '\n';
        }

        double variance = (n > 1) ? (M2 / (n - 1)) : 0.0;
        double std_dev = sqrt(variance);

        // cout << "\nAfter " << n << " instances:\n";
        // cout << "Avg    = " << mean << '\n';
        // cout << "StdDev = " << std_dev << '\n';
        // cout << "Min    = " << min_val << '\n';
        // cout << "Max    = " << max_val << '\n';

        if (n >= minInstances) {

            double diff  = abs(mean - prevBatchMean);
            double scale = abs(prevBatchMean);
            bool converged = isfinite(prevBatchMean) &&
                             diff <= atol + rtol * scale;   // relative test, atol guards mean ~ 0: diff/scale < rtol + atol

            if (converged) {
                ++stableCount;
            } else {
                stableCount = 0;
            }

            if (stableCount >= requiredStableBatches) {
                if (debug_)  cout << "Early stopping: mean stabilized.\n";
                return {mean, std_dev, min_val, max_val, n};
            }
        }

        prevBatchMean = mean;
    }

    double variance = (n > 1) ? (M2 / (n - 1)) : 0.0;
    double std_dev = sqrt(variance);

    return {mean, std_dev, min_val, max_val, n};
}



// int main() {
   
//     string filename ="./Analysis/Param_data/N3_m2/N3_m2_1.txt";  //"./data/N2_m2/N2_m2_2.txt";
//     vector<Job> jobs = readJobsFromFile(filename);

//     cout << "\n******** Jobs Description *******" << endl;
//     JobsDescription(jobs);


//     // // for (int i=0; i<10;i++){
//     // //     vector<Job> jobs1 = generate_determinist(jobs);
//     // //     cout << "\n******** Determinst Jobs I_"<<i<<"  *******" << endl;
//     // //     JobsDescription(jobs1);
//     // // }

//     // vector<Job> deterministjobs = generate_determinist(jobs);
//     // JobsDescription(deterministjobs);
//     // determinstJobsVI(deterministjobs);

//     computeAverageDeterministicValue(jobs,  10000);
// }