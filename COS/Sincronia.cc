
#include "Sincronia.hxx"

#define INFTY  1.0e20



void printSigma(list<int> sigma, int depth){
    cout << indent(depth) << "Sincronia: sigma = [";
        bool first = true;
        for (const auto& s : sigma) {
            if (!first) cout << ", ";
            cout << s;
            first = false;
        }
        cout << "]\n";

}

vector<Job> UnfinishedJobs(const vector<Job>& jobs, const Matrix& Y){
    int N = jobs.size();
    if(N == 0) return {}; 

    vector<Job> jobs_Y ;
    for(int j=0;j<N;j++){
        if(!isJobComplete(j,Y))         
            jobs_Y.push_back(jobs[j]);
    }
    return jobs_Y;
}


//------------------------------------------------------------------------------//
// Public Method:      getTaskExpectedProcTime replace total_transmission_time  //
//------------------------------------------------------------------------------//

double getTaskExpectedProcTime(const vector<Job>& jobs,const Matrix& Y, int j, int i, int depth){
    
    const Task& task = jobs[j].getTask(i);

    // Geometric: E[P - y | P > y] = 1/alpha (memoryless), 0 if finished
    if (task.isGeometric()) return (Y[j][i] == -1) ? 0.0 : task.expectedRemaining(Y[j][i]);
    
    double expected = 0.0;
    
    for (auto &r : task.getRemaining(Y[j][i])) {
        int processing_time = r.first;
        double prob = r.second; 
        // cout << "proc = " << processing_time <<", proba = " << prob << endl;
          
        if(Y[j][i] != -1 && processing_time > 0){
            expected += processing_time * prob; 
        }
    }

    return expected;
}

//----------------------------------------------------------------//
// Public Method:      largest_wpt -largest_pt                    //
//----------------------------------------------------------------//

int largest_wpt(int b,const vector<Job>& jobs, const Matrix& Y, set<int> &S_jobs, set<int> & S_b, map<int, double> & weights, int depth ) {

    int                 arg_min;
    double              min_wpt = INFTY;
    double              weighted_proc_time, w , p;
    
    // initialisation                                                                                                                                                                                        
    S_b.clear();
    arg_min = -1;

    // Parcourt des coflows                                                                                                                                                                                  
    for (int j:S_jobs) {     

        // If task finished on bottleneck machine b                                                                                                                                                
        if (Y[j][b] == -1 ) continue;
                                                                                                                                                                 
        S_b.insert(j);

        
        w = weights[j];
        p = getTaskExpectedProcTime(jobs,Y, j, b, depth);
        weighted_proc_time= w/p;
        
        if (debug_ ){
            cout << "\tJob " << j << " : w=" << w << ", p=" << p
            << " =>  Weighted proc. time on machine " << b
            << " is " << weighted_proc_time << endl;        
        }
        
        // Job with largest weighted proc. time                                                                                                                          
        if ( weighted_proc_time < min_wpt ) {
            min_wpt = weighted_proc_time;
            arg_min =j ;
        }                                                                                                                                                                                             
    }                                                                                                                                                                                              

    return arg_min;
}


int largest_pt(int b,const vector<Job>& jobs, const Matrix& Y, set<int> &S_jobs, set<int> & S_b, int depth ) {

    int                 arg_min;
    double              min_pt = INFTY;
    double              proc_time, w , p;
    
    // initialisation                                                                                                                                                                                        
    S_b.clear();
    arg_min = -1;

    // Parcourt des coflows                                                                                                                                                                                  
    for (int j:S_jobs) {     

        // If task finished on bottleneck machine b                                                                                                                                                
        if (Y[j][b] == -1 ) continue;
                                                                                                                                                                 
        S_b.insert(j);

        
       
        p = getTaskExpectedProcTime(jobs,Y, j, b, depth);
        proc_time= 1/p;
        
        if (debug_ ){
            cout << "\tJob " << j << " : w=" << 1 << ", p=" << p
            << " =>  Weighted proc. time on machine " << b
            << " is " << proc_time << endl;        
        }
        
        // Job with largest weighted proc. time                                                                                                                          
        if ( proc_time < min_pt ) {
            min_pt = proc_time;
            arg_min =j ;
        }                                                                                                                                                                                             
    }                                                                                                                                                                                              

    return arg_min;
}

//----------------------------------------------------------------//
// Public Method: next_release_time                               //
// Method calculating the total workload (termination time) of    //
// machine i given the set S_jobs of jobs to be executed          //
//----------------------------------------------------------------//

double next_release_time(const vector<Job>& jobs,const Matrix& Y, int i, set<int> & S_jobs, int depth) {
    double              x = 0.0;
    double              p;
    

    for (int j: S_jobs) {
        p = getTaskExpectedProcTime(jobs, Y,j,i, depth) ;
        if (debug_) 
            cerr << "\tMachine " << i << ": job " << j << " adds " << p << " to workload\n" << endl;
        
        if ( p > 0.0 )            
            x += p;        
    }

    if ( debug_ )
        cerr << "\tMachine " << i  << ": Total workload is " << x  << endl;
    
    return x;
}

//----------------------------------------------------------------//
//Public Method: bottleneck_link                                  //
// Given the set S of jobs, this method calculates             //
// the termination time of all machines and returns the ID        //
// external of the one that terminates last.                      //
//----------------------------------------------------------------//

int bottleneck_link(const vector<Job>& jobs,const Matrix& Y, set<int> & S_jobs, double & max_end_time, int depth) {
    double              x;
    int                 M,i;
    int                 arg_max = 0;


    //initialisation
    max_end_time = 0.0;

    M = Y[0].size();

    for (i=0; i<M; i++) {
        // Calculating the termination time for this machine
        x = next_release_time(jobs,Y,i,S_jobs, depth);
        
        
        // link_[i].setCompletionTime(x);
        // if ( debug_ )
        // cerr << "Completion time is " << x << " on link " << link_[i].getId() << endl;

        
        if ( x >= max_end_time ) {
            max_end_time = x;
            arg_max = i;
        }
    }

    max_end_time=next_release_time(jobs,Y,arg_max, S_jobs, depth) ;

    return arg_max;
}


//----------------------------------------------------------------//
// Public Method:     update_sincroniaCoflows_weights             //
//----------------------------------------------------------------//

void update_sincroniaCoflows_weights(const vector<Job>& jobs,const Matrix& Y,set<int> S_jobs, map<int, double>  &weights , int b, int IK, int depth){
    
    double              p_star = getTaskExpectedProcTime(jobs,Y, IK,b,depth);
    double              p;

    // Parcourt des coflows 
    if ( debug_ ) cerr << "Sincronia: update weights - ";
    
    for (int j : S_jobs) {
        if (Y[j][b] == -1 ) continue;
        if (j!= IK) {
            p = getTaskExpectedProcTime(jobs,Y,j,b , depth);
            weights[j]= weights[j]- weights[IK]*p/p_star;
            if ( debug_ )
                cerr << "w[" << j << "]=" << weights[j] << "  ";
        }
    }
    weights[IK]=0.0;
    if ( debug_ )
      cerr << endl;
}      
       
bool isJobComplete(int j, const Matrix& Y){
    bool finished = true;
    for(int i= 0; i<Y[j].size(); i++){
        if(Y[j][i] != -1) finished = false;
    }
    return finished;
}

list<int> Sincronia(const vector<Job>& jobs, const Matrix& Y , int depth){

    int N = Y.size();
    if (N == 0) return {};

    int M = Y[N-1].size();

    // S = IDs
    set<int> S_jobs;    
    list<int> sigma;     
    map<int, double> weights;

    int NumIncomplete = 0;
    for (int j = 0; j < N; j++) {

        if(isJobComplete(j,Y)) continue;
        // if(debug_) cout << "job " << j << " not complete"<< endl;
        NumIncomplete +=1;
        S_jobs.insert(j);
        weights[j] = jobs[j].getWeight(); 
    }


    double max_end_time = 0.0;
    set<int> S_b;

    for (int t = NumIncomplete; t >= 1; --t) {
        if (debug_) cout <<indent(depth) << "Sincronia: iteration " << t << "\n";

        // b = machine index 
        int b = bottleneck_link(jobs,Y, S_jobs,max_end_time , depth);
        if (debug_) cout <<indent(depth) << "   -bottleneck machine is " << b << "\n";

        // // 
        // int Kid = largest_wpt(b,jobs,Y, S_jobs, S_b, weights, depth);

        
        int Kid = largest_pt(b,jobs,Y, S_jobs, S_b, depth); // largest processint time instead of weighted proc.time

        if (Kid < 0 || Kid >= N) {
            cerr << "[BUG] largest_wpt returned invalid Kid=" << Kid << "\n";
            abort();
        }
        

        // sécurité : Kid doit être dans S
        if (S_jobs.find(Kid) == S_jobs.end()) {
            cerr << "[BUG] selected job id=" << Kid << " not in S_jobs\n";
            abort();
        }

        if (debug_) cout <<indent(depth) << "  -> Sincronia: scheduled job is id=" << Kid << "\n";

        // // Primal + cost (accès par POSITION)
        // coflow_[Kpos].setCT(max_end_time);
        // cost += coflow_[Kpos].getWeight() * max_end_time;

        // Update weights 
        update_sincroniaCoflows_weights(jobs,Y,S_jobs,weights,b, Kid, depth);

        sigma.push_front(Kid);
        S_jobs.erase(Kid);
    }

    if (debug_) {
        printSigma(sigma, depth);
        
    }

    return sigma;
}


int getMinQuantum(Task T, int y){
    if (T.isGeometric()) return T.minQuantum(y); // 1
    return T.getRemaining(y).begin()->first; //smallest remaining processing time
}



ActionsQuantum policySincronia(const vector<Job>& jobs,const Matrix& Y,bool isDynamic, list<int> sigma, int depth){
    // list<int> sigma = Sincronia(jobs, Y,depth);
    if ( isDynamic){// if dynamic, extract sigma from current state
        // cout << indent(depth) << "Dynamic sigma computing..." << endl;
        sigma = Sincronia(jobs, Y,depth);
    } 

    if (debug_){
        cout << indent(depth) << "Sincronia: "<< endl;
        printSigma(sigma,depth);
    }
    // printSigma(sigma,depth);

    if (sigma.size() == 0) {
        cerr << "Error: no selection";
        return {};
    }

    int N = jobs.size();
    if (N == 0) return {};
    int M = jobs[N-1].getNbTask();

    Action A(M, -1);
    int min_q = int(INFTY);

    auto current_j = sigma.begin();
    for(int i=0; i<M ; i++){
        bool found = false;

        for (auto it = current_j; it != sigma.end(); ++it) {
            int j = *it;
            
            if(Y[j][i] != -1){
                A[i] = j;
                found = true;

                Task t = jobs[j].getTask(i);
                int y = Y[j][i];
                int q = getMinQuantum(t,y);
                if (q < min_q) min_q = q;

                if(debug_)
                    cout << indent(depth)<<  "- Machine " << i
                        << " assigned to Job " << jobs[j].getId()
                        << " | quantum = " << min_q
                        << endl;
                break;
            }

        }
        if (!found) {
            if(debug_)
                cout << indent(depth)<<  "- Machine " << i
                    << " assigned to no job (all candidate tasks completed)" << endl;
        }
    }

    return {A, min_q};
}

double PolicyQFunction_Sincronia(const Matrix& Y,const Matrix& S,const vector<Job>& jobs,const vector<Job>& jobs_determinist,const ActionsQuantum& decision,
    map<string, double>& sin_memories,map<string, ActionsQuantum>& sin_decisions,
    map<string, DecisionValue>& sin_policy,bool isDynamic, list<int> sigma, int depth)
{
    double cost = getImmediateCostWithQuantum(S, jobs, decision.q);

    // Transition follows the sampled realization jobs_determinist, not the true distribution.
    auto next_states = generateNextStates(Y, S, jobs_determinist, decision.A, decision.q, depth);

    // Self-loop (geometric tasks): see solveSelfLoop in ValueIteration.cc
    double p_self = 0.0;
    double qval = cost;
    for (const auto& ns : next_states) {
        if (isSameState(ns, Y, S)) { p_self += ns.p; continue; }
        qval += ns.p * PolicyValueFunction_Sincronia(ns.Y, ns.S, jobs, jobs_determinist, (int)decision.A.size(), sin_memories, sin_decisions, sin_policy,isDynamic,sigma, depth + 1);
    }

    return solveSelfLoop(qval, p_self);
}


//------------------------------------//
//            Memoization             //
// Cache for already-computed states  //
//------------------------------------//

// map<string, double> sin_memories;
// map<string, ActionsQuantum> sin_decisions;
// map<string, DecisionValue> sin_policy;



double PolicyValueFunction_Sincronia(const Matrix& Y,const Matrix& S, const vector<Job>& jobs,const vector<Job>& jobs_determinist,int n_machines,
    map<string, double>& sin_memories,map<string, ActionsQuantum>& sin_decisions,
    map<string, DecisionValue>& sin_policy, bool isDynamic, list<int> sigma ,int depth){
    // if(depth == 8){
    //     return 0.0;
    // }

    bool all_finished = true;
    for (int j = 0; j < (int)jobs.size() && all_finished; ++j) {
        for (int m = 0; m < n_machines; ++m) {
            if (S[j][m] != -1) {
                all_finished = false;
                break;
            }
        }
    }

    if (all_finished) {
        if (debug_) cout << indent(depth) << "FINAL STATE V=0.0\n";
        return 0.0;
    }

    // MEMOIZATION
    // string key = states_key(Y, S);
    string key = state_key(Y);
    auto it = sin_memories.find(key);
    if (it != sin_memories.end()) return it->second;

    if (debug_) {
        cout <<"\n"<< indent(depth) << "Evaluating V(Y) depth=" << depth << endl;
        printState(Y, depth);
    }

    ActionsQuantum decision = policySincronia(jobs,Y, isDynamic, sigma, depth);
    // debug_ = true;
    if (debug_){
        cout << indent(depth) << "Action : ";
        printAction(decision.A, 0);
        cout  << " - quantum : " << decision.q << endl;;

    }
    // debug_ = false;
    double val = PolicyQFunction_Sincronia(Y, S, jobs, jobs_determinist, decision,sin_memories, sin_decisions, sin_policy, isDynamic, sigma, depth);

    sin_memories[key] = val;
    sin_decisions[key] = decision;
    sin_policy[key] = {decision.A, decision.q, val};

    // debug_= true;
    if(debug_){
        cout << indent(depth) << "********************* \n";
        cout << indent(depth)<< "State: " << endl;
        printState(Y, depth);
        cout <<indent(depth) << "Action : "; printAction(decision.A,0); cout<<"- quantum= " << decision.q << endl;
        cout << indent(depth)<<" -> STATE VALUE V= "<< val << "\n"<<endl;
    }


    // debug_= false;

    return val;
}

double PolicyValueFunction_Sincronia(const Matrix& Y,const Matrix& S, const vector<Job>& jobs,int n_machines,
    map<string, double>& sin_memories,map<string, ActionsQuantum>& sin_decisions,
    map<string, DecisionValue>& sin_policy, bool isDynamic, list<int> sigma ,int depth){
    return PolicyValueFunction_Sincronia(Y, S, jobs, jobs, n_machines, sin_memories, sin_decisions, sin_policy, isDynamic, sigma, depth);
}



//------------------------------------//
//      Policies comparison           //
//----------------------------------- //

void verifyVIvsPE(int n, int m,const vector<Job>& jobs, map<string, DecisionValue>& policy, string filename, string mode){
    ofstream out(filename);

    streambuf* old_buf = cout.rdbuf(out.rdbuf()); // redirect cout to file
    
    cout << "\n******** Jobs Description *******\n" ;
    JobsDescription(jobs);

    cout << "\n******** Policies comparison: VI("+mode+") VS Sincronia ********" << endl;
    map<string, DecisionValue> differing_states;
    
    bool isDynamic = true;
    list<int> sigma ;

    cout << "\n\t NUMBER OF STATES : " << policy.size()<< " \n";
    // Step 1: Silent computation
    debug_ = false;
    for (const auto& s : policy) {
        Matrix Y, S;
        decode_states_key(s.first, n, m, Y, S);
        cout << endl;printState(Y);
        
        ActionsQuantum decision = policySincronia( jobs,Y, isDynamic,sigma, 0);
        // int q_vi = s.second.q;
        cout << "VI- A: " ;
        printAction(s.second.A);
        cout << " - q: " << s.second.q<< endl;

        cout << "PE(Sincronia)- A: " ;
        printAction(decision.A);
        cout << " - q: " << decision.q<< endl;
        
        if (s.second.q !=decision.q  || s.second.A != decision.A) {
            differing_states[s.first] = {s.second.A, s.second.q, s.second.V};
            
        }
    }
    
    // Step 2: Debug ONLY differing states
    if (!differing_states.empty()) {
        cout << "\n=== DEBUGGING " << differing_states.size() << " DIFFERING STATES ===\n";
        // debug_ = true;
        for (auto s : differing_states) {
            
            Matrix Y, S;
            decode_states_key(s.first, n, m, Y, S);
            ActionsQuantum decision = policySincronia(jobs,Y, isDynamic , sigma , 0);  

            // debug_ = true;
            cout << "\nY: " << endl;
            printState(Y);

            // cout << "S: " << endl;
            // printState(S);
            
            // VI (from policy map)
            Action& act_vi = s.second.A;
            double vi = s.second.V;
            int q_vi = s.second.q;

            cout << "VI- A: " ;
            printAction(act_vi);
            cout << " - q: " << q_vi<< endl;

            cout << "PE(Sincronia)- A: " ;
            printAction(decision.A);
            cout << " - q: " << decision.q<< endl;


        }
        debug_ = false;
    } else {
        cout << "All states match! No debug needed.\n";
    }
    cout.rdbuf(old_buf); // restore cout
}




//------------------------------------------------------------------//
//                  Sincronia Static                                //
//                                                                  //
//                                                                  //
//------------------------------------------------------------------//




list<int> SincroniaStatic(const vector<Job>& jobs , int depth){

    int N = jobs.size();
    if (N == 0) return {};

    int M = jobs[N-1].getNbTask();

    Matrix Y = generateInitialState(jobs); // In static y=0

    // S = IDs
    set<int> S_jobs;    
    list<int> sigma;     
    map<int, double> weights;

   
    for (int j = 0; j < N; j++) {
        S_jobs.insert(j);
        weights[j] = jobs[j].getWeight(); 
    }


    double max_end_time = 0.0;
    set<int> S_b;

    for (int t = N; t >= 1; --t) {
        if (debug_) cout <<indent(depth) << "Sincronia: iteration " << t << "\n";

        // b = machine index 
        int b = bottleneck_link(jobs,Y, S_jobs,max_end_time , depth);
        if (debug_) cout <<indent(depth) << "   -bottleneck machine is " << b << "\n";

        // // 
        // int Kid = largest_wpt(b,jobs,Y, S_jobs, S_b, weights, depth);

        
        int Kid = largest_pt(b,jobs,Y, S_jobs, S_b, depth); // largest processint time instead of weighted proc.time

        if (Kid < 0 || Kid >= N) {
            cerr << "[BUG] largest_wpt returned invalid Kid=" << Kid << "\n";
            abort();
        }
        

        // sécurité : Kid doit être dans S
        if (S_jobs.find(Kid) == S_jobs.end()) {
            cerr << "[BUG] selected job id=" << Kid << " not in S_jobs\n";
            abort();
        }

        if (debug_) cout <<indent(depth) << "  -> Sincronia: scheduled job is id=" << Kid << "\n";

        // // Primal + cost (accès par POSITION)
        // coflow_[Kpos].setCT(max_end_time);
        // cost += coflow_[Kpos].getWeight() * max_end_time;

        // Update weights 
        update_sincroniaCoflows_weights(jobs,Y,S_jobs,weights,b, Kid, depth);

        sigma.push_front(Kid);
        S_jobs.erase(Kid);
    }

    if (debug_) {
        printSigma(sigma, depth);
        
    }

    return sigma;
}


void Schedule(vector<Job>& jobs , int depth){
    int m,j,i;

    list<int> sigma = SincroniaStatic(jobs, depth);

    list<int>::iterator os = sigma.begin();

    m = 0;
    for ( ; os != sigma.end(); ) {
        j = *os;  
        for (i=0; i<jobs[j].getNbTask(); i++) 
            jobs[j].setTaskPriority(i,m);

        m++;   
        
        os++;
    }     
    
}

//----------------------------------------------------------------//
// Public Method:  greedy_allocation                                  

void greedy_allocation(vector<Job>& jobs,list<int> & runningTasks, map<int,double> & rates, bool *busy,  set<int> & priorities, map<int, list<int> > &priority_map) {
    map<int, list<int> >::iterator map_it;
    set<int>::iterator             prio_it;
    list<int>::iterator            it;
    int                            i, j, t, p, m;
    int                            nb_busy_link = 0;
    double                         r, r_min;

    //initialization
    for (i=0; i<jobs[0].getNbTask(); i++)
        busy[i] = false;

    // We determine the priorities that are still active.
    for (map_it = priority_map.begin(); map_it != priority_map.end(); map_it++) {
        if ( map_it->second.empty() ) // when no task with same priority
            priorities.erase(map_it->first);
    }
    
    //Greedy bandwidth allocation to tasks in ascending order of priority
    for (prio_it = priorities.begin(); prio_it != priorities.end(); prio_it++) {

        // It scans tasks, skipping those that don't have the correct priority.
        for (it=priority_map[*prio_it].begin(); it != priority_map[*prio_it].end(); it++) {
            t = *it;
            j = t/ 1000;
            i = t % 1000; 
         
            if ( jobs[j].getTask(i).getPriority() != *prio_it ) continue;

            // machine used by current task
            if ( busy[i] )
                rates[t] = 0.0;
            else {
                rates[t] = 1.0;
                busy[i] = true;
            }
                

        } // End of loop on the tasks
    } // End of the loop on priorities

}



double Static_Scheduling(vector<Job>& jobs , int depth){
    int N = jobs.size();
    if (N==0) return {};
    int M = jobs[N-1].getNbTask();
    
    int                     j,i,t,p;
    double                  times = 0.0;       
    double                  global_time = 0.0;

    list<int>               runningTasks ;
    list<int>               finishedTasks ;

    map<int,double>         residual_size ;  
    map<int, double>        greedy_rate;  
    map<int, double>        rate;
    map<int, double>        cct;

    set<int>                priorities;
    set<int>::iterator      prio_it;
    map<int, list<int> >    priority_map;


    bool                    * busy = new bool[M]();


    Schedule(jobs, depth); // schedule jobs and add their priorities

    const Matrix Y = generateInitialState(jobs); // In static y=0

    for (j = 0; j<N ;j++){        
        for (i=0; i<M; i++){
            t = 1000 * j + jobs[j].getTaskId(i);
            p = jobs[j].getTask(i).getPriority();

            runningTasks.push_back(t);
            residual_size[t] = getTaskExpectedProcTime(jobs,Y,j,i,0);            
            priorities.insert(p);
        }
    }

    // Build priority map
    for (prio_it = priorities.begin(); prio_it != priorities.end(); prio_it++)
        priority_map[*prio_it] = list<int>();

    for (int t : runningTasks) {
        j = t / 1000;
        i = t % 1000;
        p = jobs[j].getTask(i).getPriority();
        priority_map[p].push_back(t);
    }

    while ( !runningTasks.empty()) {
        greedy_allocation(jobs, runningTasks, greedy_rate, busy, priorities, priority_map);

        times = INFTY;

        for (int t: runningTasks) {
            rate[t] = greedy_rate[t];
            
            if ( rate[t] > 0.0 ) {
                j = t / 1000;
                i = t % 1000; 
                p = jobs[j].getTask(i).getPriority();
                	
                cerr << "Rate for task (" << j << " ," << i << ") (id=" << t << ", expected size=" << getTaskExpectedProcTime(jobs,Y,j,i,0) << ")" << " = " << rate[t] << endl;
                cct[t] = residual_size[t]/rate[t];
                if ( cct[t] < times ) 
                    times = cct[t];
            }
        }
        if (times == INFTY) {
            cerr << "Warning: no task has a positive rate, breaking." << endl;
            break;
        }

        global_time += times;

        // determine the next tasks to depart and update residual sizes for the others
        for (int t : runningTasks) {
            j = t /1000;
            i = t % 1000; 
            p = jobs[j].getTask(i).getPriority();
            if ( fabs(cct[t]-times) < 1e-8 ) {
                finishedTasks.push_back(t);
            }
            else {
                residual_size[t] -= rate[t]*times;
                // if ( rate[f] > 0.0 )
                //   cerr << "Flow " << i << " of coflow " << k << " : residual size=" << residual_size[f] << endl;
            }
        }


        //remove finished flows
        for (int t : finishedTasks) {
            j = t /1000;
            i = t % 1000; 
            p = jobs[j].getTask(i).getPriority();

            cerr << "Task( " << j << "," << i << ") leaves the system at time " << global_time << endl;
            
            //      cerr << "Flow " << i << " of coflow " << k << " (id=" << f << ", size=" << coflow_[k].getFlowSize(i) << ")" << " leaves the system at time " << global_time << endl << endl;
            
            runningTasks.remove(t);
            priority_map[p].remove(t);
            residual_size.erase(t);
            greedy_rate.erase(t);
            rate.erase(t);
            cct.erase(t);
            jobs[j].updateCCT(global_time);

        }

        finishedTasks.clear();

    }

    delete [] busy;

    double avg = 0.0;
    for (j = 0; j< N; j++){
        avg += float(jobs[j].getCCT());
    }
    return avg/N;
}


