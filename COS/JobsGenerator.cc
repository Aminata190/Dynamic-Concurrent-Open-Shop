#include <math.h>
#include <vector>
#include <random>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <filesystem>
#include "Job.hxx"

using namespace std;
namespace fs = filesystem;

bool debug_ = false;
// readJobsFromFile : see Job.cc



map<int, double> generateRandomDistribution(int nbr_proba,mt19937& gen){
    uniform_real_distribution<> dis(0.1, 1.0);

    vector<double> raw(nbr_proba);
    double total = 0.0;
    for (int i = 0; i < nbr_proba; i++) {
        raw[i] = dis(gen);
        total += raw[i];
    }

    map<int, double> dist;
    int val = 0;
    uniform_int_distribution<> step(1, 7);  // 1–7 added to val each time

    for (int t = 1; t <= nbr_proba; t++) {
        val += step(gen);   // random jump
        dist[val] = raw[t-1] / total; // normalize
    }
    return dist;
}


Task GenerationTask(int t, int d, mt19937& gen){

    int nbr_proba = 1+ rand() % d; // d - max number of proba per jobs
    // uniform_int_distribution<> d(1, 10);  // 1..10
    // int nbr_proba = (d(gen) <= 8) ? 1 : 2;  // 80 % de 1, 20 % de 2
    map<int, double> distribution= generateRandomDistribution(nbr_proba, gen);

    return Task(t, distribution);
}

vector<Job> GenerationJobs(int N, int m, int d, mt19937& gen){

    vector<Job> jobs ;
    for(int j=0; j<N;j++){
        int id = j;
        double w = 1.0;

        Job J= Job(id,w,m);
        for (int t=0; t< m ; t++)
        {
            Task T = GenerationTask(t,d, gen);
            J.setTask(t,T);
        }

        jobs.push_back(J);

    }
    return jobs;
}


map<int, double> generateParametrDistribution(int a, double alpha, mt19937& gen) {

    bernoulli_distribution coin(0.35);
    uniform_int_distribution<> dis_a(1, a);

    // if (coin(gen) == 0){
    //     // cout << "d " << a+1 ;
    //     return {{dis_a(gen),1.0}};
    // }      
    // else{       
    //     // cout << "s " ;
    //     // uniform_int_distribution<> step = (a < 100)
    //     //     ? uniform_int_distribution<>(100, 200)
    //     //     : uniform_int_distribution<>(a + 1, 2 * a);

    //     uniform_int_distribution<> step = (a < 7)
    //         ? uniform_int_distribution<>(90, 100)
    //         : uniform_int_distribution<>(a + 1, 2 * a);

    //     uniform_real_distribution<> dis_alpha(0.7, alpha);
    //     double proba = dis_alpha(gen);


    //     map<int, double> dist;
    //     dist[dis_a(gen)] = proba;
    //     dist[step(gen)]  = 1.0 - proba;
    //     return dist;
    // }
    uniform_int_distribution<> step = (a < 7)
            ? uniform_int_distribution<>(90, 100)
            : uniform_int_distribution<>(a + 1, 2 * a);

    uniform_real_distribution<> dis_alpha(0.8, alpha);
    double proba = dis_alpha(gen);


    map<int, double> dist;
    dist[dis_a(gen)] = proba;
    dist[step(gen)]  = 1.0 - proba;
    return dist;

    
}

double computeCV(const map<int, double>& dist) {
    double mean = 0.0;

    // Compute mean
    for (const auto& [x, p] : dist) {
        mean += x * p;
    }

    // Compute variance
    double variance = 0.0;
    for (const auto& [x, p] : dist) {
        variance += p * (x - mean) * (x - mean);
    }

    double stddev = sqrt(variance);
    double cv = stddev / mean;
    if (debug_ ) cout << "Mean: " << mean <<"- standard deviation: " << stddev << "- cv: " << cv << endl; 
    return cv;
}


double computeXb(double a, double alpha, double cv) {
    double p = alpha;
    double q = 1.0 - alpha;
    double sqrtpq = sqrt(p * q);

    double denom = sqrtpq - cv * q;
    if (denom <= 1e-9) {
        throw runtime_error(
            "Infeasible: cv=" + to_string(cv) +
            " requires alpha closer to 1 (cv must be < sqrt(p/q) = " +
            to_string(sqrtpq / q) + ")"
        );
    }

    double b = a * (cv * p + sqrtpq) / denom;
    return b;
}



map<int, double> generateParametrDistributionFromCV(int a, double alpha, double cv, mt19937& gen) {
    
    uniform_int_distribution<> dis_a(1, a);
    int x_a = dis_a(gen);

    // p minimum requis pour que ce cv soit atteignable
    double p_min = (cv * cv) / (1.0 + cv * cv);

    // petite marge de sécurité pour éviter denom ~ 0 (b -> infini)
    double p_floor = p_min + 1e-4;

    // borne haute : ne jamais dépasser 0.99 (sinon x_b explose numériquement)
    double p_high = max(p_floor + 1e-4, min(alpha, 0.99));
    if (p_high <= p_floor) p_high = p_floor + 1e-3; // garde-fou

    uniform_real_distribution<> dis_alpha(p_floor, p_high);
    double p = dis_alpha(gen);

    double b = computeXb((double)x_a, p, cv);
    int x_b = (int) llround(b);
    
    map<int, double> dist;
    dist[x_a] = p;
    dist[x_b] = 1.0 - p;
    
    if (debug_ ) cout << "cv= " << cv <<"- x_a= " << x_a << "(" << dist[x_a] << ")"<< "- x_b= " << x_b << "(" << dist[x_b] << ")"<< endl;
    
    return dist;
}


vector<Job> GenerationJobsParameterized(int N, int m, int a, double alpha, mt19937& gen, double cv=0.0){
    vector<Job> jobs ;
    for(int j=0; j<N;j++){
        int id = j;
        double w = 1.0;

        Job J= Job(id,w,m);
        for (int t=0; t< m ; t++)
        {
            map<int, double> dist;
            if (cv == 0.0){
                dist = generateParametrDistribution(a,alpha,gen);
            }
            else {
                dist = generateParametrDistributionFromCV(a,alpha,cv, gen);
            }
            
            Task T = Task(t, dist);
          
            J.setTask(t,T);
        }

        jobs.push_back(J);

    }
    return jobs;
}





// --- Geometric generation -------------------------------------------------
// alpha ~ U[alphaMin, alphaMax] ; E[P] = 1/alpha ; CV = sqrt(1-alpha) < 1
vector<Job> GenerationJobsGeometric(int N, int m, double alphaMin, double alphaMax, mt19937& gen){
    uniform_real_distribution<> dis_alpha(alphaMin, alphaMax);

    vector<Job> jobs ;
    for(int j=0; j<N;j++){
        int id = j;
        double w = 1.0;

        Job J= Job(id,w,m);
        for (int t=0; t< m ; t++)
        {
            Task T = Task(t, dis_alpha(gen));
            J.setTask(t,T);
        }

        jobs.push_back(J);
    }
    return jobs;
}



int main(int argc, char **argv) {

    if (argc < 7) {
        cerr << "Usage: " << argv[0] << " <N> <m> <d> <INST_ID> <CV> <INPUTDIR> [type]\n"
             << "\t type : d = discrete (default), g = geometric (CV not used)\n";
        return 1;
    }

    int         N = atoi(argv[1]);
    int         m = atoi(argv[2]);
    int         d = atoi(argv[3]);
    int         inst = atoi(argv[4]);
    double      cv = atof(argv[5]);
    string      input_dir= argv[6];
    string      gentype = (argc > 7) ? argv[7] : "d";   // d : discrete, g : geometric
    bool        isCV= false;
    bool        isParamGeneration=true;        
    bool        isJobsGenerated=false;

    fs::create_directories(input_dir);
    string filename = input_dir +"N"+to_string(N)+"_m"+to_string(m)+"_"+to_string(inst)+".txt";

    random_device rd;
    mt19937 gen(rd());

    
    if(!isJobsGenerated){
        vector<Job> jobs;

        if (gentype == "g"){
            cout<<" Geometric generation " << endl;
            if (cv != 0.0)
                cerr << "Warning: CV=" << cv << " not used for geometric generation (CV = sqrt(1-alpha) < 1)" << endl;
            double alphaMin = 0.1;
            double alphaMax = 0.9;
            jobs = GenerationJobsGeometric(N,m,alphaMin,alphaMax, gen);
        }
        else if (isParamGeneration){
            cout<<" Parameterize generation " << endl;
            int a = 10;
            double alpha = 0.9;
            jobs = GenerationJobsParameterized(N,m,a,alpha, gen, cv);
        }
        else {
            // Generation 
            jobs = GenerationJobs(N,m,d, gen);
        }
        

        


        ofstream outfile(filename);
        if (!outfile) {
            cerr << "Error: cannot open " << filename << endl;
            return 1;
        }

        outfile << "# id weight nbr_Task id_task  distribution(vi:pi ... or alpha)\n";

        
        for (auto &J : jobs) {
            outfile << J.getId() << " "
                    << int(J.getWeight()) << " "
                    << J.getNbTask();

            bool first = true;
            m = J.getNbTask();
            for (int t=0; t<m ; t++){
                auto &T = J.getTask(t);

                outfile << "  "<<T.getId() << " ";
                if (T.isGeometric()) {
                    outfile << fixed << setprecision(3) << T.getAlpha();
                    continue;
                }
                for (auto &p : T.getDistribution()) {
                    if (!first) outfile << " ";
                    if (cv != 0.0){
                        outfile << p.first << ":" << fixed << setprecision(6) << p.second;
                    }
                    else {
                        outfile << p.first << ":" <<fixed <<setprecision(2)<< p.second;
                    }
                    first = false;
                }

            }

            outfile << "\n";
        }

        outfile.close();
    }
    

    // isCV= true;
    if (isCV){
        vector<Job> jobs = readJobsFromFile(filename);



        cout << "  CV calculation " << endl;
        string input_dir_cv = input_dir+ "cv/";
        fs::create_directories(input_dir_cv);
        string filename_cv = input_dir_cv + "N" + to_string(N) + "_m" + to_string(m) + "_" + to_string(inst) + "_cv.txt";
        if (debug_ ) cout << filename_cv << endl;
        ofstream outfile_cv(filename_cv);
        if (!outfile_cv) {
            cerr << "Error: cannot open " << filename_cv << endl;
            return 1;
        }

        outfile_cv << "# id weight nbr_Task id_task CV\n";
        for (auto &J : jobs) {
            outfile_cv << J.getId() << " "
                    << int(J.getWeight()) << " "
                    << J.getNbTask();

            bool first = true;
            m = J.getNbTask();
            for (int t=0; t<m ; t++){
                auto &T = J.getTask(t);
                if (debug_ ) cout << "Task " <<T.getId() << endl;
                double cv = T.isGeometric() ? T.getCV() : computeCV(T.getDistribution());
                outfile_cv << "  " << T.getId() << " "
                        << fixed << setprecision(4) << cv;
            }

            outfile_cv << "\n";
        }

        outfile_cv.close();
    }
    

    return 0;
}
