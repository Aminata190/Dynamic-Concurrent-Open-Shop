//****************************************************************//
//*                                                              *//
//*------------------- Includes SECTION -------------------------*//
//*                                                              *//
//****************************************************************//


#include "Job.hxx"

#include <fstream>
#include <sstream>
#include <cstdlib>


// Stop with a clear message when a DISCRETE-only function is called on a GEOMETRIC task
static void requireDiscrete(const Task& t, const string& function)
{
    if (t.isGeometric()) {
        cerr << "Error: Task::" << function << " is not defined for a GEOMETRIC task (task "
             << t.getId() << ", alpha=" << t.getAlpha() << ")" << endl;
        exit(1);
    }
}



//----------------------------------------------------------------//
// Private Function:   ostream << Task                            //
//----------------------------------------------------------------//


ostream &operator<<(ostream &stream, Task const &t)
{
   stream << "Task " << t.id_ << " :\n";
    if (t.isGeometric()) {
        stream << "   geometric alpha = " << t.alpha_ << "\n";
        return stream;
    }
    for (const auto &d : t.distribution) {
        stream << "   process time = " << d.first 
               << " prob = " << d.second << "\n";
    }
    return stream;
}



//-------------------------------------------------------------//
// operator>> : read a Task                                    //
//   Expected format:                                          //
//   id  (time prob)*                                 //
//-------------------------------------------------------------//

istream &operator>>(istream &is, Task &t) {
    int n;
    t.distribution.clear();
    is >> t.id_>>n;   // read id , number of realisation
    
    for (int i = 0; i < n; i++) {
        int time; double prob;
        is >> time >> prob;
        t.distribution[time] = prob;
    }
    return is;
}


//----------------------------------------------------------------//
// Private Function:   ostream << Job                          //
//----------------------------------------------------------------//


ostream &operator<<(ostream &stream, Job const &J)
{
  stream << "Job " << J.id_ 
	 << " : weight = " << J.weight
	 << ", number of task="
	 << J.nbTask_;

  return stream;
}


//----------------------------------------------------------------//
// Private Function:   istream >> Job                          //
//----------------------------------------------------------------//

istream &operator>>(istream &stream, Job &x)
{
  return
    (
     stream >> x.id_ 
     >> x.weight
     >> x.nbTask_
     );
}




//Total Probability remaining
double Task::getTotalProba(int y)
{
    if (isGeometric()) return pow(1.0 - alpha_, y); // Pr(P > y)

    double totalProb = 0.0;

    //total probability that Pj > y
    for (auto &p : distribution) {
        int processing_time = p.first;   // key
        double prob = p.second;          // value
        if (processing_time > y) {
            totalProb += prob;
        }
    }

    return totalProb;
}

double Task::getTotalProba(int y) const{
    if (isGeometric()) return pow(1.0 - alpha_, y); // Pr(P > y)

    double totalProb = 0.0;

    //total probability that Pj > y
    for (auto &p : distribution) {
        int processing_time = p.first;   // key
        double prob = p.second;          // value
        if (processing_time > y) {
            totalProb += prob;
        }
    }

    return totalProb;
}
// Fonction qui calcule les temps restants et leurs probas    
vector<pair<int,double>> Task::getRemaining(int y) {

    requireDiscrete(*this, "getRemaining"); // infinite support

    double totalProba = getTotalProba(y);

    vector<pair<int,double>> result;

    if (totalProba == 0.0) return result; // No job remaining

    for (auto &p : distribution) {
        int processing_time = p.first;
        double prob = p.second;

        if (processing_time > y){
            int remaining = processing_time - y;
            result.push_back({remaining, prob/totalProba});
        }
        
    }
    return result;
}

vector<pair<int,double>> Task::getRemaining(int y) const{

    requireDiscrete(*this, "getRemaining"); // infinite support

    double totalProba = getTotalProba(y);

    vector<pair<int,double>> result;

    if (totalProba == 0.0) return result; // No job remaining

    for (auto &p : distribution) {
        int processing_time = p.first;
        double prob = p.second;

        if (processing_time > y){
            int remaining = processing_time - y;
            result.push_back({remaining, prob/totalProba});
        }
        
    }
    return result;
}


void Task::advance(int q) {
        
    requireDiscrete(*this, "advance");

    int max_time = distribution.rbegin()->first; //  largest processing time
    if (progress + q >= max_time) {  
        progress = max_time;      
        completed = true;  
    }
    else progress +=q;
    
}
void Task::advance(int q, int current_time) {
    
    requireDiscrete(*this, "advance");

    int max_time = distribution.rbegin()->first; //  largest processing time

    int remaining = max_time - progress;
    int used = min(q, remaining);
    progress += used;
    current_time += used;
    if (progress >= max_time) {
        completed = true;
        cct_ = current_time;
    }

}



int Task::getMaxProcTime() const {
    requireDiscrete(*this, "getMaxProcTime"); // infinite support
    return distribution.rbegin()->first;
}



//----------------------------------------------------------------//
//      Distribution (DISCRETE and GEOMETRIC)                     //
//----------------------------------------------------------------//

// E[P - y | P > y]
double Task::expectedRemaining(int y) const {
    if (isGeometric()) return 1.0 / alpha_;  // memoryless

    double expected = 0.0;
    for (auto &r : getRemaining(y)) {
        if (r.first > 0) expected += r.first * r.second;
    }
    return expected;
}


// smallest remaining processing time
int Task::minQuantum(int y) const {
    if (isGeometric()) return 1;  // memoryless: decision at each time unit

    return getRemaining(y).begin()->first;
}


// one realization of P
int Task::sample(mt19937& rng) const {
    if (isGeometric()) {
        geometric_distribution<int> dist(alpha_);  // number of failures before success
        return dist(rng) + 1;
    }

    vector<int>    values;
    vector<double> probs;
    for (const auto& [val, prob] : distribution) {
        values.push_back(val);
        probs.push_back(prob);
    }
    discrete_distribution<int> dist(probs.begin(), probs.end());
    return values[dist(rng)];
}


// coefficient of variation of P
double Task::getCV() const {
    if (isGeometric()) return sqrt(1.0 - alpha_);

    double mean = 0.0;
    for (const auto& [x, p] : distribution) mean += x * p;

    double variance = 0.0;
    for (const auto& [x, p] : distribution) variance += p * (x - mean) * (x - mean);

    return sqrt(variance) / mean;
}



//------------------------------------//
//      Reading from file             //
//------------------------------------//
vector<Job> readJobsFromFile(const string &filename) {
    vector<Job> jobs;
    ifstream infile(filename);

    if (!infile) {
        cerr << "Error: cannot open " << filename << endl;
        return jobs;
    }

    bool typeKnown = false;       // type of the first task read
    bool instanceGeometric = false;

    string line;
    int line_number = 0;
    while (getline(infile, line)) {
        line_number++;
        if (line.empty() || line[0] == '#') continue; // skip comments/empty

        istringstream iss(line);
        int id,nbr_task;
        double weight;

        iss >> id >> weight >> nbr_task;
        Job J(id,weight, nbr_task);

        for (int t=0; t< nbr_task; t++){
            int id_task;
            iss >> id_task;

            
            map<int,double> dist;
            double alpha = -1.0;   // >= 0 : geometric task

            string token;
            while (iss >> token) {
                size_t pos = token.find(':');
                if (pos == string::npos) {
                    // Geometric task written without prefix ("id_task alpha"):
                    // first token of the task, contains a '.' (alpha is a real number)
                    if (dist.empty() && alpha < 0.0 && token.find('.') != string::npos) {
                        alpha = stod(token);
                        break;  // a geometric task has a single value
                    }
                    // not a distribution, push token back and break
                    iss.seekg(-(int)token.size(), ios::cur);
                    break;
                }
                if (token.substr(0,pos) == "g") {
                    alpha = stod(token.substr(pos+1));
                    continue;
                }
                int vi = stoi(token.substr(0,pos));
                double pi = stod(token.substr(pos+1));
                dist[vi] = pi;
            }

            bool taskGeometric = (alpha >= 0.0);

            if (!taskGeometric && dist.empty()) {
                cerr << "Error: " << filename << " line " << line_number << ": task " << id_task
                     << " of job " << id << " has no value (expected v:p ... , g:alpha or alpha)" << endl;
                exit(1);
            }

            if (taskGeometric && !dist.empty()) {
                cerr << "Error: " << filename << " line " << line_number << ": task " << id_task
                     << " of job " << id << " mixes g:alpha and v:p values" << endl;
                exit(1);
            }
            if (taskGeometric && (alpha <= 0.0 || alpha > 1.0)) {
                cerr << "Error: " << filename << " line " << line_number << ": task " << id_task
                     << " of job " << id << " has alpha=" << alpha << " (must be in ]0,1])" << endl;
                exit(1);
            }
            if (!typeKnown) {
                typeKnown = true;
                instanceGeometric = taskGeometric;
            }
            else if (taskGeometric != instanceGeometric) {
                cerr << "Error: " << filename << " line " << line_number << ": task " << id_task
                     << " of job " << id << " is " << (taskGeometric ? "GEOMETRIC" : "DISCRETE")
                     << " but the instance is " << (instanceGeometric ? "GEOMETRIC" : "DISCRETE")
                     << " (all tasks must have the same type)" << endl;
                exit(1);
            }

            if (taskGeometric) {
                Task T(id_task, alpha);
                J.setTask(t,T);
            }
            else {
                Task T(id_task,dist);
                J.setTask(t,T);
            }
        }
        jobs.push_back(J);
    }

    return jobs;
}


// readJobsFromFile ensures that all tasks have the same type
bool isGeometricInstance(const vector<Job>& jobs) {
    return !jobs.empty() && jobs[0].getNbTask() > 0 && jobs[0].getTask(0).isGeometric();
}
