#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <random>

// --- STABILIZED CORE v3.2 (RICH MEMORY) ---
double relu(double x) { return x > 0 ? x : 0; }
double relu_derivative(double x) { return x > 0 ? 1.0 : 0.0; }
void softmax3(double* input, double* output) {
    double max_val = std::max({input[0], input[1], input[2]});
    double sum = 0.0;
    for(int i=0; i<3; i++) { output[i] = std::exp(input[i] - max_val); sum += output[i]; }
    for(int i=0; i<3; i++) output[i] /= sum;
}

struct SovereignCache {
    double in[3], f1[3], f2[3], inter[3], score[3], alpha[3], y_att;
    double ff_in[19]; // 3 current + 4 slots * 4 dims
    double z1[32], a1[32], y_feat_raw;
    double y_squashed;
};

struct SovereignBlock {
    static const int HEADS = 3, STM_SIZE = 4, DIM_PER_SLOT = 4, HIDDEN = 32, FFN_IN = 3 + STM_SIZE * DIM_PER_SLOT;
    double wA[HEADS][3], wB[HEADS][3], wS[HEADS][3], w1[HIDDEN][FFN_IN], b1[HIDDEN], w_feat[HIDDEN], b_feat;
    double grad_wA[HEADS][3], grad_wB[HEADS][3], grad_wS[HEADS][3], grad_w1[HIDDEN][FFN_IN], grad_b1[HIDDEN], grad_w_feat[HIDDEN], grad_b_feat;

    SovereignBlock() {
        std::mt19937 gen(42); std::uniform_real_distribution<double> dis(-0.05, 0.05);
        for(int k=0; k<HEADS; k++) for(int j=0; j<3; j++) { wA[k][j]=dis(gen); wB[k][j]=dis(gen); wS[k][j]=dis(gen); }
        for(int i=0; i<HIDDEN; i++) { b1[i]=0.01; w_feat[i]=dis(gen); for(int j=0; j<FFN_IN; j++) w1[i][j]=dis(gen); }
        b_feat=0; zero_gradients();
    }
    void zero_gradients() {
        for(int k=0; k<HEADS; k++) for(int j=0; j<3; j++) { grad_wA[k][j]=grad_wB[k][j]=grad_wS[k][j]=0; }
        for(int i=0; i<HIDDEN; i++) { grad_b1[i]=grad_w_feat[i]=0; for(int j=0; j<FFN_IN; j++) grad_w1[i][j]=0; }
        grad_b_feat=0;
    }
    SovereignCache forward(double* in, double stm[4][4], int stm_ptr) {
        SovereignCache c; for(int j=0; j<3; j++) c.in[j]=in[j];
        for(int k=0; k<HEADS; k++) {
            c.f1[k]=c.f2[k]=c.score[k]=0;
            for(int j=0; j<3; j++) { c.f1[k]+=wA[k][j]*in[j]; c.f2[k]+=wB[k][j]*in[j]; c.score[k]+=wS[k][j]*in[j]; }
            c.inter[k]=c.f1[k]*c.f2[k];
        }
        softmax3(c.score, c.alpha); c.y_att=0; for(int k=0; k<HEADS; k++) c.y_att+=c.alpha[k]*c.inter[k];
        
        c.ff_in[0]=in[0]; c.ff_in[1]=in[1]; c.ff_in[2]=c.y_att;
        for(int i=0; i<STM_SIZE; i++) {
            for(int d=0; d<DIM_PER_SLOT; d++) c.ff_in[3 + i*DIM_PER_SLOT + d] = stm[(stm_ptr + i)%STM_SIZE][d];
        }

        for(int i=0; i<HIDDEN; i++) {
            c.z1[i]=b1[i]; for(int j=0; j<FFN_IN; j++) c.z1[i]+=c.ff_in[j]*w1[i][j];
            c.a1[i]=relu(c.z1[i]);
        }
        c.y_feat_raw=b_feat; for(int i=0; i<HIDDEN; i++) c.y_feat_raw+=c.a1[i]*w_feat[i];
        c.y_squashed = c.y_feat_raw / (1.0 + std::abs(c.y_feat_raw)); return c;
    }
    void backward(const SovereignCache& c, double dL_dy_squashed, double* dL_da1_inj, double* dL_din, double* dL_dff_in_out) {
        double dL_dy_raw = dL_dy_squashed / std::pow(1.0 + std::abs(c.y_feat_raw), 2);
        grad_b_feat+=dL_dy_raw; double dL_da1[HIDDEN];
        for(int i=0; i<HIDDEN; i++) { 
            grad_w_feat[i]+=dL_dy_raw*c.a1[i]; 
            dL_da1[i] = dL_dy_raw * w_feat[i];
            if(dL_da1_inj && i < 3) dL_da1[i] += dL_da1_inj[i]; // Inject dL_da from future
        }
        double l_dL_dff_in[FFN_IN]; for(int j=0; j<FFN_IN; j++) l_dL_dff_in[j]=0;
        for(int i=0; i<HIDDEN; i++) {
            double dL_dz1=dL_da1[i]*relu_derivative(c.z1[i]); grad_b1[i]+=dL_dz1;
            for(int j=0; j<FFN_IN; j++) { grad_w1[i][j]+=dL_dz1*c.ff_in[j]; l_dL_dff_in[j]+=dL_dz1*w1[i][j]; }
        }
        if(dL_dff_in_out) for(int j=0; j<FFN_IN; j++) dL_dff_in_out[j]=l_dL_dff_in[j];
        double dL_dy_att=l_dL_dff_in[2]; double dL_din_t[3]={l_dL_dff_in[0],l_dL_dff_in[1],0};
        for(int k=0; k<HEADS; k++) {
            double dL_di_k=dL_dy_att*c.alpha[k], dL_dS_k=dL_dy_att*c.alpha[k]*(c.inter[k]-c.y_att);
            double dL_df1=dL_di_k*c.f2[k], dL_df2=dL_di_k*c.f1[k];
            for(int j=0; j<3; j++) {
                grad_wA[k][j]+=dL_df1*c.in[j]; grad_wB[k][j]+=dL_df2*c.in[j]; grad_wS[k][j]+=dL_dS_k*c.in[j];
                dL_din_t[j]+=dL_df1*wA[k][j]+dL_df2*wB[k][j]+dL_dS_k*wS[k][j];
            }
        }
        if(dL_din) for(int j=0; j<3; j++) dL_din[j]=dL_din_t[j];
    }
    void clip(double thresh) {
        double ns=grad_b_feat*grad_b_feat;
        for(int k=0; k<HEADS; k++) for(int j=0; j<3; j++) ns+=grad_wA[k][j]*grad_wA[k][j]+grad_wB[k][j]*grad_wB[k][j]+grad_wS[k][j]*grad_wS[k][j];
        for(int i=0; i<HIDDEN; i++) { ns+=grad_b1[i]*grad_b1[i]+grad_w_feat[i]*grad_w_feat[i]; for(int j=0; j<FFN_IN; j++) ns+=grad_w1[i][j]*grad_w1[i][j]; }
        double n=std::sqrt(ns); if(n>thresh) {
            double s=thresh/n; grad_b_feat*=s;
            for(int k=0; k<HEADS; k++) for(int j=0; j<3; j++) { grad_wA[k][j]*=s; grad_wB[k][j]*=s; grad_wS[k][j]*=s; }
            for(int i=0; i<HIDDEN; i++) { grad_b1[i]*=s; grad_w_feat[i]*=s; for(int j=0; j<FFN_IN; j++) grad_w1[i][j]*=s; }
        }
    }
    void update(double lr) {
        for(int k=0; k<HEADS; k++) for(int j=0; j<3; j++) { wA[k][j]+=lr*grad_wA[k][j]; wB[k][j]+=lr*grad_wB[k][j]; wS[k][j]+=lr*grad_wS[k][j]; }
        for(int i=0; i<HIDDEN; i++) { b1[i]+=lr*grad_b1[i]; w_feat[i]+=lr*grad_w_feat[i]; for(int j=0; j<FFN_IN; j++) w1[i][j]+=lr*grad_w1[i][j]; }
        b_feat+=lr*grad_b_feat; zero_gradients();
    }
};

int main() {
    SovereignBlock b; double w_f=0.5, b_f=0, g_wf=0, g_bf=0;
    std::mt19937 gen(1337); 
    int DELAY = 4;
    std::cout << "--- SOVEREIGN v3.3 (RICH MEMORY RANDOM STRESS: ECHO-" << DELAY << ") ---\n\n";
    double lr=0.01;
    for(int epoch=0; epoch<5001; epoch++) {
        std::vector<double> seq; for(int i=0; i<100; i++) seq.push_back((gen()%2==0)?0.5:-0.5);
        double tl=0; std::vector<SovereignCache> batch; double stm[4][4]; for(int i=0; i<4; i++) for(int d=0; d<4; d++) stm[i][d]=0;
        int ptr=0;
        
        // Inspection metrics
        double slot_grads[4] = {0,0,0,0};
        int grad_count = 0;
        for(int t=0; t<seq.size(); t++) {
            double in[3]={seq[t],0,0}; SovereignCache c=b.forward(in, stm, ptr); batch.push_back(c);
            stm[ptr][0]=c.y_squashed; stm[ptr][1]=c.a1[0]; stm[ptr][2]=c.a1[1]; stm[ptr][3]=c.a1[2];
            ptr=(ptr+1)%4;
        }
        std::vector<double> dL_dyq_hist(seq.size()+10, 0); 
        std::vector<std::vector<double>> dL_da1_hist(seq.size()+10, std::vector<double>(3, 0.0));
        b.zero_gradients(); g_wf=g_bf=0;
        for(int t=seq.size()-1; t>=10; t--) {
            double target=(t>=DELAY)?seq[t-DELAY]:0.0; double pred=b_f + batch[t].y_squashed*w_f; double err=target-pred;
            tl+=err*err; double dyq = err*w_f + dL_dyq_hist[t]; g_wf+=err*batch[t].y_squashed; g_bf+=err;
            double dL_dff[19]; b.backward(batch[t], dyq, dL_da1_hist[t].data(), nullptr, dL_dff);
            
            // Backprop into past richness
            // Slots 0,1,2,3 map to t-4, t-3, t-2, t-1
            int slots[4] = {t-4, t-3, t-2, t-1};
            for(int s=0; s<4; s++) {
                int past_t = slots[s];
                // Accumulate inspection data
                for(int d=0; d<4; d++) slot_grads[s] += std::abs(dL_dff[3 + s*4 + d]);
                grad_count++;

                if(past_t >= 0) {
                    dL_dyq_hist[past_t] += dL_dff[3 + s*4 + 0];
                    dL_da1_hist[past_t][0] += dL_dff[3 + s*4 + 1];
                    dL_da1_hist[past_t][1] += dL_dff[3 + s*4 + 2];
                    dL_da1_hist[past_t][2] += dL_dff[3 + s*4 + 3];
                }
            }
        }
        b.clip(10.0); b.update(lr); w_f+=lr*g_wf; b_f+=lr*g_bf;
        if (epoch % 1000 == 0) {
            std::cout << "Epoch " << std::setw(5) << epoch << " | MSE: " << tl/(seq.size()-10) << "\n";
            std::cout << "  Gradients (S0-S3): ";
            for(int s=0; s<4; s++) std::cout << "S" << s << ":" << std::fixed << std::setprecision(4) << slot_grads[s]/grad_count << " ";
            std::cout << "\n";
        }
    }
    std::cout << "\n--- FINAL CHECK (RICH RANDOM SEQ) ---\n";
    std::vector<double> f_seq; for(int i=0; i<20; i++) f_seq.push_back((gen()%2==0)?0.5:-0.5);
    double stm_f[4][4]; for(int i=0; i<4; i++) for(int d=0; d<4; d++) stm_f[i][d]=0; int ptr_f=0;
    for(int t=0; t<f_seq.size(); t++) {
        double in[3]={f_seq[t],0,0}; SovereignCache c=b.forward(in, stm_f, ptr_f);
        double p=b_f+c.y_squashed*w_f; double tar=(t>=DELAY)?f_seq[t-DELAY]:0;
        std::cout << "T=" << std::setw(2) << t << " | In: " << std::setw(4) << f_seq[t] << " | Target: " << std::setw(4) << tar << " | Pred: " << std::setw(6) << p << "\n";
        stm_f[ptr_f][0]=c.y_squashed; stm_f[ptr_f][1]=c.a1[0]; stm_f[ptr_f][2]=c.a1[1]; stm_f[ptr_f][3]=c.a1[2];
        ptr_f=(ptr_f+1)%4;
    }
    return 0;
}
