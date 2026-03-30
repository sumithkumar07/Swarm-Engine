#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <random>

// --- SYSTEM UTILS (DOUBLE PRECISION) ---
double relu(double x) { return x > 0 ? x : 0; }
double relu_derivative(double x) { return x > 0 ? 1.0 : 0.0; }
void softmax3(double* input, double* output) {
    double max_val = std::max({input[0], input[1], input[2]});
    double sum = 0.0;
    for(int i=0; i<3; i++) { output[i] = std::exp(input[i] - max_val); sum += output[i]; }
    for(int i=0; i<3; i++) output[i] /= sum;
}
void softmax_n(double* input, double* output, int n) {
    double max_val = input[0];
    for(int i=1; i<n; i++) if(input[i]>max_val) max_val=input[i];
    double sum = 0.0;
    for(int i=0; i<n; i++) { output[i] = std::exp(input[i] - max_val); sum += output[i]; }
    for(int i=0; i<n; i++) output[i] /= sum;
}

// --- CACHE FOR BPTT ---
static const int KEY_DIM = 4;
static const int STM_SIZE = 16;
static const int DIM_PER_SLOT = 4;
static const int HIDDEN = 48;
static const int HEADS = 3;
static const int FFN_IN = 3 + DIM_PER_SLOT; // 7: [x, 0, y_att, read[4]]

struct SovereignCache {
    double in[3], f1[3], f2[3], inter[3], score[3], alpha_head[3], y_att;
    // Attention read cache
    double query[KEY_DIM], keys[STM_SIZE][KEY_DIM];
    double mem_scores[STM_SIZE], mem_alpha[STM_SIZE];
    double read_vec[DIM_PER_SLOT];
    double stm_snapshot[STM_SIZE][DIM_PER_SLOT]; // copy of memory at this timestep
    // FFN cache
    double ff_in[FFN_IN];
    double z1[HIDDEN], a1[HIDDEN], y_feat_raw, y_squashed;
};

struct SovereignBlock {
    // Input Attention (existing)
    double wA[HEADS][3], wB[HEADS][3], wS[HEADS][3];
    // Memory Read Head (NEW — the ONE feature)
    double W_query[KEY_DIM][3];   // 3 → KEY_DIM  (12 params)
    double W_key[KEY_DIM][DIM_PER_SLOT]; // DIM → KEY_DIM (16 params)
    // FFN
    double w1[HIDDEN][FFN_IN], b1[HIDDEN], w_feat[HIDDEN], b_feat;

    // --- Gradient Accumulators ---
    double grad_wA[HEADS][3], grad_wB[HEADS][3], grad_wS[HEADS][3];
    double grad_W_query[KEY_DIM][3], grad_W_key[KEY_DIM][DIM_PER_SLOT];
    double grad_w1[HIDDEN][FFN_IN], grad_b1[HIDDEN], grad_w_feat[HIDDEN], grad_b_feat;

    SovereignBlock() {
        std::mt19937 gen(42); std::uniform_real_distribution<double> dis(-0.05, 0.05);
        for(int k=0; k<HEADS; k++) for(int j=0; j<3; j++) { wA[k][j]=dis(gen); wB[k][j]=dis(gen); wS[k][j]=dis(gen); }
        for(int d=0; d<KEY_DIM; d++) { for(int j=0; j<3; j++) W_query[d][j]=dis(gen); for(int j=0; j<DIM_PER_SLOT; j++) W_key[d][j]=dis(gen); }
        for(int i=0; i<HIDDEN; i++) { b1[i]=0.01; w_feat[i]=dis(gen); for(int j=0; j<FFN_IN; j++) w1[i][j]=dis(gen); }
        b_feat=0; zero_gradients();
    }

    void zero_gradients() {
        for(int k=0; k<HEADS; k++) for(int j=0; j<3; j++) { grad_wA[k][j]=grad_wB[k][j]=grad_wS[k][j]=0; }
        for(int d=0; d<KEY_DIM; d++) { for(int j=0; j<3; j++) grad_W_query[d][j]=0; for(int j=0; j<DIM_PER_SLOT; j++) grad_W_key[d][j]=0; }
        for(int i=0; i<HIDDEN; i++) { grad_b1[i]=grad_w_feat[i]=0; for(int j=0; j<FFN_IN; j++) grad_w1[i][j]=0; }
        grad_b_feat=0;
    }

    SovereignCache forward(double* in, double stm[][DIM_PER_SLOT], int stm_ptr) {
        SovereignCache c;
        for(int j=0; j<3; j++) c.in[j]=in[j];

        // 1. Input Attention (unchanged from v3)
        for(int k=0; k<HEADS; k++) {
            c.f1[k]=c.f2[k]=c.score[k]=0;
            for(int j=0; j<3; j++) { c.f1[k]+=wA[k][j]*in[j]; c.f2[k]+=wB[k][j]*in[j]; c.score[k]+=wS[k][j]*in[j]; }
            c.inter[k]=c.f1[k]*c.f2[k];
        }
        softmax3(c.score, c.alpha_head); c.y_att=0; for(int k=0; k<HEADS; k++) c.y_att+=c.alpha_head[k]*c.inter[k];

        // 2. Memory Read Head (THE NEW FEATURE)
        // 2a. Query from current state
        double q_in[3] = {in[0], in[1], c.y_att};
        for(int d=0; d<KEY_DIM; d++) { c.query[d]=0; for(int j=0; j<3; j++) c.query[d]+=W_query[d][j]*q_in[j]; }
        // 2b. Keys from each memory slot + snapshot
        for(int s=0; s<STM_SIZE; s++) {
            int idx = (stm_ptr + s) % STM_SIZE;
            for(int d=0; d<DIM_PER_SLOT; d++) c.stm_snapshot[s][d] = stm[idx][d];
            for(int d=0; d<KEY_DIM; d++) { c.keys[s][d]=0; for(int j=0; j<DIM_PER_SLOT; j++) c.keys[s][d]+=W_key[d][j]*c.stm_snapshot[s][j]; }
        }
        // 2c. Scores + Softmax
        double scale = 1.0 / std::sqrt((double)KEY_DIM);
        for(int s=0; s<STM_SIZE; s++) { c.mem_scores[s]=0; for(int d=0; d<KEY_DIM; d++) c.mem_scores[s]+=c.query[d]*c.keys[s][d]; c.mem_scores[s]*=scale; }
        softmax_n(c.mem_scores, c.mem_alpha, STM_SIZE);
        // 2d. Weighted Read
        for(int d=0; d<DIM_PER_SLOT; d++) { c.read_vec[d]=0; for(int s=0; s<STM_SIZE; s++) c.read_vec[d]+=c.mem_alpha[s]*c.stm_snapshot[s][d]; }

        // 3. FFN (smaller input: 7 instead of 19)
        c.ff_in[0]=in[0]; c.ff_in[1]=in[1]; c.ff_in[2]=c.y_att;
        for(int d=0; d<DIM_PER_SLOT; d++) c.ff_in[3+d]=c.read_vec[d];

        for(int i=0; i<HIDDEN; i++) {
            c.z1[i]=b1[i]; for(int j=0; j<FFN_IN; j++) c.z1[i]+=c.ff_in[j]*w1[i][j];
            c.a1[i]=relu(c.z1[i]);
        }
        c.y_feat_raw=b_feat; for(int i=0; i<HIDDEN; i++) c.y_feat_raw+=c.a1[i]*w_feat[i];
        c.y_squashed = c.y_feat_raw / (1.0 + std::abs(c.y_feat_raw));
        return c;
    }

    // Backward: returns dL/d(stm_snapshot) for BPTT
    void backward(const SovereignCache& c, double dL_dy_squashed, double* dL_da1_inj,
                  double dL_dstm_out[STM_SIZE][DIM_PER_SLOT]) {
        // --- FFN backward (same as v3) ---
        double dL_dy_raw = dL_dy_squashed / std::pow(1.0 + std::abs(c.y_feat_raw), 2);
        grad_b_feat += dL_dy_raw;
        double dL_da1[HIDDEN];
        for(int i=0; i<HIDDEN; i++) {
            grad_w_feat[i] += dL_dy_raw * c.a1[i];
            dL_da1[i] = dL_dy_raw * w_feat[i];
            if(dL_da1_inj && i < 3) dL_da1[i] += dL_da1_inj[i];
        }
        double dL_dff_in[FFN_IN]; for(int j=0; j<FFN_IN; j++) dL_dff_in[j]=0;
        for(int i=0; i<HIDDEN; i++) {
            double dL_dz1 = dL_da1[i] * relu_derivative(c.z1[i]);
            grad_b1[i] += dL_dz1;
            for(int j=0; j<FFN_IN; j++) { grad_w1[i][j]+=dL_dz1*c.ff_in[j]; dL_dff_in[j]+=dL_dz1*w1[i][j]; }
        }

        // --- Memory Read Head backward (THE NEW BACKPROP) ---
        // dL/d(read_vec) from FFN inputs [3..6]
        double dL_dread[DIM_PER_SLOT];
        for(int d=0; d<DIM_PER_SLOT; d++) dL_dread[d] = dL_dff_in[3+d];

        // dL/d(mem_alpha[s]) = dot(dL_dread, stm_snapshot[s])
        double dL_dalpha[STM_SIZE];
        for(int s=0; s<STM_SIZE; s++) { dL_dalpha[s]=0; for(int d=0; d<DIM_PER_SLOT; d++) dL_dalpha[s]+=dL_dread[d]*c.stm_snapshot[s][d]; }

        // dL/d(stm_snapshot[s]) through VALUE path: alpha[s] * dL_dread
        for(int s=0; s<STM_SIZE; s++) for(int d=0; d<DIM_PER_SLOT; d++) dL_dstm_out[s][d] = c.mem_alpha[s] * dL_dread[d];

        // dL/d(mem_scores[s]) via softmax jacobian
        double dot_alpha_dalpha = 0;
        for(int s=0; s<STM_SIZE; s++) dot_alpha_dalpha += c.mem_alpha[s] * dL_dalpha[s];
        double dL_dscore[STM_SIZE];
        double scale = 1.0 / std::sqrt((double)KEY_DIM);
        for(int s=0; s<STM_SIZE; s++) dL_dscore[s] = c.mem_alpha[s] * (dL_dalpha[s] - dot_alpha_dalpha) * scale;

        // dL/d(query) and dL/d(keys)
        double dL_dquery[KEY_DIM]; for(int d=0; d<KEY_DIM; d++) dL_dquery[d]=0;
        double dL_dkeys[STM_SIZE][KEY_DIM];
        for(int s=0; s<STM_SIZE; s++) {
            for(int d=0; d<KEY_DIM; d++) {
                dL_dquery[d] += dL_dscore[s] * c.keys[s][d];
                dL_dkeys[s][d] = dL_dscore[s] * c.query[d];
            }
        }

        // Accumulate W_query gradients
        double q_in[3] = {c.in[0], c.in[1], c.y_att};
        for(int d=0; d<KEY_DIM; d++) for(int j=0; j<3; j++) grad_W_query[d][j] += dL_dquery[d] * q_in[j];

        // Accumulate W_key gradients + dL/d(stm_snapshot) through KEY path
        for(int s=0; s<STM_SIZE; s++) {
            for(int d=0; d<KEY_DIM; d++) {
                for(int j=0; j<DIM_PER_SLOT; j++) {
                    grad_W_key[d][j] += dL_dkeys[s][d] * c.stm_snapshot[s][j];
                    dL_dstm_out[s][j] += dL_dkeys[s][d] * W_key[d][j]; // KEY path gradient
                }
            }
        }

        // --- Input Attention backward (unchanged from v3) ---
        double dL_dy_att = dL_dff_in[2];
        // Also from query: dL/d(q_in[2]) = dL/d(y_att) through query path
        for(int d=0; d<KEY_DIM; d++) dL_dy_att += dL_dquery[d] * W_query[d][2];

        for(int k=0; k<HEADS; k++) {
            double dL_di_k = dL_dy_att * c.alpha_head[k];
            double dL_dS_k = dL_dy_att * c.alpha_head[k] * (c.inter[k] - c.y_att);
            double dL_df1 = dL_di_k * c.f2[k], dL_df2 = dL_di_k * c.f1[k];
            for(int j=0; j<3; j++) {
                grad_wA[k][j] += dL_df1 * c.in[j];
                grad_wB[k][j] += dL_df2 * c.in[j];
                grad_wS[k][j] += dL_dS_k * c.in[j];
            }
        }
    }

    void clip(double thresh) {
        double ns = grad_b_feat*grad_b_feat;
        for(int k=0; k<HEADS; k++) for(int j=0; j<3; j++) ns+=grad_wA[k][j]*grad_wA[k][j]+grad_wB[k][j]*grad_wB[k][j]+grad_wS[k][j]*grad_wS[k][j];
        for(int d=0; d<KEY_DIM; d++) { for(int j=0; j<3; j++) ns+=grad_W_query[d][j]*grad_W_query[d][j]; for(int j=0; j<DIM_PER_SLOT; j++) ns+=grad_W_key[d][j]*grad_W_key[d][j]; }
        for(int i=0; i<HIDDEN; i++) { ns+=grad_b1[i]*grad_b1[i]+grad_w_feat[i]*grad_w_feat[i]; for(int j=0; j<FFN_IN; j++) ns+=grad_w1[i][j]*grad_w1[i][j]; }
        double n=std::sqrt(ns);
        if(n>thresh) {
            double s=thresh/n; grad_b_feat*=s;
            for(int k=0; k<HEADS; k++) for(int j=0; j<3; j++) { grad_wA[k][j]*=s; grad_wB[k][j]*=s; grad_wS[k][j]*=s; }
            for(int d=0; d<KEY_DIM; d++) { for(int j=0; j<3; j++) grad_W_query[d][j]*=s; for(int j=0; j<DIM_PER_SLOT; j++) grad_W_key[d][j]*=s; }
            for(int i=0; i<HIDDEN; i++) { grad_b1[i]*=s; grad_w_feat[i]*=s; for(int j=0; j<FFN_IN; j++) grad_w1[i][j]*=s; }
        }
    }

    void update(double lr) {
        for(int k=0; k<HEADS; k++) for(int j=0; j<3; j++) { wA[k][j]+=lr*grad_wA[k][j]; wB[k][j]+=lr*grad_wB[k][j]; wS[k][j]+=lr*grad_wS[k][j]; }
        for(int d=0; d<KEY_DIM; d++) { for(int j=0; j<3; j++) W_query[d][j]+=lr*grad_W_query[d][j]; for(int j=0; j<DIM_PER_SLOT; j++) W_key[d][j]+=lr*grad_W_key[d][j]; }
        for(int i=0; i<HIDDEN; i++) { b1[i]+=lr*grad_b1[i]; w_feat[i]+=lr*grad_w_feat[i]; for(int j=0; j<FFN_IN; j++) w1[i][j]+=lr*grad_w1[i][j]; }
        b_feat+=lr*grad_b_feat; zero_gradients();
    }

    int count_params() {
        int p = 0;
        p += HEADS*3*3; // wA, wB, wS
        p += KEY_DIM*3 + KEY_DIM*DIM_PER_SLOT; // W_query, W_key
        p += HIDDEN*FFN_IN + HIDDEN + HIDDEN + 1; // w1, b1, w_feat, b_feat
        return p;
    }
};

int main() {
    SovereignBlock b;
    double w_f=0.5, b_f=0, g_wf=0, g_bf=0;
    std::mt19937 gen(1337);
    int DELAY = 12;

    std::cout << "--- SOVEREIGN v4.0 (SPARSE READ ATTENTION: ECHO-" << DELAY << ") ---\n";
    std::cout << "Parameters: " << b.count_params() + 2 << " (block) + 2 (final layer)\n";
    std::cout << "STM Slots: " << STM_SIZE << " | FFN Input: " << FFN_IN << "\n\n";

    double lr = 0.01;
    for(int epoch=0; epoch<10001; epoch++) {
        std::vector<double> seq; for(int i=0; i<100; i++) seq.push_back((gen()%2==0)?0.5:-0.5);
        double tl=0; std::vector<SovereignCache> batch;
        double stm[STM_SIZE][DIM_PER_SLOT]; for(int i=0; i<STM_SIZE; i++) for(int d=0; d<DIM_PER_SLOT; d++) stm[i][d]=0;
        int ptr=0;

        for(int t=0; t<(int)seq.size(); t++) {
            double in[3]={seq[t],0,0};
            SovereignCache c = b.forward(in, stm, ptr);
            batch.push_back(c);
            stm[ptr][0]=c.y_squashed; stm[ptr][1]=c.a1[0]; stm[ptr][2]=c.a1[1]; stm[ptr][3]=c.a1[2];
            ptr=(ptr+1)%STM_SIZE;
        }

        std::vector<double> dL_dyq_hist(seq.size()+10, 0);
        std::vector<std::vector<double>> dL_da1_hist(seq.size()+10, std::vector<double>(3, 0.0));
        b.zero_gradients(); g_wf=g_bf=0;

        for(int t=seq.size()-1; t>=10; t--) {
            double target=(t>=DELAY)?seq[t-DELAY]:0.0;
            double pred=b_f + batch[t].y_squashed * w_f;
            double err=target-pred;
            tl+=err*err;
            double dyq = err*w_f + dL_dyq_hist[t];
            g_wf+=err*batch[t].y_squashed; g_bf+=err;

            double dL_dstm[STM_SIZE][DIM_PER_SLOT];
            b.backward(batch[t], dyq, dL_da1_hist[t].data(), dL_dstm);

            // BPTT: propagate dL/dstm back to past timesteps
            // Slot s maps to time t-(STM_SIZE-s) in ring buffer order
            for(int s=0; s<STM_SIZE; s++) {
                int past_t = t - (STM_SIZE - s);
                if(past_t >= 0) {
                    dL_dyq_hist[past_t]    += dL_dstm[s][0]; // y_squashed
                    dL_da1_hist[past_t][0]  += dL_dstm[s][1]; // a1[0]
                    dL_da1_hist[past_t][1]  += dL_dstm[s][2]; // a1[1]
                    dL_da1_hist[past_t][2]  += dL_dstm[s][3]; // a1[2]
                }
            }
        }

        b.clip(10.0); b.update(lr);
        w_f+=lr*g_wf; b_f+=lr*g_bf;

        if(epoch%1000==0) {
            std::cout << "Epoch " << std::setw(5) << epoch << " | MSE: " << tl/(seq.size()-10) << "\n";
        }
    }

    std::cout << "\n--- FINAL CHECK (NEW RANDOM SEQ) ---\n";
    std::vector<double> f_seq; for(int i=0; i<30; i++) f_seq.push_back((gen()%2==0)?0.5:-0.5);
    double stm_f[STM_SIZE][DIM_PER_SLOT]; for(int i=0; i<STM_SIZE; i++) for(int d=0; d<DIM_PER_SLOT; d++) stm_f[i][d]=0;
    int ptr_f=0;
    for(int t=0; t<(int)f_seq.size(); t++) {
        double in[3]={f_seq[t],0,0};
        SovereignCache c=b.forward(in, stm_f, ptr_f);
        double p=b_f+c.y_squashed*w_f;
        double tar=(t>=DELAY)?f_seq[t-DELAY]:0;
        std::cout << "T=" << std::setw(2) << t << " | In: " << std::setw(4) << f_seq[t] << " | Target: " << std::setw(4) << tar << " | Pred: " << std::setw(8) << p << "\n";
        stm_f[ptr_f][0]=c.y_squashed; stm_f[ptr_f][1]=c.a1[0]; stm_f[ptr_f][2]=c.a1[1]; stm_f[ptr_f][3]=c.a1[2];
        ptr_f=(ptr_f+1)%STM_SIZE;
    }
    return 0;
}
