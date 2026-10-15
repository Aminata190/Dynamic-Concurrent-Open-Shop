#include <math.h>
#include <vector>
#include <random>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <unistd.h>
#include <cstdlib>

#include "Stochastic.hxx"

using namespace std;
namespace fs = filesystem;



void usage(const char* prog){
    cerr << "Usage: " << prog
         << " [-a start] [-b end] [-l allocation] [-g generationType] [-p] [-c policyDir]"
         << " <N> <m> <algo> <inputDir> <outputDir>\n"
         << "\t algo           : o = VI (optimal), g = GitHeur, s = Sincronia, t = Static\n"
         << "\t -l allocation  : VI only, p/1 = phase, s/0 = semi-phase (default), u/-1 = unit\n"
         << "\t -g generation  : VI only, 0 = recursive, 1 = order (default), 2 = successive\n"
         << "\t -p             : VI only, save the policy in <outputDir>/policy_<instance>\n"
         << "\t -c policyDir   : g or s only, compare with the VI policy <policyDir>/policy_<instance>\n";
}



int main(int argc, char **argv) {
    int             c;
    int             start_file= 0;
    int             end_file= 1000;
    int             N,m;
    string          algo;
    int             allocation_mode = 0;    // semi-phase
    int             generationType = 1;     // order-based
    bool            savePolicy = false;
    string          policyDir = "";         // empty: no comparison
    string          gentype;                  // d : discrete, g : geometric
    bool            printDecision = false;

    while ((c = getopt(argc, argv, "a:b:l:g:pc:")) != -1) {
        switch (c) {
            case 'a':
                start_file = atoi(optarg);
                break;
            case 'b':
                end_file = atoi(optarg);
                break;
            case 'l': {
                string allocation = optarg;
                if (allocation == "p" || allocation == "1") allocation_mode = 1;
                else if (allocation == "s" || allocation == "0") allocation_mode = 0;
                else if (allocation == "u" || allocation == "-1") allocation_mode = -1;
                else {
                    cerr << "Invalid allocation: " << allocation << ". Use 'p' or 'u' or 's' (or 1/0/-1).\n";
                    return 1;
                }
                break;
            }
            case 'g':
                generationType = atoi(optarg);
                break;
            case 'p':
                savePolicy = true;
                break;
            case 'c':
                policyDir = optarg;
                break;
            default:
                usage(argv[0]);
                return 1;
        }
    }
    if (argc - optind < 6) {
        usage(argv[0]);
        return 1;
    }

    N = atoi(argv[optind]);
    m = atoi(argv[optind + 1]);
    algo = argv[optind + 2];
    gentype = argv[optind + 3];


    fs::path inputDir = argv[optind + 4];
    fs::path outputDir = argv[optind + 5];

    if (!isValidAlgo(algo)) {
        cerr << "Invalid algo: " << algo << "\n";
        usage(argv[0]);
        return 1;
    }
    if (generationType < 0 || generationType > 2) {
        cerr << "Invalid generationType: " << generationType << "\n";
        return 1;
    }

    if (!fs::exists(outputDir)) {
        fs::create_directories(outputDir);
    }

    string algo_type = algoName(algo);

    // Same file names and headers as the former exec_vi / exec_githeur / exec_sincronia
    string results_prefix = (algo == "o") ? "Results_opt_N" : "Results_N";
    string exec_prefix    = (algo == "o") ? "exec_opt_" : "exec_";

    string results_filename = outputDir /(results_prefix+to_string(N)+"_m"+to_string(m)+"_"+to_string(start_file)+"_"+to_string(end_file)+".txt");
    ofstream out_res(results_filename, ios::out);

    if (algo == "o"){
        if (generationType == 0)      out_res << "Inst N°    VI(Actions Recursive Generation)       Execution Time\n";
        else if (generationType == 1) out_res << "Inst N°    VI(Actions Order Generation)       Execution Time\n";
        else                          out_res << "Inst N°    VI(Actions Successive Generation)       Execution Time\n";
    }
    else if (algo == "g") out_res << "Inst N°    GitHeur       Execution Time\n";
    else if (algo == "s") out_res << "Inst N°    Sincronia       Execution Time\n";
    else                  out_res << "Inst N°    Static       Execution Time\n";


    vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(inputDir)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path());
        }
    }

    sort(files.begin(), files.end(), [](const fs::path& a, const fs::path& b) {
        return extract_index(a) < extract_index(b);
    });

    if (files.empty()) {
        cerr << "No files found in " << inputDir << "\n";
        return 1;
    }

    if (end_file >= static_cast<int>(files.size())) { // end_file must not exceed directory files
        end_file = static_cast<int>(files.size()) - 1;
    }

    if (start_file > end_file) {
        cerr << "Invalid range: start_file > end_file\n";
        return 1;
    }

    for (int i = start_file; i <= end_file; ++i) {
        const fs::path& filename = files[i];
        cout << "\tInstance " << i << "/" << end_file << endl;

        fs::path out_filename = outputDir/ (exec_prefix + filename.filename().string());

        vector<Job> jobs = readJobsFromFile(filename);

        ofstream out(out_filename);

        streambuf* old_buf = cout.rdbuf(out.rdbuf()); // redirect cout to file


        if (gentype == "g")
            cout << "\n******** Jobs(Geo distribution Generation) Description *******" << endl;
        else
            cout << "\n******** Jobs(Discrete distribution Generation) Description *******" << endl;
        
        JobsDescription(jobs);

        cout << "******************** Stochastic Computing(" <<algo_type <<") ********************"<< endl;

        for (auto &J : jobs)
            JobTasksInPhase(J, allocation_mode);

        map<string, DecisionValue> policy;

        auto start = chrono::high_resolution_clock::now();

        // debug_ = true;
        double V = stochasticValue(jobs, m, algo, policy, allocation_mode, generationType);

        auto end = chrono::high_resolution_clock::now();

        chrono::duration<double> elapsed = end - start;

        cout << "\t====>Value Function of Initial State (" <<algo_type<< ") = " << V << endl;
        cout << "\t| Execution time: " << elapsed.count()*1000 << " *10⁻³ seconds |\n";

        out_res << i << " " << V << "  " << elapsed.count()*1000 << "\n";

        // printDecision = true;
        if (printDecision){
            map<string, DecisionValue> policy_final;
            printDecisionsTrajectoryPerPhase(jobs, policy, policy_final);
        }
        if (savePolicy && algo == "o"){
            map<string, DecisionValue> policy_final;
            printDecisionsTrajectoryPerPhase(jobs, policy, policy_final);
            save_policy(policy_final, outputDir/ ("policy_" + filename.filename().string()));
        }

        cout.rdbuf(old_buf);


        if (!policyDir.empty()){
            fs::path policy_file     = fs::path(policyDir)/ ("policy_" + filename.filename().string());
            fs::path comparison_file = outputDir/ ("comparison_" + filename.filename().string());
            cerr << "Policies comparison: VI VS " << algo_type << " (" << policy_file << ")" << endl;
            compareWithVI(N, m, jobs, algo, policy_file, comparison_file);
        }
    }

    return 0;
}
