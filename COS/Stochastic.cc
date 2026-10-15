#include "Stochastic.hxx"



string algoName(const string& algo){
    if (algo == "o") return "Order-based";
    if (algo == "g") return "Githeur";
    if (algo == "s") return "Sincronia";
    if (algo == "t") return "Static";
    return "Unknown";
}


bool isValidAlgo(const string& algo){
    return algo == "o" || algo == "g" || algo == "s" || algo == "t";
}



//------------------------------------------------------------------//
//                  Stochastic Computing                            //
//------------------------------------------------------------------//

double stochasticValue(const vector<Job>& jobs, int m, const string& algo,
    map<string, DecisionValue>& policy, int allocation_mode, int generationType)
{
    Matrix  Y = generateInitialState(jobs);
    Matrix  S = generateInitialState(jobs);

    double vi = 0.0;

    // debug_ = true;
    if (algo == "o"){
        set<Matrix>  states = GeneratePhaseStates(jobs);

        map<string, double> memories;
        map<Matrix, Action> decisions; // stores (state, action)
        map<string, ActionsQuantum> decisions_S; // stores (string of state and phase , {action,quantum})

        vi = ValueFunction(Y, S, jobs, m, allocation_mode, memories, decisions, decisions_S, policy, 0, states, generationType);
    }
    else if (algo == "g"){
        map<string, double> gipp_memories;
        map<string, ActionsQuantum> gipp_decisions;

        vi = PolicyValueFunction_GIPP(Y, S, jobs, m, gipp_memories, gipp_decisions, policy, 3, 0);
    }
    else if (algo == "s" || algo == "t"){
        map<string, double> sin_memories;
        map<string, ActionsQuantum> sin_decisions;

        bool isDynamic = (algo == "s");
        list<int> sigma;
        if (!isDynamic)
            sigma = SincroniaStatic(jobs, 0);

        vi = PolicyValueFunction_Sincronia(Y, S, jobs, m, sin_memories, sin_decisions, policy, isDynamic, sigma, 0);
    }
    else {
        cerr << "Unknown algo: " << algo << endl;
    }

    return vi;
}



//------------------------------------------------------------------//
//                  Policies comparison                             //
//------------------------------------------------------------------//

void compareWithVI(int N, int m, const vector<Job>& jobs, const string& algo,
    const fs::path& policy_file, const fs::path& comparison_file)
{
    if (algo != "g" && algo != "s") {
        cerr << "Comparison with VI only available for algo g or s (got " << algo << ")" << endl;
        return;
    }
    if (!fs::exists(policy_file)) {
        cerr << "Policy file not found: " << policy_file << endl;
        return;
    }

    map<string, DecisionValue> mdp_policy_load;
    load_policy(mdp_policy_load, policy_file);

    bool old_debug = debug_;
    debug_ = true;
    if (algo == "g")
        verifyVIvsPE(N, m, jobs, mdp_policy_load, comparison_file, "VI", 3);
    else
        verifyVIvsPE(N, m, jobs, mdp_policy_load, comparison_file, "VI");
    debug_ = old_debug;
}
