#include <math.h>
#include <vector>
#include <random>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <unistd.h>   
#include <cstdlib> 

#include "SAA.hxx"

using namespace std;
namespace fs = filesystem;



int main(int argc, char **argv) {
    int             c;
    int             start_file= 0;
    int             end_file= 1000;
    int             N,m,minInstances,nbInstancesMax,batchSize,requiredStableBatches;
    float           tolerance,cv;
    string          algo; 
    string          gentype;                  // d : discrete, g : geometric

    while ((c = getopt(argc, argv, "a:b:")) != -1) {
        switch (c) {
            case 'a':
                start_file = atoi(optarg);
                break;
            case 'b':
                end_file = atoi(optarg);
                break;
            default:
                cerr << "Usage: " << argv[0]
                          << " [-a start] [-b end] <N> <m> <algo> <nbInstancesMax> <batchSize> <tolerance> <requiredStableBatches> <inputDir> <outputDir>\n"; 
                return 1;
        }
    }
    if (argc - optind < 10) {
        cerr << "Usage: " << argv[0]
                  << " [-a start] [-b end] <N> <m> <algo> <nbInstancesMax> <batchSize> <tolerance> <requiredStableBatches> <inputDir> <outputDir>\n"; 
        return 1;
    }

    N = atoi(argv[optind]);
    m = atoi(argv[optind + 1]);

    algo = argv[optind + 2];
    
    nbInstancesMax = atoi(argv[optind + 3]); minInstances=100;
    batchSize = atoi(argv[optind + 4]);
    tolerance = stof(argv[optind + 5]);
    requiredStableBatches = atoi(argv[optind + 6]);

    cv = atof(argv[optind + 7]);

    gentype = argv[optind + 8];
    
    
    if (nbInstancesMax <= 0) {
        cerr << "Error: nbInstances must be > 0, got: " << argv[optind + 3] << "\n";
        cerr << "Check argument order: <N> <m> <algo> <nbInstancesMax> <batchSize> <tolerance> <requiredStableBatches> <inputDir> <outputDir>\n";
        return 1;
    }
    
    fs::path inputDir = argv[optind + 9];
    fs::path outputDir = argv[optind + 10];

    
    
    if (!fs::exists(outputDir)) {
        fs::create_directories(outputDir);
    }
    string algo_type;
    if (algo == "o") algo_type = "Order-based";
    else if (algo == "s") algo_type = "Sincronia"; 
    else if (algo == "t") algo_type = "Static";    
    else algo_type = "Githeur";

    string results_filename = outputDir /("Results_N"+to_string(N)+"_m"+to_string(m)+"_"+to_string(start_file)+"_"+to_string(end_file)+".txt");
    
    ofstream out_res(results_filename, ios::out);
    
    // out_res << "Inst N°    Stochastic("<<algo_type<<")       Execution Time\n";
    out_res << "Inst N° \t Avg \t min \t max \t std \t UsedInstances \t nbInstancesMax \t batchSize \t tolerance \t requiredStableBatches\n";
        


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

        fs::path stem = filename.stem();   
        fs::path ext   = filename.extension(); 

        fs::path out_filename = outputDir/ ("exec_" + filename.filename().string());
        // fs::path out_filename = outputDir/ ("exec_" + filename.stem().string() + "_cv"+to_string(cv) + filename.extension().string() );
        

        vector<Job> jobs = readJobsFromFile(filename);

        ofstream out(out_filename);

        streambuf* old_buf = cout.rdbuf(out.rdbuf()); // redirect cout to file

        
        if (gentype == "g")
            cout << "\n******** Jobs(Geo distribution Generation) Description *******" << endl;
        else
            cout << "\n******** Jobs(Discrete distribution Generation) Description *******" << endl;
        
        JobsDescription(jobs);
        
        
        
        cout << "******************** SAA Computing(" <<algo_type <<") ********************"<< endl;


        auto start = chrono::high_resolution_clock::now();

        // SAAResult res = computeAverageDeterministicValue(jobs, nbInstances, algo);

        // SAAResultBatch res = computeAverageDeterministicValuePerBatch(jobs, nbInstancesMax, algo,100,100, 1e-2, 3);
        SAAResultBatch res = computeAverageDeterministicValuePerBatch(jobs, nbInstancesMax, algo, batchSize , minInstances , tolerance , requiredStableBatches );
        
        auto end = chrono::high_resolution_clock::now();

        chrono::duration<double> elapsed = end - start;

        
        // if (debug_) cout<< "********************* " ;
        cout << "\t ====>Average Value Function of Initial State (" <<algo_type<< ") | after "<<res.usedInstances<<"( Stability= "<<requiredStableBatches <<"; Tolerance= "<< tolerance <<" )| = " << res.avg <<endl;
        cout << "\t| Execution time: " << elapsed.count()*1000 << " *10⁻³ seconds |\n";

        // out_res << i << " " << res.avg << "  " << elapsed.count()*1000 << "\n"; // turue
        out_res << i << "\t" << res.avg << "\t" << res.min_val <<"\t" << res.max_val << "\t" << res.std_dev<<"\t" <<res.usedInstances<< "\t" <<nbInstancesMax<< "\t"<<batchSize<<"\t"<<tolerance<<"\t"<<requiredStableBatches<<"\n"; 


       

        cout.rdbuf(old_buf);
    }

    
  
    
    return 0;
}