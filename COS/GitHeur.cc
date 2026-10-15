
#include <math.h>
#include <iomanip>
#include "GitHeur.hxx"


//------------------------------------------------------------------//
//  Method 1: GIPP per machine (independent of required tasks)      //
//                                                                  //
//  Description:                                                     //
//    Compute the Gittins Index Priority Policy (GIPP) separately    //
//    on each machine, ignoring cross-machine task coupling.         //
//    Each machine selects the job with the highest local index.     //
//                                                                  //
//  Notes:                                                           //
//    - No coordination across machines (Concurrent Open Shop).      //
//    - Preempt only at quantum boundaries (q* per job,machine).     //
//    - Good as a baseline; may be suboptimal for job completion.    //
//------------------------------------------------------------------//

double getProba(const Task& T, int y){
    if (T.isGeometric()) return T.getTotalProba(y); // Pr(P > y) = (1-alpha)^y

    double result = 0.0;
    for (auto &p : T.getDistribution()) {
        if (p.first > y) result +=p.second;
    }
    return result;
}

double Investment(Task T, int q, int y)
{
    // Geometric: E[min(P-y, q) | P > y] = sum_{k=0}^{q-1} (1-alpha)^k  (memoryless)
    if (T.isGeometric()) return (1.0 - pow(1.0 - T.getAlpha(), q)) / T.getAlpha();

    double totalProba = T.getTotalProba(y);
    if (totalProba == 0.0) return 0.0;


    double somme = 0.0;

    for (auto &r : T.getRemaining(y)) {
        int remaining_processing_time = r.first;   //Pj-y
        double remaining_prob = r.second;

        if (remaining_processing_time > 0) {
            somme += min(remaining_processing_time, q) * remaining_prob;  //Sum(min(Pj-y, q)* Pr(min(Pj-y,q)|Pj>y))
            // cout<<"\t min: " << minn <<", proba: "<< remaining_prob<<"  ,somme: " << somme<< endl;
        }
    }

    return somme;
}



double Reward(Task T, int q, int y)
{
    // Geometric: Pr(P-y <= q | P > y) = 1-(1-alpha)^q  (memoryless)
    if (T.isGeometric()) return 1.0 - pow(1.0 - T.getAlpha(), q);
    
    double totalProba = T.getTotalProba(y);
    if (totalProba == 0.0) return 0.0;

    double probs = 0.0;

    for (auto &r : T.getRemaining(y)) {
        int remaining_processing_time = r.first;   //Pj-y
        double remaining_prob = r.second;

        if (remaining_processing_time <= q) {
            probs += remaining_prob ; //Sum(Pr[Pj-y<= q])
        }
    }

    return probs  ; // Weight = 1, in other case check
}

pair<int,double> GittinsIndex(Task T, int y)
{
    // Geometric: Reward/Investment = alpha for every q -> quantum 1 (exact cost, see expandGeometricTask)
    if (T.isGeometric()) return make_pair(1, T.getAlpha());

    double val_rank, max_rank=0.0 ;
    int ind_max_rank = 0;

    for (auto &r : T.getRemaining(y)) {
        int remaining_processing_time = r.first;   //Pj-y  ~ q
        double remaining_prob = r.second;

        val_rank = Reward(T,remaining_processing_time,y) / Investment(T, remaining_processing_time,y);
        // cout<<"job: "<< T.getId() <<" , reward: "<< Reward(T,remaining_processing_time,y)<<", I="<< Investment(T, remaining_processing_time,y)<<" , rank= " << val_rank<< endl;
        // cout<<"job: "<< T.getId() <<", I=["<< Investment(T, remaining_processing_time,y)<<"] " <<endl;
        if (val_rank > max_rank)
        {
            max_rank = val_rank;
            ind_max_rank = remaining_processing_time;
        }
    }
    return make_pair(ind_max_rank,max_rank);
}

pair<int,double> GittinsIndex_MinQ(Task T, int y)
{
    // Geometric: same index for every y (memoryless)
    if (T.isGeometric()) return make_pair(T.minQuantum(y), T.getAlpha());

    double val_rank, max_rank=0.0 ;
    int min_q = 0;

    min_q = T.getRemaining(y).begin()->first; //smallest remaining processing time

    for (auto &r : T.getRemaining(y)) {
        int remaining_processing_time = r.first;   //Pj-y  ~ q
        double remaining_prob = r.second;

        val_rank = Reward(T,remaining_processing_time,y) / Investment(T, remaining_processing_time,y);
        if (val_rank > max_rank)
        {
            max_rank = val_rank;

        }
    }



    return make_pair(min_q,max_rank);
}

vector<pair<int, double>> GittinsRanks(Task T, int y)
{
    if (T.isGeometric()) return { {1, T.getAlpha()} };

    vector<pair<int, double>> ranks ;
    double val_rank;

    for (auto &r : T.getRemaining(y)) {
        int remaining_processing_time = r.first;   //Pj-y  ~ q
        double remaining_prob = r.second;

        val_rank = Reward(T,remaining_processing_time,y) / Investment(T, remaining_processing_time,y);

        ranks.push_back({remaining_processing_time, val_rank});
    }
    return ranks;
}


// Execute task on machines indepently of other: like the machines are not related
pair<double, double> GIPP_M1(vector<Job> &jobs) {

    int N= jobs.size();
    int M = jobs[0].getNbTask(); // number of machine
    vector<int> machineTime(M, 0); // local clock per machine

    while (any_of(jobs.begin(), jobs.end(), [](Job &j){ return !j.isCompleted(); })) {

        for(int m=0; m< M; m++){ // execute per machine independently

            // compute best index for tasks on machine m
            Task *best = nullptr;
            int best_q = -1;
            double best_rank = -1.0;
            int job_index ;

            // Find best job by Gittins index
            for (auto &J : jobs) {
                if (!J.isCompleted()) {
                    Task &T = J.getTask(m); // reference to task on machine m
                    if (T.isCompleted()) continue;
                    // cout <<"\t\t j"<< j.getId()<< " process: " <<  j.getProgress()<< endl;
                    auto [q, rank] = GittinsIndex(T, T.getProgress());

                    if (rank > best_rank) {
                        best_q = q;
                        best_rank = rank;
                        best = &T;
                        job_index= J.getId();
                        cout << "\tTask ("<<job_index<< ","<< best->getId()<<") : best q found= " << best_q << ", best rank= " << best_rank<< endl;

                    }
                }
            }

            // Execution
            if (best) {
                cout << "Machine "<<m<< " |Time=" << machineTime[m]
                    << " | Running Task ( " <<job_index
                    << ","<< best->getId()
                    << ")  for quantum q=" << best_q
                    << " (rank=" << best_rank << ")\n"
                    << endl;

                best->advance(best_q);   // update progress immediately
                machineTime[m] += best_q;

                if(best->isCompleted()) {
                    best->setCCT(machineTime[m]);
                    cout <<"\t====>Task ("<<job_index<< ","<<best->getId() <<") finished | cct= "<< best->getCCT()<<"\n"<< endl;
                }
            }

        }

         // Check job completion
        for (auto &J : jobs) {
            if (J.isCompleted()) continue;

            bool allTasksDone = true;
            int job_cct = 0;
            for (int t=0; t<J.getNbTask(); t++) {
                if (!J.getTask(t).isCompleted()) {
                    allTasksDone = false;
                    break;
                }
                job_cct = max(job_cct, J.getTask(t).getCCT());
            }

            if (allTasksDone) {
                J.setComplete(true);
                J.setCCT(job_cct);
                cout <<"\t*Job id: " <<J.getId() <<" finished | cct= "<< J.getCCT()<<"\n"<< endl;
            }
        }
    }
    // Weighted completion time
    double total = 0.0;
    for (auto &J : jobs) {
        total += J.getWeight() * J.getCCT();
    }
    // av_cct /= jobs.size();
    cout << "\t***** Weighted CCT: " << total << endl;

    double avg_cct = total/ N;
    return make_pair(total, avg_cct);
}



pair<int, int> GetBestJob_Min(vector<Job> &jobs, vector<int> &machineTime) {
    int M = machineTime.size();

    int best_job_index = -1;
    int global_best_q = -1;
    double global_best_rank = 1.1;

    for (int m = 0; m < M; m++) { // check each machine

        int local_best_q = -1;
        double local_best_rank = -1.0;
        int local_best_task_id = -1;
        int job_id=-1;
        if(debug_){
            cout << "\tMachine: " << m<< endl ;
        }
        // loop with index so we can safely reference jobs[]
        for (int i = 0; i < (int)jobs.size(); i++) {
            Job &J = jobs[i];
            if (J.isCompleted()) continue;


            Task &T = J.getTask(m);
            if (T.isCompleted()) {
                if(debug_) cout<<"\t\t|Task (" << i<<"," <<m<< ") already finished."<< endl;
                continue;
            }

            auto [q, rank] = GittinsIndex(T, T.getProgress());
            // cout << "Rank:" << rank <<endl;
            if(debug_){
                cout << "\t\t|Rank:" << rank<< "; local rank before:"
                    << local_best_rank<< "; task (" <<i <<","<< m
                    <<")"<<endl;
            }

            if (rank > local_best_rank) {
                local_best_q = q;
                local_best_rank = rank;
                local_best_task_id = m;
                job_id = i;
                if(debug_){
                    cout<<"\t\t|Local condition checked => local best rank: "
                    <<local_best_rank<<"\n"<< endl;
                }

            }

        }
        // if(debug_)
        //     cout<< "\t\t| Global rank before:" << global_best_rank
        //         <<"|Local_best_rank after:"<< local_best_rank<< endl;

        if (local_best_rank!=-1 & local_best_rank < global_best_rank ) { // Get best global rank over all machines
            global_best_rank = local_best_rank;
            global_best_q = local_best_q;
            best_job_index = job_id;
            if(debug_)
                cout <<"\n\t\t| Global condition checked => global best rank: "<< global_best_rank
                    <<"\n"<< endl;

        }

        if (local_best_task_id!= -1) {
            cout << "\t\t-Machine " << m
                << "| Task (" << jobs[job_id].getId()
                << "," << local_best_task_id
                << ") : best q=" << local_best_q
                << ", rank=" << local_best_rank
                <<"\n"<< endl;
        }

    }

    if (best_job_index != -1) {
        cout << "\t\t==> Selected job " << jobs[best_job_index].getId()
             << " | quantum=" << global_best_q << "\n" << endl;
    } else {
        cout << "\t\t==> No job available\n";
    }

    return make_pair(best_job_index, global_best_q);
}


pair<int, int> GetBestJob_Max(vector<Job> &jobs, vector<int> &machineTime) {
    int M = machineTime.size();

    int best_job_index = -1;
    int global_best_q = -1;
    double global_best_rank = -1.0;

    for (int m = 0; m < M; m++) { // check each machine

        int local_best_q = -1;
        double local_best_rank = -1.0;
        int local_best_task_id = -1;
        int job_id=-1;
        if(debug_){
            cout << "\tMachine: " << m<< endl ;
        }
        // loop with index so we can safely reference jobs[]
        for (int i = 0; i < (int)jobs.size(); i++) {
            Job &J = jobs[i];
            if (J.isCompleted()) continue;


            Task &T = J.getTask(m);
            if (T.isCompleted()) {
                if(debug_) cout<<"\t\t|Task (" << i<<"," <<m<< ") already finished."<< endl;
                continue;
            }

            auto [q, rank] = GittinsIndex(T, T.getProgress());
            // cout << "Rank:" << rank <<endl;
            if(debug_){
                cout << "\t\t|Rank:" << rank<< "; local rank before:"
                    << local_best_rank<< "; task (" <<i <<","<< m
                    <<")"<<endl;
            }

            if (rank > local_best_rank) {
                local_best_q = q;
                local_best_rank = rank;
                local_best_task_id = m;
                job_id = i;
                if(debug_){
                    cout<<"\t\t|Local condition checked => local best rank: "
                    <<local_best_rank<<"\n"<< endl;
                }

            }

        }
        // if(debug_)
        //     cout<< "\t\t| Global rank before:" << global_best_rank
        //         <<"|Local_best_rank after:"<< local_best_rank<< endl;

        if (local_best_rank!=-1 & local_best_rank > global_best_rank ) { // Get best global rank over all machines
            global_best_rank = local_best_rank;
            global_best_q = local_best_q;
            best_job_index = job_id;
            if(debug_)
                cout <<"\n\t\t| Global condition checked => global best rank: "<< global_best_rank
                    <<"\n"<< endl;

        }

        if (local_best_task_id!= -1) {
            cout << "\t\t-Machine " << m
                << "| Task (" << jobs[job_id].getId()
                << "," << local_best_task_id
                << ") : best q=" << local_best_q
                << ", rank=" << local_best_rank
                <<"\n"<< endl;
        }

    }

    if (best_job_index != -1) {
        cout << "\t\t==> Selected job " << jobs[best_job_index].getId()
             << " | quantum=" << global_best_q << "\n" << endl;
    } else {
        cout << "\t\t==> No job available\n";
    }

    return make_pair(best_job_index, global_best_q);
}


pair<double, double> GIPP_M2(vector<Job> &jobs, bool aggrega) {// aggrega: 0/1 for min/max over all local ranks
    int N = jobs.size();
    int M = jobs[0].getNbTask(); // number of machine
    vector<int> machineTime(M, 0); // local clock per machine



    while (any_of(jobs.begin(), jobs.end(), [](Job &j){ return !j.isCompleted(); })) {
        cout<< "-Selection:" << endl;
        auto [job_id, q] = (!aggrega)
            ? GetBestJob_Min(jobs, machineTime)
            : GetBestJob_Max(jobs, machineTime);

        if(job_id == -1 or q==-1 ) break;

        cout<< "-Execution: "<< endl;
        for(int m=0; m< M; m++){ // execution on machine
            Task &T = jobs[job_id].getTask(m);

            if (T.isCompleted()) continue;

            cout << "\tMachine "<<m<< " |Start Time=" << machineTime[m]
                << " | Running Task ( " <<job_id
                << ","<<T.getId()
                << ")  for quantum q=" << q
                << " \n"
                << endl;

            T.advance(q,machineTime[m]);
            machineTime[m] += q;

            if(T.isCompleted()) {
                cout<<"\t\t======>Task ("<<job_id<< ","<<T.getId()
                    <<") finished | cct= "<< T.getCCT()<<"\n"<< endl;
            }

        }


         // Check job completion
        for (auto &J : jobs) {
            if (J.isCompleted()) continue;

            bool allTasksDone = true;
            int job_cct = 0;
            for (int t=0; t<J.getNbTask(); t++) {
                if (!J.getTask(t).isCompleted()) {
                    allTasksDone = false;
                    break;
                }
                job_cct = max(job_cct, J.getTask(t).getCCT());
            }

            if (allTasksDone) {
                J.setComplete(true);
                J.setCCT(job_cct);
                cout <<"\t*Job id: " <<J.getId() <<" finished | cct= "<< J.getCCT()<<"\n"<< endl;
            }
        }
    }
    // Weighted completion time
    double total = 0.0;
    for (auto &J : jobs) {
        total += J.getWeight() * J.getCCT();
    }
    // av_cct /= jobs.size();
    cout << "\t***** Weighted CCT: " << total << endl;
    double avg_cct = total/ N;
    return make_pair(total, avg_cct);
}



// *************************************************** */
//                  GIPP : Heuristic
//
//**************************************************** */




double unfinishedWeightSum(vector<Job>& jobs){
    double running_weight = 0.0;
    for (const auto& J : jobs) {
        if (!J.isCompleted()) running_weight += J.getWeight();
    }
    return running_weight;
}


JobInfo GittinsSingleMachine(vector<Job>& jobs, int b){
    int N = jobs.size();
    if (N == 0 || b >= jobs[0].getNbTask()) return {};

    JobInfo Best_job{-1,0.0,0};

    for(int j=0;j<N;j++){
        const Task& T = jobs[j].getTask(b); //task on bottleneck
        if (T.isCompleted()) {
            if(debug_){
                cout <<  "- Bottleneck " << b
                    << " | Task("<<j<<","<<b<<"); proc= "<<T.getProgress()<<"; isCompleted= "<< T.isCompleted()<<" already completed -> skipped" << endl;}
            continue;
        }

           
        auto [q, rank] = GittinsIndex_MinQ(T, T.getProgress());
        if (rank > Best_job.rank){
            Best_job = {j, rank, q};
        }
    }

    if (Best_job.job_id == -1){
        throw runtime_error("No eligible job found for bottleneck");
    }
    else return Best_job;
}


pair<ActionsQuantum, double> chooseGIPPAction_M3(vector<Job> &jobs)
{
    // debug_ = true;
    if (jobs.empty()) {
        cout << "jobs empty" << endl;
        return {{Action{}, 0}, 0.0};
    }

    if(debug_) cout << "\n-Processing:\n";
    int step=1;
    int N = jobs.size();
    int M = jobs[0].getNbTask();

    set<JobInfo> SelectedJobs;

    for (int j = 0; j < N; j++) {
        if (jobs[j].isCompleted()) continue;

        double local_rank = numeric_limits<double>::infinity();
        int local_job = -1;
        int local_min_q = numeric_limits<int>::max();
        if(debug_)
            cout << "\n"<< indent(step)<<  "[Job " << jobs[j].getId() << "]" << endl;

        for (int m = 0; m < M; m++) {
            const Task& T = jobs[j].getTask(m);
            if (T.isCompleted()) {
                if(debug_)
                    cout << indent(step+1)<<  "- Machine " << m
                        << " | Task("<<j<<","<<m<<"); proc= "<<T.getProgress()<<"; isCompleted= "<< T.isCompleted()<<" already completed -> skipped" << endl;
                continue;
            }

            // auto [q, rank] = GittinsIndex(T, T.getProgress());
            auto [q, rank] = GittinsIndex_MinQ(T, T.getProgress());

            if(debug_)
                cout << "\n"<< indent(step+1)<<"- Machine " << m
                    << " | Task (" << jobs[j].getId() << "," << m << ")"
                    << " | candidate quantum = " << q
                    << " | rank = " << rank << endl;


            if(local_min_q > q) {
                local_min_q = q;
                if(debug_) cout << indent(step+2)<<  "| New local min quantum = " << local_min_q <<endl;
            }

            if (rank < local_rank) {
                local_rank = rank;
                local_job = j;

                if (debug_) {
                    cout << indent(step+2)<<  "-> bottleneck task(" << jobs[j].getId()
                         << "," << m
                         << ") | local min quantum = " << local_min_q
                         << " | local rank = " << local_rank << endl;
                }
            }
        }

        if(debug_)
            cout << "\n"<< indent(step)<<  "=> Job " << jobs[j].getId()
                << " summary: quantum = " << local_min_q
                <<" ,bottleneck rank = " << local_rank<< endl;

        if (local_job != -1) {
            SelectedJobs.insert({local_job, local_rank, local_min_q});
        }

    }


    if (SelectedJobs.empty()) {
        Action idle(M, -1);
        return {{idle, 0}, 0.0};
    }

    auto current_it = SelectedJobs.begin();

    if(debug_)
        cout << "\n"<< indent(step)<<  " [Global selection]"
            << " Selected reference job = " << current_it->job_id
            << " | bottleneck quantum = " << current_it->q
            << " | bottleneck rank = " << current_it->rank << endl;

    Action A(M, -1);
    int best_job = -1;
    double best_rank = current_it->rank;
    int best_q = current_it->q;
    int min_q = numeric_limits<int>::max();   // quantum = smallest remaining time among the tasks actually run

    if(debug_) cout<<"\n"<<indent(step) << "[Machine assignment]" << endl;

    for (int m = 0; m < M; m++) {
        bool found = false;

        for (auto it = current_it; it != SelectedJobs.end(); ++it) {
            int candidate_job = it->job_id;

            Task& Tm = jobs[candidate_job].getTask(m);
            if (!Tm.isCompleted()) {
                best_job = candidate_job;
                best_rank = it->rank;

                // Limit the quantum to THIS task's own remaining time on THIS machine,
                // not to the job's globally-smallest task (local_min_q, which can be ~1
                // and forces one recursion level per unit of the longest task -> stack overflow).
                int q_m = GittinsIndex_MinQ(Tm, Tm.getProgress()).first;
                best_q = q_m;
                if (q_m < min_q) min_q = q_m;

                A[m] = best_job;
                found = true;

                if(debug_)
                    cout << indent(step+3)<<  "- Machine " << m
                        << " assigned to Job " << jobs[best_job].getId()
                        << " | task remaining = " << q_m
                        << " | quantum = " << min_q
                        << " | rank = " << best_rank << endl;
                break;
            }
        }

        if (!found) {
            A[m] = -1;
            if(debug_)
                cout << indent(step+3)<<  "- Machine " << m
                    << " assigned to no job (all candidate tasks completed)" << endl;
        }
    }

    if (min_q == numeric_limits<int>::max()) min_q = 0;   // no machine received a job

    if(debug_){
        cout << "\n"<< indent(step)<< "[Final action]";
        printAction(A,step);
        cout << "| common execution quantum = " << min_q
            << " | reference rank = " << best_rank << endl;
    }

    // debug_ = false;
    return {{A, min_q}, best_rank};
}



pair<double, double> GIPP_M3(vector<Job>& jobs) {
    int N = jobs.size();
    int M = jobs[0].getNbTask();
    vector<int> machineTime(M, 0);

    double total_value = 0.0;


    while (any_of(jobs.begin(), jobs.end(), [](Job &j){ return !j.isCompleted(); })) {


        auto best = chooseGIPPAction_M3(jobs);
        Action A = best.first.A;
        int quantum = best.first.q;
        double rank = best.second;

        cout << "-Execution:\n";

        for (int m = 0; m < M; m++) {
            if (A[m] == -1) continue; // the job is not processing on this machine
            int job_id = A[m];
            Task &T = jobs[job_id].getTask(m);

            cout << "\tMachine " << m
                 << " | Start Time = " << machineTime[m]
                 << " | Running Task (" << jobs[job_id].getId()
                 << "," << T.getId()
                 << ") for quantum q = " << quantum << "\n" << endl;

            T.advance(quantum, machineTime[m]);
            machineTime[m] += quantum;

            if (T.isCompleted()) {
                cout << "\t\t======> Task (" << jobs[job_id].getId()
                     << "," << T.getId()
                     << ") finished | cct = " << T.getCCT() << "\n" << endl;
            }
        }


        total_value += unfinishedWeightSum(jobs) * quantum;

        for (auto &J : jobs) {
            if (J.isCompleted()) continue;

            bool allTasksDone = true;
            int job_cct = 0;

            for (int t = 0; t < J.getNbTask(); t++) {
                if (!J.getTask(t).isCompleted()) {
                    allTasksDone = false;
                    break;
                }
                job_cct = max(job_cct, J.getTask(t).getCCT());
            }

            if (allTasksDone) {
                J.setComplete(true);
                J.setCCT(job_cct);
                cout << "\t* Job id: " << J.getId()
                     << " finished | cct = " << J.getCCT() << "\n" << endl;
            }
        }


    }

    double total_wct = 0.0;
    for (auto &J : jobs) {
        total_wct += J.getWeight() * J.getCCT();
    }
    cout << "\t***** Accumulated value = " << total_value << endl;
    cout << "\t***** Weighted completion time = " << total_wct << endl;



    // cout << "\t***** Weighted CCT: " << total_wct << endl;
    double avg_cct = total_wct / N;
    return make_pair(total_wct, avg_cct);
}






ActionsQuantum chooseGIPPAction_M4(vector<Job>& jobs)
{
    // debug_ = true;
    if (jobs.empty()) {
        cout << "jobs empty" << endl;
        return {Action{}, 0};
    }

    if(debug_) cout << "\n-Processing:\n";
    int step=1;
    int N = jobs.size();
    int M = jobs[0].getNbTask();

    Action A(M, -1);
    int select_q = numeric_limits<int>::max();

    set<JobInfo> SelectedJobs;
    for (int i=0; i<M; i++){

        double max_rank = - numeric_limits<double>::infinity();
        int select_job = -1; 
        int quantum = numeric_limits<int>::max();    

        for (int j = 0; j < N; j++) {
            if (jobs[j].isCompleted()) continue;

            const Task& T = jobs[j].getTask(i);
            if (T.isCompleted()) {
                if(debug_)
                    cout << indent(step+1)<<  "- Machine " << i
                        << " | Task("<<j<<","<<i<<"); proc= "<<T.getProgress()<<"; isCompleted= "<< T.isCompleted()<<" already completed -> skipped" << endl;
                continue;
            }

           
            auto [q, rank] = GittinsIndex_MinQ(T, T.getProgress());

            if (rank > max_rank) {
                max_rank = rank;
                select_job = j;
                quantum= q;
                
                if (debug_) {
                    cout << indent(step+2)<<  "-> bottleneck task(" << jobs[j].getId()
                         << "," << i
                         << ") | quantum = " << quantum
                         << " | max rank = " << max_rank << endl;
                }
            }
        }

        if (select_job != -1) {
            A[i] = select_job;
            if (select_q > quantum) select_q = quantum;

        }

    }

    if(debug_) {
        cout<<"\n"<<indent(step) << "[Machine assignment]" << endl;
        for (int i=0; i<M; i++){
            if (A[i] != -1)
                cout << indent(step+3)<<  "- Machine " << i
                    << " assigned to Job " << jobs[A[i]].getId()
                    << " | quantum = " << select_q << endl;
            else 
                cout << indent(step+3)<<  "- Machine " << i
                    << " assigned to no job (all candidate tasks completed)" << endl;
        }
    }
  

    if(debug_){
        cout << "\n"<< indent(step)<< "[Final action]";
        printAction(A,step);
    }

   
    return {A, select_q};
}

ActionsQuantum chooseGIPPAction_M5(vector<Job>& jobs)
{
    // debug_ = true;
    if (jobs.empty()) {
        cout << "jobs empty" << endl;
        return {Action{}, 0};
    }

    if(debug_) cout << "\n-Processing:\n";
    int step=1;
    int N = jobs.size();
    int M = jobs[0].getNbTask();

    Action A(M, -1);
    int select_q = numeric_limits<int>::max();

    set<JobInfo> SelectedJobs;
    for (int i=0; i<M; i++){

        double min_rank = numeric_limits<double>::infinity();
        int select_job = -1; 
        int quantum = numeric_limits<int>::max();    

        for (int j = 0; j < N; j++) {
            if (jobs[j].isCompleted()) continue;

            const Task& T = jobs[j].getTask(i);
            if (T.isCompleted()) {
                if(debug_)
                    cout << indent(step+1)<<  "- Machine " << i
                        << " | Task("<<j<<","<<i<<"); proc= "<<T.getProgress()<<"; isCompleted= "<< T.isCompleted()<<" already completed -> skipped" << endl;
                continue;
            }

           
            auto [q, rank] = GittinsIndex_MinQ(T, T.getProgress());

            if (rank < min_rank) {
                min_rank = rank;
                select_job = j;
                quantum= q;
                
                if (debug_) {
                    cout << indent(step+2)<<  "-> bottleneck task(" << jobs[j].getId()
                         << "," << i
                         << ") | quantum = " << quantum
                         << " | min rank = " << min_rank << endl;
                }
            }
        }

        if (select_job != -1) {
            A[i] = select_job;
            if (select_q > quantum) select_q = quantum;

        }

    }

    if(debug_) {
        cout<<"\n"<<indent(step) << "[Machine assignment]" << endl;
        for (int i=0; i<M; i++){
            if (A[i] != -1)
                cout << indent(step+3)<<  "- Machine " << i
                    << " assigned to Job " << jobs[A[i]].getId()
                    << " | quantum = " << select_q << endl;
            else 
                cout << indent(step+3)<<  "- Machine " << i
                    << " assigned to no job (all candidate tasks completed)" << endl;
        }
    }
  

    if(debug_){
        cout << "\n"<< indent(step)<< "[Final action]";
        printAction(A,step);
    }

   
    return {A, select_q};
}


pair<double, double> GIPP_M4(vector<Job>& jobs) {
    int N = jobs.size();
    int M = jobs[0].getNbTask();
    vector<int> machineTime(M, 0);

    double total_value = 0.0;


    while (any_of(jobs.begin(), jobs.end(), [](Job &j){ return !j.isCompleted(); })) {


        auto best = chooseGIPPAction_M4(jobs);
        Action A = best.A;
        int quantum = best.q;

        cout << "-Execution:\n";

        for (int m = 0; m < M; m++) {
            if (A[m] == -1) continue; // the job is not processing on this machine
            int job_id = A[m];
            Task &T = jobs[job_id].getTask(m);

            cout << "\tMachine " << m
                 << " | Start Time = " << machineTime[m]
                 << " | Running Task (" << jobs[job_id].getId()
                 << "," << T.getId()
                 << ") for quantum q = " << quantum << "\n" << endl;

            T.advance(quantum, machineTime[m]);
            machineTime[m] += quantum;

            if (T.isCompleted()) {
                cout << "\t\t======> Task (" << jobs[job_id].getId()
                     << "," << T.getId()
                     << ") finished | cct = " << T.getCCT() << "\n" << endl;
            }
        }


        total_value += unfinishedWeightSum(jobs) * quantum;

        for (auto &J : jobs) {
            if (J.isCompleted()) continue;

            bool allTasksDone = true;
            int job_cct = 0;

            for (int t = 0; t < J.getNbTask(); t++) {
                if (!J.getTask(t).isCompleted()) {
                    allTasksDone = false;
                    break;
                }
                job_cct = max(job_cct, J.getTask(t).getCCT());
            }

            if (allTasksDone) {
                J.setComplete(true);
                J.setCCT(job_cct);
                cout << "\t* Job id: " << J.getId()
                     << " finished | cct = " << J.getCCT() << "\n" << endl;
            }
        }


    }

    double total_wct = 0.0;
    for (auto &J : jobs) {
        total_wct += J.getWeight() * J.getCCT();
    }
    cout << "\t***** Accumulated value = " << total_value << endl;
    cout << "\t***** Weighted completion time = " << total_wct << endl;



    // cout << "\t***** Weighted CCT: " << total_wct << endl;
    double avg_cct = total_wct / N;
    return make_pair(total_wct, avg_cct);
}





//------------------------------------------------------------------//
//                  Value Function Evaluation                      //
//                                                                  //
//                                                                  //
//------------------------------------------------------------------//


double getImmediateCostWithQuantum(const Matrix& S,const vector<Job>& jobs,int quantum)
{
    double sum_w = 0.0;

    for (int j = 0; j < (int)S.size(); ++j) {
        bool finished = true;
        for (int i = 0; i < (int)S[j].size(); ++i) {
            if (S[j][i] != -1) {
                finished = false;
                break;
            }
        }
        if (!finished) sum_w += jobs[j].getWeight();
    }

    return sum_w * quantum;
}



vector<NextState> expandOneMachine(const vector<NextState>& states,int machine_id,int job_id,
                            int quantum,const vector<Job>& jobs,int depth)
{
    vector<NextState> out;
    out.reserve(states.size() * 2);

    for (const auto& st : states) {
        auto Y = st.Y;
        auto S = st.S;
        double p = st.p;

        if (debug_){
        cout <<indent(depth)<< "Execution job " << job_id << " on machine: "<< machine_id << "- q= " << quantum<<endl;
        cout << indent(depth)<< "Current State: "<< endl;
        printState(Y,depth);
        }
        // printState(S,0);
        // IDLE
        if ((int)job_id == -1) {
            if (debug_)
                cout << indent(depth)<< "- job id=-1: no job selected " << endl;

            out.push_back({Y, S, p}); // no changing
            continue;
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

        int proc_time = Y[job_id][machine_id];
        // Already finished in Y
        if (proc_time == -1) {
            // Keep consistent: if set Y=-1, S should also be -1
            S[job_id][machine_id] = -1;
            out.push_back({Y, S, p});
            continue;
        }

        // Geometric task (memoryless): see expandGeometricTask in ValueIteration.cc
        if (task.isGeometric()) {
            expandGeometricTask(Y, S, p, job_id, machine_id, quantum, task, out, depth);
            continue;
        }

        int current_phase_id = phase_id;
        while (true) {
            const Phase& phase = phases[current_phase_id];

            if (proc_time + quantum <= phase.duration) {
                break;
            }

            current_phase_id++;

            if (current_phase_id >= (int)phases.size()) { // to verify
                // task finished
                Y[job_id][machine_id] = -1;
                S[job_id][machine_id] = -1;
                out.push_back({Y, S, p});
                // continue;
                break;
            }
        }
        if (current_phase_id != (int)phases.size()){
            phase_id = current_phase_id;
            S[job_id][machine_id] = phase_id;

            const Phase& phase = phases[phase_id];

            // cout << indent(depth)<< "-phase_id = " <<phase_id << "; phases.size() = "<< phases.size() <<endl;

            // cout <<indent(depth) << "proc_time= "<<proc_time<< "; quantum= "<<quantum << "; phase.duration= "<<phase.duration<< endl;

            if (proc_time + quantum < phase.duration) {
                Y[job_id][machine_id] += quantum;
                out.push_back({Y, S, p});

                if (debug_){
                    cout << indent(depth)<< "=>Next state within phase - with proba = " << p << endl;
                    printState(Y, depth);

                }

                continue;
            }
            // Phase completes now
            if (proc_time + quantum == phase.duration) {
                // cout << indent(depth) << "proc_time + quantum" << proc_time + quantum << "; phase.duration= "<<phase.duration<< endl;

                bool last = (phase_id == (int)phases.size() - 1);

                if (!last) {
                    // SUCCESS -> next phase
                    auto Y_succ = Y;
                    auto S_succ = S;
                    Y_succ[job_id][machine_id] += quantum;
                    S_succ[job_id][machine_id] += 1;
                    out.push_back({Y_succ, S_succ, p * phase.beta_prob});

                    if (debug_){
                        cout << indent(depth)<< "=>Next state after reashed current phase- with proba = " << p * phase.beta_prob << endl;
                        printState(Y_succ, depth);
                    }

                }

                // FINISH/FAIL -> task ends
                auto Y_fin = Y;
                auto S_fin = S;
                Y_fin[job_id][machine_id] = -1;
                S_fin[job_id][machine_id] = -1;
                out.push_back({Y_fin, S_fin, p * (1.0 - phase.beta_prob)}); // 1-phase.beta_prob not corret when reached final terminal

                if (debug_){
                    cout << indent(depth) << "=>Final state - with proba = " << p * (1-phase.beta_prob) << endl;
                    printState(Y_fin, depth);
                }

                continue;
            }

        }

    }

    return out;
}


vector<NextState> generateNextStates(const Matrix& Y0,const Matrix& S0,const vector<Job>& jobs,const Action& A, int quantum,int depth)
{
    vector<NextState> states = { {Y0, S0, 1.0} };
  
    for (int m = 0; m < (int)A.size(); ++m) {
        states = expandOneMachine(states, m, A[m], quantum , jobs, depth);
    }
   
    return states;
}




vector<Job> BuildJobsFromState(const Matrix& Y,const Matrix& S,const vector<Job>& jobs_template)
{
    vector<Job> jobs_copy = jobs_template;

    for (int j = 0; j < (int)jobs_copy.size(); ++j) {
        bool jobCompleted = true;

        for (int i = 0; i < jobs_copy[j].getNbTask(); ++i) {
            Task& T = jobs_copy[j].getTask(i);

            if (S[j][i] == -1 &&  Y[j][i] == -1) {
                // task completed
                // cout << "task ("<<j<<","<<i<<") is complete; Max proc = "<<T.getMaxProcTime() <<endl;
                T.setProgress(T.isGeometric() ? 0 : T.getMaxProcTime()); // no max for geometric
                T.setComplete(true);

            } else {
                jobCompleted = false;

                T.setProgress(Y[j][i]);
                T.setComplete(false);
                // cout << "task ("<<j<<","<<i<<") not complete; Task progress = "<<T.getProgress() <<endl;

            }
        }

        jobs_copy[j].setComplete(jobCompleted);
    }

    return jobs_copy;
}


ActionsQuantum policyGIPP(const Matrix& Y,const Matrix& S,const vector<Job>& jobs_template, int version)
{
    vector<Job> local_jobs = BuildJobsFromState(Y, S, jobs_template);
    ActionsQuantum best;
    if(version == 3){
        best = chooseGIPPAction_M3(local_jobs).first;
    }
    else if (version == 4){
        best = chooseGIPPAction_M4(local_jobs);
    }
    else if (version == 5){
        best = chooseGIPPAction_M5(local_jobs);
    }
    else {
        cout <<"Error: Incorrect version" <<endl;
        exit;
    }
    
    return best;
}



double PolicyQFunction_GIPP(const Matrix& Y,const Matrix& S,const vector<Job>& jobs,const vector<Job>& jobs_determinist,
    const ActionsQuantum& decision,map<string, double>& gipp_memories,map<string, ActionsQuantum>& gipp_decisions,
    map<string, DecisionValue>& gipp_policy, int version,int depth)
{
    double cost = getImmediateCostWithQuantum(S, jobs, decision.q);

    // Transition follows the sampled realization jobs_determinist, not the true distribution.
    auto next_states = generateNextStates(
        Y, S, jobs_determinist, decision.A, decision.q, depth);

    // Self-loop (geometric tasks): see solveSelfLoop in ValueIteration.cc
    double p_self = 0.0;
    double qval = cost;
    for (const auto& ns : next_states) {
        if (isSameState(ns, Y, S)) { p_self += ns.p; continue; }
        qval += ns.p * PolicyValueFunction_GIPP(
            ns.Y, ns.S, jobs, jobs_determinist, (int)decision.A.size(), gipp_memories, gipp_decisions,gipp_policy, version, depth + 1);
    }

    return solveSelfLoop(qval, p_self);
}


//------------------------------------//
//            Memoization             //
// Cache for already-computed states  //
//------------------------------------//


double PolicyValueFunction_GIPP(const Matrix& Y,const Matrix& S,
    const vector<Job>& jobs,const vector<Job>& jobs_determinist,int n_machines,map<string, double>& gipp_memories,
    map<string, ActionsQuantum>& gipp_decisions,map<string, DecisionValue>& gipp_policy, int version,int depth)
{
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
    auto it = gipp_memories.find(key);
    if (it != gipp_memories.end()) return it->second;

    if (debug_) {
        cout <<"\n"<< indent(depth) << "Evaluating V(Y) depth=" << depth << endl;
        printState(Y, depth);
    }

    ActionsQuantum decision = policyGIPP(Y, S, jobs, version);
    // debug_ = true;
    if (debug_){
        cout << indent(depth) << "Action : ";
        printAction(decision.A, 0);
        cout  << " - quantum : " << decision.q << endl;;

    }
    // debug_ = false;
    double val = PolicyQFunction_GIPP(
        Y, S, jobs, jobs_determinist, decision, gipp_memories, gipp_decisions,gipp_policy, version, depth);

    gipp_memories[key] = val;
    gipp_decisions[key] = decision;
    gipp_policy[key] = {decision.A, decision.q, val};

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

double PolicyValueFunction_GIPP(const Matrix& Y,const Matrix& S,
    const vector<Job>& jobs,int n_machines,map<string, double>& gipp_memories,
    map<string, ActionsQuantum>& gipp_decisions,map<string, DecisionValue>& gipp_policy, int version,int depth){
    return PolicyValueFunction_GIPP(Y, S, jobs, jobs, n_machines, gipp_memories, gipp_decisions, gipp_policy, version, depth);
}



void verifyVIvsPE(int n, int m,const vector<Job>& jobs, map<string, DecisionValue>& policy, string filename, string mode, int version){
    // vector<string> differing_states;  // Collect diffs first
    ofstream out(filename);

    streambuf* old_buf = cout.rdbuf(out.rdbuf()); // redirect cout to file
    
    cout << "\n******** Jobs Description *******\n" ;
    JobsDescription(jobs);

    cout << "\n******** Policies comparison: VI("+mode+") VS GitHeur ********" << endl;
    map<string, DecisionValue> differing_states;
    
    cout << "\n\t NUMBER OF STATES : " << policy.size()<< " \n";
    // Step 1: Silent computation
    debug_ = false;
    for (const auto& s : policy) {
        Matrix Y, S;
        decode_states_key(s.first, n, m, Y, S);
        cout << endl;printState(Y);
        ActionsQuantum decision = policyGIPP(Y, S, jobs, version);
        // int q_vi = s.second.q;
        cout << "VI- A: " ;
        printAction(s.second.A);
        cout << " - q: " << s.second.q<< endl;

        cout << "PE(Githeur)- A: " ;
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
            ActionsQuantum decision = policyGIPP(Y, S, jobs, version);  

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

            cout << "PE- A: " ;
            printAction(decision.A);
            cout << " - q: " << decision.q<< endl;


        }
        debug_ = false;
    } else {
        cout << "All states match! No debug needed.\n";
    }
    cout.rdbuf(old_buf); // restore cout
}