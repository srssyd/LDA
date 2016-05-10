#include<iostream>
#include<string>
#include<cstdio>
#include<cstring>
#include<cstdlib>
#include<cassert>
#include<vector>
#include<map>
#include<sstream>
#include<cmath>
#include<getopt.h>

#define __DEBUG

#ifdef __DEBUG
#define debug(str,...) printf(str,__VA_ARGS__)
#else
#define debug(str,...)
#endif


using namespace std;

class LDASolver{

    int **topic_word_count;
    int **document_topic_count;
    int *sigma_topic_count;

    int topic_count;
    int word_count;
    int document_count;

    vector<int> *words;

    vector<int>* topic_for_word;
    map<int,string> word_str;

    vector<double> alpha;
    vector<double> beta;

    vector<int>* translate_words(vector<string> *words,int n){
        assert(words != NULL);
        vector<int> * result = new vector<int> [n];
        int index = 0;
        map<string,int> word_index;
        for(int i=0;i < n;i++){
            for(int j=0;j<words[i].size();j++){
                result[i].push_back(0);
                string word = words[i][j];
                if(!word_index.count(word)){
                    word_index[word] = index;
                    word_str[index] = word;
                    result[i][j] = index++;
                }else{
                    result[i][j] = word_index[word];
                }
            }
        }
        this->word_count = index;
//        debug("Total words:%d\n",word_count);
        return result;
    }


    void init(){
        for(int i=0;i< topic_count; i++)
            memset(topic_word_count[i],0,word_count * sizeof(int));
        for(int i=0;i< document_count; i++)
            memset(document_topic_count[i],0,topic_count * sizeof(int));
        memset(sigma_topic_count,0,topic_count * sizeof(int));



        for(int i=0;i<document_count;i++){
            topic_for_word[i].clear();
            for(int j=0;j<words[i].size();j++){
                int topic = rand() % topic_count;
                topic_for_word[i].push_back(topic);
                topic_word_count[topic][words[i][j]] ++;
                document_topic_count[i][topic] ++;
                sigma_topic_count[topic] ++;
            }
        }

    }

public:

    LDASolver(int document_count,vector<string> *words,int topic_count){
        this->document_count = document_count;
        this->topic_count = topic_count;
        this->words = translate_words(words,document_count);
        this->topic_word_count = new int *[topic_count];
        this->sigma_topic_count = new int[topic_count];
        this->topic_for_word = new vector<int> [document_count];
        for(int i=0;i < topic_count;i++)
            this->topic_word_count[i] = new int [word_count];
        this->document_topic_count = new int *[document_count];
        for(int i=0;i < document_count; i++)
            this->document_topic_count[i] = new int[topic_count];
    }

    ~LDASolver(){
        delete [] words;
    }

    void solve(double alpha,double beta){

        vector<double> a;
        vector<double> b;
        for(int i=0;i<topic_count;i++)
            a.push_back(alpha);
        for(int i=0;i<word_count;i++)
            b.push_back(beta);
        solve(a,b);
    }


    void solve(vector<double>alpha,vector<double>beta){
        assert(alpha.size() == topic_count);
        assert(beta.size() == word_count);
        this->alpha = alpha;
        this->beta = beta;
        init();
        int iter = 0;
        double last ;
        double new_likelihood = calc_likelihood();
        double improve;
        double eps = 1e-7;
        do{
            last = new_likelihood;
            debug("Iteration %d:%f\n",iter++,last);
            for(int m=0;m<document_count;m++)
                for(int n=0;n<words[m].size();n++){
                    int word = words[m][n];
                    int current_topic = topic_for_word[m][n];
                    document_topic_count[m][current_topic] --;
                    topic_word_count[current_topic][word] --;
                    int new_topic = sample(m,n);
                    //printf("%d:%d pre:%d current:%d\n",m,n,current_topic,new_topic);
                    sigma_topic_count[current_topic] --;
                    topic_for_word[m][n] = new_topic;
                    topic_word_count[new_topic][word] ++;
                    document_topic_count[m][new_topic] ++;
                    sigma_topic_count[new_topic] ++;
                }
            new_likelihood = calc_likelihood();
            improve = (new_likelihood - last);
        }while(improve>= -last * eps );

    }
    void output(){
        /*
        freopen("output.txt","w",stdout);
        for(int i=0;i<document_count;i++){
            for(int j=0;j<words[i].size();j++)
                printf("%s:%d ",word_str[words[i][j]].c_str(),topic_for_word[i][j]);
            printf("\n");
        }
*/

        freopen("topic.txt","w",stdout);
        for(int i=0;i<topic_count;i++){
            printf("Topic %d:\n",i);
            map<double,string> words;
            for(int t=0;t<word_count;t++)
                words[-get_phi(i,t)] = word_str[t];
            int count = 0;
            //Select the top 20 results.
            for(auto iter=words.begin();iter != words.end();iter++){
                printf("%s : %f\n",iter->second.c_str(),-iter->first);
                count++;
                if(count > 20 )
                    break;
            }
        }

    }
private:

    double get_theta(int m,int k){
        return (document_topic_count[m][k] + alpha[k]) / (words[m].size()+alpha[k]);
    }

    double get_phi(int k,int t){
        return (topic_word_count[k][t] + beta[t]) / (sigma_topic_count[k] + beta[t]);
    }

    double calc_likelihood(){
        double result = 0;
        for(int m=0;m<document_count;m++)
            for(int n=0;n<words[m].size();n++){
                double tmp =0;
                for(int k=0;k<topic_count;k++)
                    tmp += get_theta(m,k) * get_phi(k,words[m][n]);
                result += log(tmp);
            }


        return result;
    }

    int sample(int m,int n){

        double value = (double)rand()/(double)RAND_MAX;
        double sum=0;
        int result = -1;
        vector<double> pro;
        for(int i=0;i<topic_count;i++){
             double pr = get_sample_probability(m,words[m][n],i);
             sum += pr;
             pro.push_back(pr);
        }
        for(int i=0;i<topic_count;i++)
            pro[i] /= sum;

        sum = 0;
        do{
            result ++;
            assert(result < topic_count);
            sum += pro[result];
        }while(value >= sum);
        return result;
    }

    double get_sample_probability(int m,int t,int topic){
        //printf("%lf %lf\n",beta[t],alpha[topic]);
        double result = (topic_word_count[topic][t] + beta[t] ) * (document_topic_count[m][topic] + alpha[topic] );
        //printf("result:%lf\n",result);
        result = result / (double)(sigma_topic_count[topic] + beta[t]) ;
        return result;
    }


};


int main(int argc,char **argv){

    struct option long_options[] = {
        {"k", required_argument, 0 ,0},
        {0,0,0,0}
    };

    int topic_count = 3;

    int option_index;
    int c = getopt_long(argc,argv,"k:",long_options,&option_index);

    if(c == -1){
        fprintf(stderr,"You need to input the topic count by -k\n");
        return -1;
    }
    else
        topic_count = atoi(optarg);

    srand(time(0));

    freopen("data.txt","r",stdin);
    int document_count;
    scanf("%d",&document_count);

    vector<string> *documents = new vector<string> [document_count];
    for(int i=0;i<document_count;i++){
        string document;
        getline(cin,document);
        stringstream ss(document);
        string word;
        while(ss>>word)
            documents[i].push_back(word);
    }

    LDASolver *solver = new LDASolver(document_count,documents,topic_count);

    solver->solve(2.0/topic_count,0.01);

    solver->output();
    return 0;
}

