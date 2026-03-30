#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <random>

// --- SYSTEM UTILS ---
double relu(double x) { return x > 0 ? x : 0; }
double relu_derivative(double x) { return x > 0 ? 1.0 : 0.0; }
double sigmoid(double x) { return 1.0 / (1.0 + std::exp(-x)); }

// --- DIMENSIONS ---
static const int GRU_IN = 1;    // [x]
static const int H_DIM = 24;    // hidden state size (scaled for Echo-12)
static const int GRU_CONCAT = GRU_IN + H_DIM; // 25
static const int FFN_IN = 1 + H_DIM; // [x, h[0..23]] = 25
static const int HIDDEN = 32;

// --- CACHE ---
struct SovereignCache {
    double gru_in[GRU_IN], concat_zh[GRU_CONCAT];
    double pre_z[H_DIM], z[H_DIM];          // update gate
    double pre_r[H_DIM], r[H_DIM];          // reset gate
    double rh[H_DIM], concat_rh[GRU_CONCAT]; // reset * h_prev
    double pre_hc[H_DIM], h_cand[H_DIM];    // candidate
    double h_prev[H_DIM], h_new[H_DIM];     // states
    // FFN
    double ff_in[FFN_IN], z1[HIDDEN], a1[HIDDEN], y_feat_raw, y_squashed;
};

// --- SOVEREIGN BLOCK v4.2 (PURE GRU + HIGHWAY) ---
struct SovereignBlock {
    // GRU Gates (clean, no bloat)
    double W_z[H_DIM][GRU_CONCAT], b_z[H_DIM]; // update gate
    double W_r[H_DIM][GRU_CONCAT], b_r[H_DIM]; // reset gate
    double W_h[H_DIM][GRU_CONCAT], b_h[H_DIM]; // candidate
    // FFN
    double w1[HIDDEN][FFN_IN], b1[HIDDEN], w_feat[HIDDEN], b_feat;
    double w_read[H_DIM]; // Direct readout from hidden state (gradient highway)

    // Gradient accumulators
    double grad_W_z[H_DIM][GRU_CONCAT], grad_b_z[H_DIM];
    double grad_W_r[H_DIM][GRU_CONCAT], grad_b_r[H_DIM];
    double grad_W_h[H_DIM][GRU_CONCAT], grad_b_h[H_DIM];
    double grad_w1[HIDDEN][FFN_IN], grad_b1[HIDDEN], grad_w_feat[HIDDEN], grad_b_feat;
    double grad_w_read[H_DIM];

    SovereignBlock() {
        std::mt19937 gen(42);
        std::uniform_real_distribution<double> dis(-0.05, 0.05);
        for(int i=0;i<H_DIM;i++) {
            b_z[i]=-2.0; b_r[i]=0; b_h[i]=0; // b_z negative → gate starts "remembering"
            for(int j=0;j<GRU_CONCAT;j++) { W_z[i][j]=dis(gen); W_r[i][j]=dis(gen); W_h[i][j]=dis(gen); }
        }
        for(int i=0;i<HIDDEN;i++) { b1[i]=0.01; w_feat[i]=dis(gen); for(int j=0;j<FFN_IN;j++) w1[i][j]=dis(gen); }
        for(int i=0;i<H_DIM;i++) w_read[i]=dis(gen);
        b_feat=0; zero_gradients();
    }

    void zero_gradients() {
        for(int i=0;i<H_DIM;i++) {
            grad_b_z[i]=grad_b_r[i]=grad_b_h[i]=0;
            for(int j=0;j<GRU_CONCAT;j++) { grad_W_z[i][j]=grad_W_r[i][j]=grad_W_h[i][j]=0; }
        }
        for(int i=0;i<HIDDEN;i++) { grad_b1[i]=grad_w_feat[i]=0; for(int j=0;j<FFN_IN;j++) grad_w1[i][j]=0; }
        for(int i=0;i<H_DIM;i++) grad_w_read[i]=0;
        grad_b_feat=0;
    }

    SovereignCache forward(double x, double* h_state) {
        SovereignCache c;
        // 1. GRU Update
        c.gru_in[0]=x;
        for(int i=0;i<H_DIM;i++) c.h_prev[i]=h_state[i];

        // Concat [gru_in, h_prev]
        c.concat_zh[0]=c.gru_in[0];
        for(int j=0;j<H_DIM;j++) c.concat_zh[1+j]=c.h_prev[j];

        // Update gate: z = sigmoid(W_z * concat + b_z)
        for(int i=0;i<H_DIM;i++) {
            c.pre_z[i]=b_z[i]; for(int j=0;j<GRU_CONCAT;j++) c.pre_z[i]+=W_z[i][j]*c.concat_zh[j];
            c.z[i]=sigmoid(c.pre_z[i]);
        }
        // Reset gate: r = sigmoid(W_r * concat + b_r)
        for(int i=0;i<H_DIM;i++) {
            c.pre_r[i]=b_r[i]; for(int j=0;j<GRU_CONCAT;j++) c.pre_r[i]+=W_r[i][j]*c.concat_zh[j];
            c.r[i]=sigmoid(c.pre_r[i]);
        }
        // Reset * h_prev
        for(int i=0;i<H_DIM;i++) c.rh[i]=c.r[i]*c.h_prev[i];
        
        // Concat [gru_in, r*h_prev]
        c.concat_rh[0]=c.gru_in[0];
        for(int j=0;j<H_DIM;j++) c.concat_rh[1+j]=c.rh[j];
        
        // Candidate: h_cand = tanh(W_h * concat_rh + b_h)
        for(int i=0;i<H_DIM;i++) {
            c.pre_hc[i]=b_h[i]; for(int j=0;j<GRU_CONCAT;j++) c.pre_hc[i]+=W_h[i][j]*c.concat_rh[j];
            c.h_cand[i]=std::tanh(c.pre_hc[i]);
        }
        // New state: h_new = (1-z)*h_prev + z*h_cand
        for(int i=0;i<H_DIM;i++) {
            c.h_new[i]=(1.0-c.z[i])*c.h_prev[i] + c.z[i]*c.h_cand[i];
            h_state[i]=c.h_new[i]; // Update external state
        }

        // 2. FFN
        c.ff_in[0]=x;
        for(int i=0;i<H_DIM;i++) c.ff_in[1+i]=c.h_new[i];

        for(int i=0;i<HIDDEN;i++) {
            c.z1[i]=b1[i]; for(int j=0;j<FFN_IN;j++) c.z1[i]+=c.ff_in[j]*w1[i][j];
            c.a1[i]=relu(c.z1[i]);
        }
        c.y_feat_raw=b_feat; for(int i=0;i<HIDDEN;i++) c.y_feat_raw+=c.a1[i]*w_feat[i];
        
        // Direct readout (gradient highway)
        for(int i=0;i<H_DIM;i++) c.y_feat_raw+=c.h_new[i]*w_read[i];
        c.y_squashed=c.y_feat_raw/(1.0+std::abs(c.y_feat_raw));
        return c;
    }

    // Backward: takes dL/dy_squashed + dL/dh_new (from future), returns dL/dh_prev (for BPTT)
    void backward(const SovereignCache& c, double dL_dy_squashed, double* dL_dh_inject, double* dL_dh_prev_out) {
        // --- FFN Backward ---
        double dL_dy_raw = dL_dy_squashed / std::pow(1.0+std::abs(c.y_feat_raw),2);
        grad_b_feat+=dL_dy_raw;
        double dL_da1[HIDDEN];
        for(int i=0;i<HIDDEN;i++) { grad_w_feat[i]+=dL_dy_raw*c.a1[i]; dL_da1[i]=dL_dy_raw*w_feat[i]; }
        double dL_dff[FFN_IN]; for(int j=0;j<FFN_IN;j++) dL_dff[j]=0;
        for(int i=0;i<HIDDEN;i++) {
            double dz=dL_da1[i]*relu_derivative(c.z1[i]); grad_b1[i]+=dz;
            for(int j=0;j<FFN_IN;j++) { grad_w1[i][j]+=dz*c.ff_in[j]; dL_dff[j]+=dz*w1[i][j]; }
        }

        // dL/dh_new from FFN + direct readout + injected from future
        double dL_dh_new[H_DIM];
        for(int i=0;i<H_DIM;i++) {
            dL_dh_new[i] = dL_dff[1+i] + dL_dy_raw*w_read[i]; // Direct path!
            if(dL_dh_inject) dL_dh_new[i] += dL_dh_inject[i];
            grad_w_read[i] += dL_dy_raw * c.h_new[i]; // Accumulate direct read grad
        }

        // --- GRU Backward ---
        // h_new = (1-z)*h_prev + z*h_cand
        double dL_dz[H_DIM], dL_dh_cand[H_DIM], dL_dh_prev[H_DIM];
        for(int i=0;i<H_DIM;i++) {
            dL_dz[i] = dL_dh_new[i] * (c.h_cand[i] - c.h_prev[i]);
            dL_dh_cand[i] = dL_dh_new[i] * c.z[i];
            dL_dh_prev[i] = dL_dh_new[i] * (1.0 - c.z[i]); // direct path
        }

        // h_cand = tanh(pre_hc) → dL/dpre_hc
        double dL_dpre_hc[H_DIM];
        for(int i=0;i<H_DIM;i++) dL_dpre_hc[i] = dL_dh_cand[i] * (1.0 - c.h_cand[i]*c.h_cand[i]);

        // Accumulate W_h gradients
        for(int i=0;i<H_DIM;i++) {
            grad_b_h[i]+=dL_dpre_hc[i];
            for(int j=0;j<GRU_CONCAT;j++) grad_W_h[i][j]+=dL_dpre_hc[i]*c.concat_rh[j];
        }
        // dL/d(concat_rh) → dL/d(rh) → dL/dr, dL/dh_prev (through r path)
        double dL_drh[H_DIM];
        for(int i=0;i<H_DIM;i++) {
            dL_drh[i]=0;
            for(int ii=0;ii<H_DIM;ii++) dL_drh[i]+=dL_dpre_hc[ii]*W_h[ii][1+i];
        }
        double dL_dr[H_DIM];
        for(int i=0;i<H_DIM;i++) {
            dL_dr[i] = dL_drh[i] * c.h_prev[i];
            dL_dh_prev[i] += dL_drh[i] * c.r[i]; // r path to h_prev
        }

        // z gate: z = sigmoid(pre_z) → dL/dpre_z
        double dL_dpre_z[H_DIM];
        for(int i=0;i<H_DIM;i++) dL_dpre_z[i] = dL_dz[i] * c.z[i] * (1.0-c.z[i]);
        for(int i=0;i<H_DIM;i++) {
            grad_b_z[i]+=dL_dpre_z[i];
            for(int j=0;j<GRU_CONCAT;j++) grad_W_z[i][j]+=dL_dpre_z[i]*c.concat_zh[j];
        }
        // z gate path to h_prev
        for(int i=0;i<H_DIM;i++)
            for(int ii=0;ii<H_DIM;ii++) dL_dh_prev[i]+=dL_dpre_z[ii]*W_z[ii][1+i];

        // r gate: r = sigmoid(pre_r) → dL/dpre_r
        double dL_dpre_r[H_DIM];
        for(int i=0;i<H_DIM;i++) dL_dpre_r[i] = dL_dr[i] * c.r[i] * (1.0-c.r[i]);
        for(int i=0;i<H_DIM;i++) {
            grad_b_r[i]+=dL_dpre_r[i];
            for(int j=0;j<GRU_CONCAT;j++) grad_W_r[i][j]+=dL_dpre_r[i]*c.concat_zh[j];
        }
        // r gate path to h_prev
        for(int i=0;i<H_DIM;i++)
            for(int ii=0;ii<H_DIM;ii++) dL_dh_prev[i]+=dL_dpre_r[ii]*W_r[ii][1+i];

        if(dL_dh_prev_out) for(int i=0;i<H_DIM;i++) dL_dh_prev_out[i]=dL_dh_prev[i];
    }

    void clip(double thresh) {
        double ns=grad_b_feat*grad_b_feat;
        for(int i=0;i<H_DIM;i++) { ns+=grad_b_z[i]*grad_b_z[i]+grad_b_r[i]*grad_b_r[i]+grad_b_h[i]*grad_b_h[i]+grad_w_read[i]*grad_w_read[i];
            for(int j=0;j<GRU_CONCAT;j++) ns+=grad_W_z[i][j]*grad_W_z[i][j]+grad_W_r[i][j]*grad_W_r[i][j]+grad_W_h[i][j]*grad_W_h[i][j]; }
        for(int i=0;i<HIDDEN;i++) { ns+=grad_b1[i]*grad_b1[i]+grad_w_feat[i]*grad_w_feat[i]; for(int j=0;j<FFN_IN;j++) ns+=grad_w1[i][j]*grad_w1[i][j]; }
        double n=std::sqrt(ns); if(n>thresh) {
            double s=thresh/n; grad_b_feat*=s;
            for(int i=0;i<H_DIM;i++) { grad_b_z[i]*=s; grad_b_r[i]*=s; grad_b_h[i]*=s; grad_w_read[i]*=s;
                for(int j=0;j<GRU_CONCAT;j++) { grad_W_z[i][j]*=s; grad_W_r[i][j]*=s; grad_W_h[i][j]*=s; } }
            for(int i=0;i<HIDDEN;i++) { grad_b1[i]*=s; grad_w_feat[i]*=s; for(int j=0;j<FFN_IN;j++) grad_w1[i][j]*=s; }
        }
    }

    void update(double lr) {
        for(int i=0;i<H_DIM;i++) { b_z[i]+=lr*grad_b_z[i]; b_r[i]+=lr*grad_b_r[i]; b_h[i]+=lr*grad_b_h[i]; w_read[i]+=lr*grad_w_read[i];
            for(int j=0;j<GRU_CONCAT;j++) { W_z[i][j]+=lr*grad_W_z[i][j]; W_r[i][j]+=lr*grad_W_r[i][j]; W_h[i][j]+=lr*grad_W_h[i][j]; } }
        for(int i=0;i<HIDDEN;i++) { b1[i]+=lr*grad_b1[i]; w_feat[i]+=lr*grad_w_feat[i]; for(int j=0;j<FFN_IN;j++) w1[i][j]+=lr*grad_w1[i][j]; }
        b_feat+=lr*grad_b_feat; zero_gradients();
    }

    int count_params() {
        return H_DIM*GRU_CONCAT*3 + H_DIM*3 + H_DIM + HIDDEN*FFN_IN + HIDDEN + HIDDEN + 1;
    }
};

int main() {
    SovereignBlock b;
    double w_f=0.5, b_f=0, g_wf=0, g_bf=0;
    std::mt19937 gen(1337);
    int DELAY = 12;

    std::cout << "--- SOVEREIGN v4.2 (PURE PURE GRU ABLATED: ECHO-" << DELAY << ") ---\n";
    std::cout << "Parameters: " << b.count_params()+2 << "\n";
    std::cout << "Hidden State: " << H_DIM << "D | FFN Input: " << FFN_IN << "\n\n";

    double lr=0.01;
    for(int epoch=0; epoch<40001; epoch++) { // Run for 40k epochs since Echo-12 is hard
        std::vector<double> seq; for(int i=0;i<100;i++) seq.push_back((gen()%2==0)?0.5:-0.5);
        double tl=0; std::vector<SovereignCache> batch;
        double h_state[H_DIM]; for(int i=0;i<H_DIM;i++) h_state[i]=0;

        // Forward
        for(int t=0;t<(int)seq.size();t++) {
            SovereignCache c=b.forward(seq[t], h_state);
            batch.push_back(c);
        }

        // Backward (BPTT through hidden state)
        double dL_dh[H_DIM]; for(int i=0;i<H_DIM;i++) dL_dh[i]=0;
        b.zero_gradients(); g_wf=g_bf=0;

        for(int t=seq.size()-1; t>=10; t--) {
            double target=(t>=DELAY)?seq[t-DELAY]:0.0;
            double pred=b_f + batch[t].y_squashed * w_f;
            double err=target-pred;
            tl+=err*err;
            double dyq=err*w_f;
            g_wf+=err*batch[t].y_squashed; g_bf+=err;

            double dL_dh_prev[H_DIM];
            b.backward(batch[t], dyq, dL_dh, dL_dh_prev);
            for(int i=0;i<H_DIM;i++) dL_dh[i]=dL_dh_prev[i]; // carry back
        }

        if(epoch%2000==0) {
            double gru_ns=0, ffn_ns=0;
            for(int i=0;i<H_DIM;i++) for(int j=0;j<GRU_CONCAT;j++) gru_ns+=b.grad_W_z[i][j]*b.grad_W_z[i][j]+b.grad_W_r[i][j]*b.grad_W_r[i][j]+b.grad_W_h[i][j]*b.grad_W_h[i][j];
            for(int i=0;i<HIDDEN;i++) for(int j=0;j<FFN_IN;j++) ffn_ns+=b.grad_w1[i][j]*b.grad_w1[i][j];
            std::cout << "Epoch " << std::setw(5) << epoch << " | MSE: " << tl/(seq.size()-10) << " | GRU_grad: " << std::sqrt(gru_ns) << " | FFN_grad: " << std::sqrt(ffn_ns) << "\n";
        }

        b.clip(5.0); b.update(lr);
        w_f+=lr*g_wf; b_f+=lr*g_bf;
    }

    std::cout << "\n--- FINAL CHECK (NEW RANDOM SEQ) ---\n";
    std::vector<double> f_seq; for(int i=0;i<30;i++) f_seq.push_back((gen()%2==0)?0.5:-0.5);
    double h_f[H_DIM]; for(int i=0;i<H_DIM;i++) h_f[i]=0;
    for(int t=0;t<(int)f_seq.size();t++) {
        SovereignCache c=b.forward(f_seq[t], h_f);
        double p=b_f+c.y_squashed*w_f;
        double tar=(t>=DELAY)?f_seq[t-DELAY]:0;
        std::cout << "T=" << std::setw(2) << t << " | In: " << std::setw(4) << f_seq[t] << " | Target: " << std::setw(4) << tar << " | Pred: " << std::setw(8) << p << "\n";
    }
    return 0;
}
