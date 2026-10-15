#include <math.h>
#include <algorithm>
#include <iomanip>
#include <bits/stdc++.h>
#include <vector>
#include <cmath>

#include "ValueIteration.hxx"


bool debug_;

// readJobsFromFile : see Job.cc



int extract_index(const fs::path& p) {
    string s = p.filename().string();
    // Strip extension
    size_t dot = s.find('.');
    if (dot != string::npos) s = s.substr(0, dot);
    // After "N2_M2_"
    size_t last_underscore = s.find_last_of('_');
    if (last_underscore == string::npos) return 0;
    try {
        return stoi(s.substr(last_underscore + 1));
    } catch (...) {
        return 0;
    }
}

//------------------------------------//
//      Printing states               //
//------------------------------------//
string indent(int d) {
    return string(d*3 , ' ');
}

void printState(Matrix Y, int depth){
    cout << indent(depth) << "[";
    for ( auto j = 0; j<Y.size(); j++){
        
        for (int i =0; i<Y[j].size() ; i++){
            cout << Y[j][i] << " ";
            // cout << "\ty("<<j<< "," << i <<")= " << Y[j][i];
        }
        if (j== Y.size() -1) cout <<"]" << endl;
        else {
            cout << endl;  
            cout << indent(depth);
        }
        
    }
    
}


void printAction(Action A, int decallage){
    cout << string(decallage, '\t') << "[";
    for ( auto a = 0; a<A.size(); a++){
        cout << A[a] ;       
        if (a== A.size() -1) cout <<"]";
        else cout << " ";          
    }
    
}

void printActionMatrix(Matrix actions, int depth){
    cout << indent(depth) << "| ";
    for (auto a:actions) printAction(a);
}

void printPhase(vector<Phase> phases, int decallage){ 
    cout << string(decallage, '\t');   
    for ( auto ph: phases){
        cout  << "("<< ph.duration << " , " << ph.beta_prob << "); " ;               
    }
    cout << endl;
}


// ******************************************************************//

void JobsDescription(const vector<Job> jobs){
    for (auto &J : jobs) {
        cout << "Job: " << J.getId()
            << " , weight=" << J.getWeight()
            << " , nbTask=" << J.getNbTask()
            << ":\n";
        for (int t = 0; t < J.getNbTask(); t++) {
            cout << "  Task: " << J.getTaskId(t) << "\n";
            if (J.getTask(t).isGeometric()) {
                cout << "\t | alpha=" << J.getTask(t).getAlpha() << endl;
                continue;
            }
            for (auto &p : J.getTask(t).getDistribution()) {
                cout << "\t | time=" << p.first
                    << " prob=" << p.second << endl;
            }
        }
    }
}



// NextState argmaxProb(const vector<NextState>& next_states) {
//     int best = 0;
//     for (int i = 1; i < (int)next_states.size(); ++i)
//         if (next_states[i].p > next_states[best].p) best = i;
//     return next_states[best];
// }

// bool isTerminal(const Matrix& S) {
//     for (const auto& row : S)
//         for (int x : row)
//             if (x != -1) return false;
//     return true;
// }


//------------------------------------//
//            Memoization             //
// Cache for already-computed states  //
//------------------------------------//
// map<string, double> memories;

// map<Matrix, Action> decisions; // stores (state, action)

// map<string, ActionsQuantum> decisions_S; // stores (string of state and phase , {action,quantum})

// map<string, DecisionValue> mdp_policy; // stores (string of state and phase , {action,quantum})


//------------------------------------//
//              Encoding              //
// Serialize (State, PhaseState) into //
//  a single key                      //
//------------------------------------//
string state_key(const Matrix& Y) {
    ostringstream oss;
    
    // Encode Y
    oss << "Y:";
    for (const auto& row : Y) {
        for (int x : row) {
            oss << (x + 1000) << ",";  // +100 → -1 becomes 99
        }
    }
    
    return oss.str();
}

void decode_state_key( const string& key,int n,int m, Matrix& Y) {
    Y.assign(n, vector<int>(m));

    stringstream y_stream(key.substr(2));  // skip "Y:"

    string token;

    // Decode Y
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            getline(y_stream, token, ',');
            Y[i][j] = stoi(token) - 1000;
        }
    }

}


string states_key(const Matrix& Y, const Matrix& S) {
    ostringstream oss;
    
    // Encode Y
    oss << "Y:";
    for (const auto& row : Y) {
        for (int x : row) {
            oss << (x + 1000) << ",";  // +100 → -1 becomes 99
        }
    }
    
    // Encode S (phases)
    oss << "|S:";
    for (const auto& row : S) {
        for (int x : row) {
            oss << (x + 1000) << ",";  // +100 for -1
        }
    }
    
    return oss.str();
}



void decode_states_key( const string& key,int n,int m, Matrix& Y,Matrix& S) {
    Y.assign(n, vector<int>(m));
    S.assign(n, vector<int>(m));

    // Split into Y and S parts
    size_t split_pos = key.find("|S:");
    string Y_part = key.substr(2, split_pos - 2);     // skip "Y:"
    string S_part = key.substr(split_pos + 3);        // skip "|S:"

    stringstream y_stream(Y_part);
    stringstream s_stream(S_part);

    string token;

    // Decode Y
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            getline(y_stream, token, ',');
            Y[i][j] = stoi(token) - 1000;
        }
    }

    // Decode S
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            getline(s_stream, token, ',');
            S[i][j] = stoi(token) - 1000;
        }
    }
}


//------------------------------------//
//              Records              //
// Saving informations               //
//------------------------------------//

void save_policy(const map<string, DecisionValue>& mdp_policy, const string& filename) {
    ofstream file(filename, ios::binary);
    if (!file) {
        cout << "Error: No permission/path incorrect"<< endl;
        return;
        }

    // Write map size
    uint64_t size = mdp_policy.size();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));

    for (const auto& pair : mdp_policy) {
        // Write key (string): size + data
        uint64_t key_size = pair.first.size();
        file.write(reinterpret_cast<const char*>(&key_size), sizeof(key_size));
        file.write(pair.first.data(), key_size);

        // Write value: A.size + A.data + q + V
        const auto& dv = pair.second;
        uint64_t a_size = dv.A.size();
        file.write(reinterpret_cast<const char*>(&a_size), sizeof(a_size));
        file.write(reinterpret_cast<const char*>(dv.A.data()), a_size * sizeof(int));
        file.write(reinterpret_cast<const char*>(&dv.q), sizeof(dv.q));
        file.write(reinterpret_cast<const char*>(&dv.V), sizeof(dv.V));
    }
    file.close();
}



void load_policy(map<string, DecisionValue>& mdp_policy_l, const string& filename) {
    ifstream file(filename, ios::binary);
    if (!file) {
        cerr << "Error:" << endl;
        return;
    }

    // Read map size
    uint64_t size;
    file.read(reinterpret_cast<char*>(&size), sizeof(size));

    mdp_policy_l.clear();
    for (uint64_t i = 0; i < size; ++i) {
        // Read key
        uint64_t key_size;
        file.read(reinterpret_cast<char*>(&key_size), sizeof(key_size));
        string key(key_size, '\0');
        file.read(&key[0], key_size);

        // Read value
        DecisionValue dv;
        uint64_t a_size;
        file.read(reinterpret_cast<char*>(&a_size), sizeof(a_size));
        dv.A.resize(a_size);
        file.read(reinterpret_cast<char*>(dv.A.data()), a_size * sizeof(int));
        file.read(reinterpret_cast<char*>(&dv.q), sizeof(dv.q));
        file.read(reinterpret_cast<char*>(&dv.V), sizeof(dv.V));

        mdp_policy_l[key] = move(dv);
    }
    file.close();
}

void PrintPolicy(int n, int m,const map<string, DecisionValue>& mdp_policy){
    for (const auto& p : mdp_policy) {
        Matrix Y, S;
        decode_states_key(p.first, n, m, Y, S);
        // decode_state_key(p.first, n, m, Y);

        cout << "\nY:"<< endl;
        printState(Y);
        cout << "S:"<< endl;
        printState(S);

        cout << "A:";printAction(p.second.A);

        cout << " - q:"<<p.second.q << " - V= " << p.second.V <<endl;
    }
}




//------------------------------------//
//      Conditional Probability β     //
// β_i = Pr(P > p_i | P > p_{i-1}) //
//------------------------------------//
double getBeta(const map<int, double> &dist, int state) {

    vector<pair<int, double>> values(dist.begin(), dist.end());
    int n = values.size();

    double num = 0.0, denom = 0.0;


    for (int i = state; i < n; ++i)  denom += values[i].second;

    for (int i = state + 1; i < n; ++i) num += values[i].second;
    // if (debug_)
    //     cerr << "num= " << num << ", denom= " << denom << endl;

    if (denom == 0.0) return 0.0;

    return num / denom;  // S(p_{i+1}) / S(p_i)
}



// allocation_mode ={  -1 : unit allocation
//                      0 : semi phase allocation
//                      1 : phase allocation}
//------------------------------------//
//      Job decomposition in Phases   //
// Cululative decomposition           //
//------------------------------------//
void JobTasksInPhase(Job& J, int allocation_mode) {

    for (int t = 0; t < J.getNbTask(); t++) {
        auto &task = J.getTask(t);

        // Geometric task: a single phase of one time unit which loops on itself
        // (finish with alpha, otherwise same state; see expandGeometricTask)
        if (task.isGeometric()) {
            task.setPhases({ {1, 1.0 - task.getAlpha()} });
            continue;
        }

        map<int, double> dist = task.getDistribution();

        vector<Phase> phases;

        vector<pair<int,double>> values(dist.begin(), dist.end());
        int m = values.size();

        for (int i = 0; i < m; ++i) {
            int processing_time = values[i].first;
            double prob = values[i].second;                

            double beta = getBeta(dist, i); // probability of NOT finishing after this phase
            phases.push_back({processing_time, beta});


        }
        task.setPhases(phases);

        if(debug_){
            cout << "\nJob " << J.getId() << " (Task " << t << ")\n";
            cout << fixed << setprecision(3);
            for (int i = 0; i < task.getPhases().size(); ++i)
                cout << "Phase " << i + 1
                        << " | duration=" << task.getPhases()[i].duration
                        << " | beta =" << task.getPhases()[i].beta_prob << endl;
            }       
    }

}



//------------------------------------//
//     Actions functions              //
//------------------------------------//

//------------------------------------ ------//
//     Recursives Actions functions         //

void generateActionsRecursive(int machine_id, Action& current_action, int n_jobs, int n_machines, Matrix& all_actions) {
    
    if (machine_id == n_machines) {
        all_actions.push_back(current_action);
        return;
    }
    
    // For this machine, try all choices: -1, 0, 1, ..., n_jobs-1
    for (int job_id = -1; job_id < n_jobs; ++job_id) {
        current_action[machine_id] = job_id;
        generateActionsRecursive(machine_id + 1, current_action, n_jobs, n_machines, all_actions);
    }
}

Matrix getAllActionsRecursive(int n, int m) {
    Matrix all_actions;
    Action current_action(m);
    
    generateActionsRecursive(0, current_action, n, m, all_actions);
    return all_actions;
}

//------------------------------------ --------------------//
//          Order Actions functions                        //
// Generate all permutations of indices [0, 1, ..., N-1].  //
// Each permutation is an order in which to process the N jobs.//
//
Matrix getAllPermutations(int N) {
    // Vector of indices: 0, 1, ..., N-1
    Action indices(N);
    iota(indices.begin(), indices.end(), 0); //microseconds

    Matrix permutations;
    do {
        permutations.push_back(indices); 
    } while (next_permutation(indices.begin(), indices.end())); //lexicographical permutation

    return permutations;
}


pair<Action, bool> ActionChoosed(int v, const Matrix& Y, Action A0) {
    const int m = A0.size();
    bool changed = false;

    for (int i = 0; i < m; ++i) {
        if (A0[i] == -1 && Y[v][i] != -1) {// machine k "available" & job v still has task k to execute
            A0[i] = v;              
            changed = true;
            
        }
    }

    return {A0, changed};
}

bool containsAction(const Matrix& allActions, const Action& target) {
    auto it = find(allActions.begin(), allActions.end(), target);
    return it != allActions.end();
}

Matrix getAllActionsFromOrder(const Matrix& Y, int depth) {
    const int N = Y.size();
    const int M = Y[0].size();  

    Matrix allPerms = getAllPermutations(N);
    if(debug_) {cout <<indent(depth)<<"| All permutations order: "<<endl;printState(allPerms, depth);}

    Matrix allActions;
    
    // Base action: all machines initially "unassigned"
    Action A0(M, -1);

    for (const Action& order : allPerms) {
        Action A = A0;  // reset for each permutation
        
        if(debug_) {cout<<"\n" <<indent(depth) << " |Order: "; printAction(order);}
        // cout << "\nInitial action: " ;printAction(A);
        for (int v : order) {
            auto [newAction, anyChange] = ActionChoosed(v, Y, A);
            // Optionally filter: only keep actions that actually change something
            if (anyChange) {
                A = move(newAction);
                if(!containsAction(allActions, A)){
                    if(debug_) {cout <<"\n"<<indent(depth)<< "|Action choosed not in allActions: " ;
                                 printAction(A); cout << endl;}
                    allActions.push_back(A);
                }
            }
        }

        
    }

    return allActions;
}



//------------------------------------ --------------------//
//          Order Actions functions (Successive generation)//
//From all indices [0, 1, ..., N-1], generate successively //
// possible actions to be executed from a given state.     //
Action eraseValue(const Action& priority_list, int v) {
    Action next_priority_list = priority_list;
    auto it = find(next_priority_list.begin(), next_priority_list.end(), v);
    if (it != next_priority_list.end()) {
        next_priority_list.erase(it);
    }
    return next_priority_list;
}

void nextPriorityAction(const Matrix& Y,const Action& A, int i, int v, const Action& Priority_list, set<Action>& resultActions){
    if (Y[v][i] != -1){
        Action nextA = A;
        nextA[i] = v;
        resultActions.erase(A); // remove old A
        resultActions.insert(nextA);
        
    }
    else{ // v cannot be assigned, try lower priority jobs
        Action lowerPriority = eraseValue(Priority_list, v);
        if (!lowerPriority.empty()){
            for (int next_v : lowerPriority){
               nextPriorityAction(Y,A, i, next_v, lowerPriority, resultActions);
            }
            
        }
        // else nothing
        
    }
}



set<Action> generateSuccessiveOrderActions(const Matrix& Y, int depth) {
    const int N = Y.size();
    const int M = Y[0].size();

    Action A0(M, -1);

    Action indices(N);
    iota(indices.begin(), indices.end(), 0);

    set<Action> globalActions;

    
    

    for (int v : indices) {
        Action priority_list = indices;
        if (debug_) {
            cout << indent(depth) << "| === Top priority job: " << v
                 << " | priority_list: "; printAction(priority_list); cout <<endl;
        }

        set<Action> allActions = {A0};

        
        for (int i = 0; i < M; i++) {

            // Snapshot of current actions to iterate over safely !Important
            set<Action> snapshot = allActions;
            set<Action> nextActions = allActions;

            for (const Action& A : snapshot) {
                if (A[i] != -1) continue; 

                if (Y[v][i] != -1) {
                    Action newA = A;
                    newA[i] = v;
                    nextActions.erase(A);
                    nextActions.insert(newA);

                    if (debug_) {
                        cout << indent(depth) << "| Assigned job " << v
                             << " to machine " << i << ": "; printAction(newA);cout << endl;
                    }
                } else {
                    // v cannot take machine i → try lower priority jobs
                    Action lowerPriority = eraseValue(priority_list, v);

                    if (debug_) {
                        cout << indent(depth) << "| Job " << v
                             << " cannot take machine " << i
                             << " | lower priority size: "
                             << lowerPriority.size() << endl;
                    }

                    for (int nv : lowerPriority) {
                        nextPriorityAction(Y, A, i, nv, lowerPriority, nextActions);

                        if (debug_) {
                            cout << indent(depth) << "| After trying nv=" << nv << ": ";
                            for (auto& a : nextActions) printAction(a);
                        }
                    }
                }
            }

            allActions = nextActions;

            if (debug_) {
                cout << indent(depth) << "| Actions after machine " << i << ": ";
                for (auto& a : allActions) printAction(a);
                cout << endl;
            }
        }

        if (debug_) {
            cout << indent(depth) << "| Job " << v << " done. Actions: ";
            for (auto& a : allActions) printAction(a);
            cout << endl;
        }

        // Accumulate into global set: set have unique element
        for (const auto& a : allActions)
            globalActions.insert(a);

        // v is consumed: remove from priority list for next iterations
        priority_list = eraseValue(priority_list, v);
    }

    return globalActions;
}



//------------------------------------//
//     Actions Control Functions      //
//------------------------------------//
bool actionMakesProgressOnAnyMachine(const Matrix& Y, const Matrix& S, const vector<Job>& jobs, const Action& A)
{
    int n_machines = (int)A.size();
    for (int m = 0; m < n_machines; ++m) {
        int j = A[m];
        if (j == -1) continue;                 // idle -> no progress

        if (S[j][m] == -1) continue;           // already finished

        // If it is a valid active task, then one step will change Y or finish it
        int phase_id = S[j][m];
        const Task& task = jobs[j].getTask(m);
        const auto& phases = task.getPhases();
        const Phase& phase = phases[phase_id];

        int proc_time = Y[j][m];
        if (proc_time == -1) continue;         // already finished in Y (shouldn't happen if consistent)

        // One tick will either increment proc_time or finish -> progress
        return true;
    }
    return false;
}


bool invalidAction(const  Action& action){
    // invalid if ALL entries are -1
    for (int a : action) {
        if (a != -1) return false; // valid
    }
    return true; // invalid
}

bool machineHasUnfinishedTask(const Matrix& Y,const Matrix& S, int machine_id) {
    
    for (int j = 0; j < (int)S.size(); ++j) {
        if (S[j][machine_id] != -1 || Y[j][machine_id] != -1) {
            return true;
        }
    }
    return false;
}

// Violation : If some job is still unfinished on this machine,than this machine cannot be idle.
bool violatesNoIdleConstraint(const Matrix& Y, const Matrix& S, const Action& A) {
    int m = A.size();

    for (int i = 0; i < m; ++i) {
        bool hasUnfinished = machineHasUnfinishedTask(Y,S, i);
        
        if (hasUnfinished && A[i] == -1) {
            return true;
        }
    }
    return false;
}


bool isUselessAction(const Matrix& Y, const Matrix& S, const Action& A){
    int m = A.size();
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < (int)S.size(); ++j) {
            if (S[j][i] == -1 && A[i]!= -1 ) {
                return true;
            }
        }
    }
    return false;
}


//------------------------------------//
//     State transition functions     //
//------------------------------------//

Matrix generateInitialState(const vector<Job>& jobs) {
    Matrix Y;
    Y.reserve(jobs.size());

    for (const auto& job : jobs) {
        int m = job.getNbTask();
        Y.emplace_back(m, 0);  
    }
    return Y;
}


// To print Different States
vector<NextState> getNewStatePerPhase(const Matrix& Y,const Matrix& S,
                                    const Action& A,int quantum,const vector<Job>& jobs,int depth)
{
    vector<NextState> states = { {Y, S, 1.0} };

    for (int m = 0; m < (int)A.size(); ++m) {
        states = expandOneMachine(states, m, A[m], quantum, jobs,0, depth + 1); // true code
        // states = expandOneMachine(states, m, A[m], quantum, jobs,0, depth);
    }

    return states;
}



double getImmediatCost(const Matrix& S, const vector<Job> &jobs){
    double sum_w = 0.0;
    // if (debug_) cout << endl;
    for (int j = 0 ; j<S.size(); j++){
        bool finished = true;
        for( int i=0; i< S[j].size(); i++){
            if (S[j][i] != -1) { // Check jobs not finished
                finished = false;                
            }
        }
        if (! finished) {
            sum_w += jobs[j].getWeight();   
            // if (debug_) cout << "\tJob " << j << " not finished" << endl;        
        }

    }

    return sum_w ;

}

double getImmediatCostPerPhase(const Matrix& Y,const Matrix& S,const vector<Job>& jobs, const Action& A) {
    double sum_w = 0.0;
    int max_q = 0;  // -1 → 0
    
    for (int j = 0; j < (int)S.size(); j++) {
        bool finished = true;
        for (int i = 0; i < (int)S[j].size(); i++) {
            if (S[j][i] != -1) {
                finished = false;
            }
        }
        if (!finished) {
            sum_w += jobs[j].getWeight();
        }
    }
    
    // Get the max quantum execution over all tasks
    for ( int i = 0; i < (int)A.size(); i++){
        if (A[i] != -1) {
            int j = A[i];
            const Task& task = jobs[j].getTask(i); 
            int phase_id = S[j][i];
            if (phase_id > -1 && phase_id < (int)task.getPhases().size()) {
                Phase phase = task.getPhases()[phase_id];

                int remaining = (int)phase.duration - Y[j][i];

                max_q = max(max_q, remaining);
            }
        }
    }

    return sum_w * max_q;
}

pair <double, int > getImmediatCostPerSemiPhase(const Matrix& Y, const Matrix& S,const vector<Job>& jobs, const Action& A) {
    double sum_w = 0.0;
    int min_q = INT_MAX;  // -1 → 0
    
    for (int j = 0; j < (int)S.size(); j++) {
        bool finished = true;
        for (int i = 0; i < (int)S[j].size(); i++) {
            if (S[j][i] != -1) {
                finished = false;
            }
        }
        if (!finished) {
            sum_w += jobs[j].getWeight();
        }
    }
    // Get the min quantum execution over all tasks
    for ( int i = 0; i < (int)A.size(); i++){
        if (A[i] != -1) {
            int j = A[i];
            const Task& task = jobs[j].getTask(i); 
            int phase_id = S[j][i];

            if (phase_id > -1 && phase_id < (int)task.getPhases().size()) {
                Phase phase = task.getPhases()[phase_id];
                int remaining = (int)phase.duration - Y[j][i];
                min_q = min(min_q, remaining);
            }
        }
    }
    if (min_q == INT_MAX) min_q = 0;
    
    return make_pair(sum_w * min_q, min_q);
}

//------------------------------------//
//  Geometric task (memoryless)       //
//  Y and S stay at 0 while the task  //
//  runs: after q time units          //
//   - finished : Y=S=-1, proba 1-(1-alpha)^q
//   - otherwise: same state, proba (1-alpha)^q
//------------------------------------//
void expandGeometricTask(const Matrix& Y, const Matrix& S, double p, int job_id, int machine_id,
                         int quantum, const Task& task, vector<NextState>& out, int depth)
{
    double p_stay = pow(1.0 - task.getAlpha(), quantum);

    auto Y_fin = Y;
    auto S_fin = S;
    Y_fin[job_id][machine_id] = -1;
    S_fin[job_id][machine_id] = -1;
    out.push_back({Y_fin, S_fin, p * (1.0 - p_stay)});

    out.push_back({Y, S, p * p_stay});

    if (debug_){
        cout << indent(depth) << "=>Geometric task (" << job_id << "," << machine_id << ") finished - proba = " << p * (1.0 - p_stay) << endl;
        cout << indent(depth) << "=>Geometric task (" << job_id << "," << machine_id << ") not finished - proba = " << p * p_stay << endl;
    }
}



vector<NextState> expandOneMachine(const vector<NextState>& states,int machine_id,int job_id, int quantum,const vector<Job>& jobs, int allocation_mode,int depth)
{
    vector<NextState> out;
    out.reserve(states.size() * 2);

    for (const auto& st : states) {
        auto Y = st.Y;
        auto S = st.S;
        double p = st.p;

        // IDLE 
        if (job_id == -1) {
            if (debug_)
                cout << indent(depth)<< "- job id=-1: no job selected " << endl;
            
            out.push_back({Y, S, p}); // no changing
            continue;
        }

        if (debug_){
            cout << indent(depth)<< "- job id= "<<job_id << endl;
            // cout << indent(depth)<< "- job id= "<<job_id << "; quantum = " << quantum<< endl;
            cout << indent(depth)<< "State:" << endl;
            printState(Y, depth);
        }
        

        int phase_id = S[job_id][machine_id];

        
        // Finished task (phase marked finished)
        if (phase_id == -1){
            if (debug_)
                cout  << indent(depth) <<  "***job finished(" << job_id << "," << machine_id<<") " << phase_id << endl;
            
            out.push_back({Y, S, p}); // no changing
            continue;
            // break;
        }
        const Task& task = jobs[job_id].getTask(machine_id);
        const auto& phases = task.getPhases();
        
        const Phase& phase = phases[phase_id];

        int proc_time = Y[job_id][machine_id];

        // Already finished in Y
        if (proc_time == -1) {
            // Keep consistent: if you ever set Y=-1, S should also be -1
            S[job_id][machine_id] = -1;
            out.push_back({Y, S, p});
            continue;
        }

        // ************Geometric task: same transition for every allocation mode ************
        if (task.isGeometric()) {
            expandGeometricTask(Y, S, p, job_id, machine_id, quantum, task, out, depth);
            continue;
        }


        // **************************Handle allocation mode************************************
        // ALLOCATION CASE: Unit-phase
        if (allocation_mode == -1){ 
            if (proc_time + 1 < phase.duration) {
                // cout << indent(depth) << "" << endl;
                Y[job_id][machine_id] += 1;
                out.push_back({Y, S, p});
                
                if (debug_){
                    cout << indent(depth)<< "=>Next state within phase" << endl;
                    printState(Y, depth);
                    cout << indent(depth)<< " - proba = " << p << endl;
                }
                    
                continue;
            }

            // Phase completes now
            // cout << "current proc_time =  " << proc_time << ", quantum = " << 1  << "; phase duration= " << phase.duration<< endl;
            if (proc_time + 1 == phase.duration) {
                
                bool last = (phase_id == (int)phases.size() - 1);

                if (!last) {
                    // SUCCESS -> next phase
                    auto Y_succ = Y;
                    auto S_succ = S;
                    Y_succ[job_id][machine_id] += 1;
                    S_succ[job_id][machine_id] += 1;
                    out.push_back({Y_succ, S_succ, p * phase.beta_prob});
                    
                    if (debug_){
                        cout << indent(depth)<< "=>Next state after reashed current phase" << endl;
                        printState(Y_succ, depth);
                        cout << indent(depth)<< " - proba = " << p * phase.beta_prob << endl;
                    }
                    
                }

                // FINISH/FAIL -> task ends
                auto Y_fin = Y;
                auto S_fin = S;
                Y_fin[job_id][machine_id] = -1;
                S_fin[job_id][machine_id] = -1;
                out.push_back({Y_fin, S_fin, p * (1.0 - phase.beta_prob)}); // 1-phase.beta_prob not corret when reached final terminal 

                if (debug_){
                    cout << indent(depth) << "=>Final state " << endl;
                    printState(Y_fin, depth);
                    cout << indent(depth)<< " - proba = " << p * (1-phase.beta_prob) << endl;
                }
                
                continue;
            }

        
        }
       
        // ALLOCATION CASE: Semi-phase
        else if (allocation_mode == 0){ 
            // cout << "current proc_time =  " << proc_time << ", quantum = " << quantum  << "; phase duration= " << phase.duration<< endl;
            
            if (proc_time + quantum < phase.duration) {
                // cout << indent(depth) << "" << endl;
                Y[job_id][machine_id] += quantum;
                out.push_back({Y, S, p});
                
                if (debug_){
                    cout << indent(depth)<< "=>Next state within phase" << endl;
                    printState(Y, depth);
                    cout << indent(depth)<< " - proba = " << p << endl;
                }
            
                continue;
            }
            // Phase completes now
            if (proc_time + quantum == phase.duration) {
            
                bool last = (phase_id == (int)phases.size() - 1);

                if (!last) {
                    // SUCCESS -> next phase
                    auto Y_succ = Y;
                    auto S_succ = S;
                    Y_succ[job_id][machine_id] += quantum;
                    S_succ[job_id][machine_id] += 1;
                    out.push_back({Y_succ, S_succ, p * phase.beta_prob});
                    
                    if (debug_){
                        cout << indent(depth)<< "=>Next state after reashed current phase" << endl;
                        printState(Y_succ, depth);
                        cout << indent(depth)<< " - proba = " << p * phase.beta_prob << endl;
                    }
                    
                }

                // FINISH/FAIL -> task ends
                auto Y_fin = Y;
                auto S_fin = S;
                Y_fin[job_id][machine_id] = -1;
                S_fin[job_id][machine_id] = -1;
                out.push_back({Y_fin, S_fin, p * (1.0 - phase.beta_prob)}); // 1-phase.beta_prob not corret when reached final terminal 

                if (debug_){
                    cout << indent(depth) << "=>Final state " << endl;
                    printState(Y_fin, depth);
                    cout << indent(depth)<< " - proba = " << p * (1-phase.beta_prob) << endl;
                }
                
                continue;
            }            
        }

        // ALLOCATION CASE: Phase
        else { 
            // cout << "current proc_time =  " << proc_time << ", quantum = " << quantum  << "; phase duration= " << phase.duration<< endl;
            quantum = phase.duration - proc_time; 
            bool last = (phase_id == (int)phases.size() - 1);

            if (!last) {
                // SUCCESS -> next phase
                auto Y_succ = Y;
                auto S_succ = S;
                Y_succ[job_id][machine_id] += quantum; // Adding phase quantum q;
                S_succ[job_id][machine_id] += 1;
                out.push_back({Y_succ, S_succ, p * phase.beta_prob});
                
                    if (debug_){
                    cout << indent(depth)<< "=>Next state after reashed current phase" << endl;
                    printState(Y_succ, depth);
                    cout << indent(depth)<< " - proba = " << p * phase.beta_prob << endl;
                    }
                
            }

            // FINISH/FAIL -> task ends
            auto Y_fin = Y;
            auto S_fin = S;
            Y_fin[job_id][machine_id] = -1;
            S_fin[job_id][machine_id] = -1;
            out.push_back({Y_fin, S_fin, p * (1.0 - phase.beta_prob)}); // 1-phase.beta_prob not corret when reached final terminal 

            if (debug_){
                cout << indent(depth) << "=>Final state " << endl;
                printState(Y_fin, depth);
                cout << indent(depth)<< " - proba = " << p * (1.0 - phase.beta_prob) << endl;
            }

            }
    
    }

    return out;
}


//------------------------------------//
//  Self-loop (geometric tasks)       //
//------------------------------------//
bool isSameState(const NextState& ns, const Matrix& Y, const Matrix& S)
{
    return ns.Y == Y && ns.S == S;
}

// q_partial = cost + sum over next states different from the current one
double solveSelfLoop(double q_partial, double p_self)
{
    if (p_self == 0.0) return q_partial;                                 // discrete tasks: unchanged
    if (p_self >= 1.0) return numeric_limits<double>::infinity();      // action never makes progress
    return q_partial / (1.0 - p_self);
}



vector<NextState> generateNextStates(const Matrix& Y0,const Matrix& S0,const vector<Job>& jobs,const Action& A, int quantum, int allocation_mode,int depth)
{
    vector<NextState> states = { {Y0, S0, 1.0} };

    for (int m = 0; m < (int)A.size(); ++m) {
        states = expandOneMachine(states, m, A[m], quantum , jobs,allocation_mode, depth);
    }

    return states;
}


// Print Phases States
set<Matrix> GeneratePhaseStates(const vector<Job>& jobs) {
    set<Matrix> states;
    Matrix current = generateInitialState(jobs);
   
    int N = jobs.size();

    function<void(int, int)> dfs = [&](int j, int i) {
        if (j == N) {
            states.emplace(current);
            return;
        }

        int next_j = j;
        int next_i = i + 1;

        if (next_i == jobs[j].getNbTask()) {
            next_i = 0;
            next_j++;
        }

        const Task& task = jobs[j].getTask(i);
        const auto& phases = task.getPhases();

        // duration 0 is always allowed
        current[j][i] = 0;
        dfs(next_j, next_i);

        for (int ph=0; ph< phases.size() ; ph++ ) {
            if (ph +1 == phases.size())
                current[j][i] = -1;
            else current[j][i] = phases[ph].duration;
            dfs(next_j, next_i);
        }
    };

    dfs(0, 0);
    return states;
}




pair<double, int > getCostAndQuantum(const Matrix& Y,const Matrix& S, const vector<Job>& jobs, const Action& A,int allocation_mode){
    double cost;
    int quantum = 1;

    if (allocation_mode == -1)
        cost = getImmediatCost(S, jobs);

    else if (allocation_mode == 0){
        auto couple = getImmediatCostPerSemiPhase(Y,S, jobs, A);
        cost = couple.first;
        quantum = couple.second;
    }
           
    else
        cost = getImmediatCostPerPhase(Y,S, jobs, A);

    return make_pair(cost, quantum);
}

pair<double, int> QFunction(const Matrix& Y, const Matrix& S, const vector<Job>& jobs, const vector<Job>& jobs_determinist, const Action& A,int allocation_mode,
    map<string, double>& memories,map<Matrix, Action>& decisions,map<string, ActionsQuantum>& decisions_S,
    map<string, DecisionValue>& mdp_policy, int depth, set<Matrix> phasesStates, int generationType)
{
    
    auto couple = getCostAndQuantum(Y,S, jobs, A, allocation_mode);
    double cost = couple.first;
    int quantum = couple.second;
  
    auto next_states = generateNextStates(Y, S, jobs_determinist, A, quantum, allocation_mode, depth);

    // Self-loop (geometric tasks: nobody finished): Q = cost + p_self*Q + rest
    //  => Q = (cost + rest) / (1 - p_self). For discrete tasks p_self = 0.
    double p_self = 0.0;
    double q = cost;
    for (const auto& ns : next_states) {
        if (isSameState(ns, Y, S)) { p_self += ns.p; continue; }
        q += ns.p * ValueFunction(ns.Y, ns.S, jobs, jobs_determinist, (int)A.size(), allocation_mode, memories, decisions,decisions_S, mdp_policy, depth + 1, phasesStates, generationType); // true code
        // q += ns.p * ValueFunction(ns.Y, ns.S, jobs, (int)A.size(), allocation_mode, depth, phasesStates, generationType);
    }
    q = solveSelfLoop(q, p_self);
    return make_pair(q, quantum);
}




double ValueFunction(const Matrix& Y, const Matrix& S, const vector<Job>& jobs , const vector<Job>& jobs_determinist, int n_machines,int allocation_mode,
 map<string, double>& memories,map<Matrix, Action>& decisions,map<string, ActionsQuantum>& decisions_S,
 map<string, DecisionValue>& mdp_policy,int depth, set<Matrix> phasesStates, int generationType)
{
    int n_jobs = (int)jobs.size();

    // BASE CASE: all finished?
    bool all_finished = true;
    for (int j = 0; j < n_jobs && all_finished; ++j) {
        for (int m = 0; m < n_machines; ++m) {
            if (S[j][m] != -1) { all_finished = false; break; }
        }
    }
    if (all_finished) {
        if (debug_) cout << indent(depth) << "FINAL STATE V=0.0\n";
        return 0.0;
    }

    // MEMOIZATION
    string key = states_key(Y, S);
    // string key = state_key(Y);
    auto it = memories.find(key);
    if (it != memories.end()) return it->second;

    if (debug_) {
        cout << indent(depth) << "Evaluating V(Y) depth=" << depth << endl;
        printState(Y, depth);
    }

    // -----ACTIONS
  
    Matrix actions;
   
    if(generationType == 0){// Recursive Generation 
        actions = getAllActionsRecursive(n_jobs, n_machines); 
        if(debug_){
            cout << indent(depth) << "Recursive Generation  "<< endl;
            printActionMatrix(actions, depth);
        }
    }
    else if (generationType == 1){// Order Generation 
        // debug_ = false;
        actions = getAllActionsFromOrder(Y,depth);
        // debug_ = true;
        if(debug_){
            cout <<"\n"<< indent(depth) << "Order Generation "<< endl;
            printActionMatrix(actions, depth);
        }
    }
    else if (generationType == 2){ // Successive generation
        // debug_ = false;
        set<Action> actions_set = generateSuccessiveOrderActions(Y,depth);
        // debug_ = true;
        actions.assign(actions_set.begin(), actions_set.end());
        if(debug_){
            cout << indent(depth) << "Successive generation "<< endl;
            printActionMatrix(actions, depth);
        }
    }
    else cout << "error" << endl;

    double best_val = numeric_limits<double>::infinity();
    Action best_action;
    int quantum = 0;
    int k = 0;

    for (const auto& act : actions) {
        ++k;
        if (debug_){
            cout <<"\n"<< indent(depth) << "Action " << k << ": ";
            printAction(act, 0);
         }
        
        
        if (invalidAction(act)) {
            if (debug_) cout  << " => Action invalid\n";
            continue;
        }
        
        if (violatesNoIdleConstraint(Y,S, act)) {
            if (debug_) cout << " => Violation:  No-idle if job still unfinished on machine \n";
            continue;
        }
        // if (isUselessAction(Y,S, act)) {
        //     cout << indent(depth) << "Y:" ; printState(Y, 0);cout << endl;
        //     cout << indent(depth) << "S:" ; printState(S, 0); cout << endl;
        //     cout << indent(depth) << "A:" ; printAction(act, 0);cout << "\n -------------"<<endl;
        //     if (debug_) cout << " => Useless Action:  job choosed when finished \n";
        //     continue;
        // }

        if (!actionMakesProgressOnAnyMachine(Y, S, jobs, act)) {
             if (debug_) cout << "- action makes no changing" << endl; ;
            // This action produces next state identical to current -> skip it
            continue;
        }

        if (debug_) cout << endl;
        auto couple = QFunction(Y, S, jobs, jobs_determinist, act, allocation_mode,memories,decisions, decisions_S,mdp_policy,depth, phasesStates,generationType); 
        double val = couple.first;
        
        if (val < best_val) {
            best_val = val;
            best_action = act;
            quantum = couple.second;
        }
    }

    memories[key] = best_val;

    decisions[Y] = best_action; // store the state

    decisions_S[key] = {best_action, quantum}; // store the state
    mdp_policy[key] = {best_action, quantum, best_val};

    // Printing Reashed State
    // debug_= true;
    if(debug_){
        cout << indent(depth) << "********************* \n";
        cout <<indent(depth) << "Action : "; printAction(best_action,0); cout<< endl;
        cout << indent(depth)<< "State: " << endl;
        printState(Y, depth);
        cout << indent(depth)<<" -> STATE VALUE V= "<< best_val<< "\n"<<endl;
    }
    // debug_= false;

   

    
    if (debug_) {
        cout << indent(depth) << "→ Best V=" << best_val << " | Best action=[";
        for (int a : best_action) cout << a << " ";
        cout << "] | quantum= "<< quantum <<"\n";
    }

    return best_val;
}


double ValueFunction(const Matrix& Y, const Matrix& S, const vector<Job>& jobs ,  int n_machines,int allocation_mode,
 map<string, double>& memories,map<Matrix, Action>& decisions,map<string, ActionsQuantum>& decisions_S,
 map<string, DecisionValue>& mdp_policy,int depth, set<Matrix> phasesStates, int generationType){
    return ValueFunction(Y, S,jobs , jobs,  n_machines, allocation_mode,memories, decisions, decisions_S,
                            mdp_policy, depth, phasesStates,  generationType);
 }




void printDecisionsTrajectoryPerPhase_rec(const Matrix& Y,const Matrix& S,const vector<Job>& jobs,
                                            const map<string, DecisionValue>& mdp_policy,const set<string>& visited,int depth,double pathProb,map<string, DecisionValue>& mdp_policy_final)
{
    string key = states_key(Y, S);
    // string key = state_key(Y);

    cout << indent(depth) << "State (probability = " << pathProb << ")\n";
    cout << indent(depth) << "Y:\n";
    printState(Y, depth);

   
    bool terminal = true;
    for (const auto& row : S) {
        for (int x : row) {
            if (x != -1) {
                terminal = false;
                break;
            }
        }
        if (!terminal) break;
    }

    if (terminal) {
        cout << indent(depth) << "=> Terminal state\n";
        cout << indent(depth) << "-----------------\n";
        return;
    }

    auto it = mdp_policy.find(key);
    if (it == mdp_policy.end()) {
        cout << indent(depth) << "=> No decision found. Stopping.\n";
        cout << indent(depth) << "-----------------\n";
        return;
    }

    const Action& action = it->second.A;
    int quantum = it->second.q;
    float val = it->second.V;

    mdp_policy_final[key] = {action, quantum, val};

    cout << indent(depth) << "Action: ";
    printAction(action, 0);
    cout << " - quantum: " << quantum << "  ---- V(Y) = "<< val<<"\n";

    auto nextStates = getNewStatePerPhase(Y, S, action, quantum, jobs, depth);

    if (nextStates.empty()) {
        cout << indent(depth) << "=> No successor states generated.\n";
        cout << indent(depth) << "-----------------\n";
        return;
    }
    cout << indent(depth) << "Next States: " << endl;
    for (int b = 0; b < (int)nextStates.size(); ++b) {
        const auto& ns = nextStates[b];        
        cout << indent(depth) << "=> Y':\n";
        printState(ns.Y, depth);

    }

    for (int b = 0; b < (int)nextStates.size(); ++b) {
        const auto& ns = nextStates[b];
        double nextProb = pathProb * ns.p;

        // cout << indent(depth) << "Branch " << b
        //      << " | transition probability = " << ns.p
        //      << " | cumulative path probability = " << nextProb << "\n";
        cout << indent(depth) << "Niveau " << depth+1<< "- State "<<b
             << " | transition probability = " << ns.p
             << " | cumulative path probability = " << nextProb << "\n";

        string next_key = states_key(ns.Y, ns.S);

        if (visited.count(next_key)) {
            cout << indent(depth + 1) << "[Loop detected: state already visited]\n";
            continue;
        }

        set<string> visited_copy = visited;
        visited_copy.insert(next_key);

        printDecisionsTrajectoryPerPhase_rec(
            ns.Y,
            ns.S,
            jobs,
            mdp_policy,
            visited_copy,
            depth + 1,
            nextProb,mdp_policy_final
        );
    }

    cout << indent(depth) << "-----------------\n";
}

void printDecisionsTrajectoryPerPhase(const vector<Job>& jobs,const map<string, DecisionValue>& mdp_policy,map<string, DecisionValue>& mdp_policy_final){
    cout << "\n===== Decisions (full trajectory tree from initial state) =====\n";

    Matrix Y0 = generateInitialState(jobs);
    Matrix S0 = generateInitialState(jobs);

    set<string> visited;
    visited.insert(states_key(Y0, S0));

    

    printDecisionsTrajectoryPerPhase_rec(
        Y0,
        S0,
        jobs,
        mdp_policy,
        visited,
        0,
        1.0, mdp_policy_final
    );
}


