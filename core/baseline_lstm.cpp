#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <random>
#include <string>
#include <fstream>
#include <streambuf>

double sigmoid(double x) { return 1.0 / (1.0 + std::exp(-x)); }

void softmax(double* logits, double* probs, int size) {
    double max_val = logits[0];
    for(int i=1; i<size; i++) if(logits[i] > max_val) max_val = logits[i];
    double sum = 0.0;
    for(int i=0; i<size; i++) {
        probs[i] = std::exp(logits[i] - max_val);
        sum += probs[i];
    }
    for(int i=0; i<size; i++) probs[i] /= sum;
}

static const int VOCAB = 256;
static const int EMBED_DIM = 8;
static const int H_DIM = 75;
static const int LSTM_CONCAT = EMBED_DIM + H_DIM; // 83

struct LSTMCache {
    int input_char;
    double embed[EMBED_DIM];
    double concat[LSTM_CONCAT];
    double pre_i[H_DIM], i_gate[H_DIM];
    double pre_f[H_DIM], f_gate[H_DIM];
    double pre_o[H_DIM], o_gate[H_DIM];
    double pre_c[H_DIM], c_cand[H_DIM];
    double c_prev[H_DIM], c_new[H_DIM];
    double h_prev[H_DIM], h_new[H_DIM];
    double logits[VOCAB], probs[VOCAB];
};

struct LSTMBlock {
    double W_embed[VOCAB][EMBED_DIM];
    double W_i[H_DIM][LSTM_CONCAT], b_i[H_DIM];
    double W_f[H_DIM][LSTM_CONCAT], b_f[H_DIM];
    double W_o[H_DIM][LSTM_CONCAT], b_o[H_DIM];
    double W_c[H_DIM][LSTM_CONCAT], b_c[H_DIM];
    double W_out[VOCAB][H_DIM], b_out[VOCAB];

    double grad_W_embed[VOCAB][EMBED_DIM];
    double grad_W_i[H_DIM][LSTM_CONCAT], grad_b_i[H_DIM];
    double grad_W_f[H_DIM][LSTM_CONCAT], grad_b_f[H_DIM];
    double grad_W_o[H_DIM][LSTM_CONCAT], grad_b_o[H_DIM];
    double grad_W_c[H_DIM][LSTM_CONCAT], grad_b_c[H_DIM];
    double grad_W_out[VOCAB][H_DIM], grad_b_out[VOCAB];

    LSTMBlock() {
        std::mt19937 gen(42);
        std::uniform_real_distribution<double> dis(-0.05, 0.05);

        for(int v=0; v<VOCAB; v++) {
            b_out[v] = 0;
            for(int d=0; d<EMBED_DIM; d++) W_embed[v][d] = dis(gen);
            for(int i=0; i<H_DIM; i++) W_out[v][i] = dis(gen);
        }

        for(int i=0; i<H_DIM; i++) {
            b_i[i]=0; b_o[i]=0; b_c[i]=0; b_f[i]=1.0; // standard LSTM forget bias initialization
            for(int j=0; j<LSTM_CONCAT; j++) {
                W_i[i][j]=dis(gen); W_f[i][j]=dis(gen); W_o[i][j]=dis(gen); W_c[i][j]=dis(gen);
            }
        }
        zero_gradients();
    }

    void zero_gradients() {
        for(int v=0; v<VOCAB; v++) {
            grad_b_out[v] = 0;
            for(int d=0; d<EMBED_DIM; d++) grad_W_embed[v][d] = 0;
            for(int i=0; i<H_DIM; i++) grad_W_out[v][i] = 0;
        }
        for(int i=0; i<H_DIM; i++) {
            grad_b_i[i]=grad_b_f[i]=grad_b_o[i]=grad_b_c[i]=0;
            for(int j=0; j<LSTM_CONCAT; j++) {
                grad_W_i[i][j]=grad_W_f[i][j]=grad_W_o[i][j]=grad_W_c[i][j]=0;
            }
        }
    }

    LSTMCache forward(int x, double* h_state, double* c_state) {
        LSTMCache L;
        L.input_char = x;
        for(int d=0; d<EMBED_DIM; d++) L.embed[d] = W_embed[x][d];
        for(int i=0; i<H_DIM; i++) { L.h_prev[i] = h_state[i]; L.c_prev[i] = c_state[i]; }

        for(int j=0; j<EMBED_DIM; j++) L.concat[j] = L.embed[j];
        for(int j=0; j<H_DIM; j++) L.concat[EMBED_DIM+j] = L.h_prev[j];

        for(int i=0; i<H_DIM; i++) {
            double zi=b_i[i], zf=b_f[i], zo=b_o[i], zc=b_c[i];
            for(int j=0; j<LSTM_CONCAT; j++) {
                zi += W_i[i][j]*L.concat[j];
                zf += W_f[i][j]*L.concat[j];
                zo += W_o[i][j]*L.concat[j];
                zc += W_c[i][j]*L.concat[j];
            }
            L.pre_i[i]=zi; L.i_gate[i]=sigmoid(zi);
            L.pre_f[i]=zf; L.f_gate[i]=sigmoid(zf);
            L.pre_o[i]=zo; L.o_gate[i]=sigmoid(zo);
            L.pre_c[i]=zc; L.c_cand[i]=std::tanh(zc);
            
            L.c_new[i] = L.f_gate[i]*L.c_prev[i] + L.i_gate[i]*L.c_cand[i];
            L.h_new[i] = L.o_gate[i]*std::tanh(L.c_new[i]);

            c_state[i] = L.c_new[i];
            h_state[i] = L.h_new[i];
        }

        for(int v=0; v<VOCAB; v++) {
            L.logits[v] = b_out[v];
            for(int i=0; i<H_DIM; i++) L.logits[v] += L.h_new[i] * W_out[v][i];
        }
        softmax(L.logits, L.probs, VOCAB);

        return L;
    }

    void backward(const LSTMCache& L, int target_char, double* dL_dh_inject, double* dL_dc_inject, double* dL_dh_prev_out, double* dL_dc_prev_out) {
        double dL_dlogits[VOCAB];
        for(int v=0; v<VOCAB; v++) {
            dL_dlogits[v] = -L.probs[v];
            if(v == target_char) dL_dlogits[v] += 1.0;
        }

        double dL_dh_new[H_DIM]; for(int i=0; i<H_DIM; i++) dL_dh_new[i] = dL_dh_inject ? dL_dh_inject[i] : 0;
        double dL_dc_new[H_DIM]; for(int i=0; i<H_DIM; i++) dL_dc_new[i] = dL_dc_inject ? dL_dc_inject[i] : 0;

        for(int v=0; v<VOCAB; v++) {
            double dl = dL_dlogits[v];
            grad_b_out[v] += dl;
            for(int i=0; i<H_DIM; i++) {
                grad_W_out[v][i] += dl * L.h_new[i];
                dL_dh_new[i] += dl * W_out[v][i];
            }
        }

        double dL_dconcat[LSTM_CONCAT]; for(int j=0; j<LSTM_CONCAT; j++) dL_dconcat[j]=0;
        double dL_dh_prev[H_DIM]; for(int i=0; i<H_DIM; i++) dL_dh_prev[i]=0;
        double dL_dc_prev[H_DIM]; for(int i=0; i<H_DIM; i++) dL_dc_prev[i]=0;

        for(int i=0; i<H_DIM; i++) {
            double th_c = std::tanh(L.c_new[i]);
            double do_gate = dL_dh_new[i] * th_c;
            dL_dc_new[i] += dL_dh_new[i] * L.o_gate[i] * (1.0 - th_c*th_c);

            double df_gate = dL_dc_new[i] * L.c_prev[i];
            dL_dc_prev[i] = dL_dc_new[i] * L.f_gate[i];

            double di_gate = dL_dc_new[i] * L.c_cand[i];
            double dc_cand = dL_dc_new[i] * L.i_gate[i];

            double dpre_i = di_gate * L.i_gate[i]*(1.0 - L.i_gate[i]);
            double dpre_f = df_gate * L.f_gate[i]*(1.0 - L.f_gate[i]);
            double dpre_o = do_gate * L.o_gate[i]*(1.0 - L.o_gate[i]);
            double dpre_c = dc_cand * (1.0 - L.c_cand[i]*L.c_cand[i]);

            grad_b_i[i] += dpre_i; grad_b_f[i] += dpre_f; grad_b_o[i] += dpre_o; grad_b_c[i] += dpre_c;
            for(int j=0; j<LSTM_CONCAT; j++) {
                grad_W_i[i][j] += dpre_i * L.concat[j]; dL_dconcat[j] += dpre_i * W_i[i][j];
                grad_W_f[i][j] += dpre_f * L.concat[j]; dL_dconcat[j] += dpre_f * W_f[i][j];
                grad_W_o[i][j] += dpre_o * L.concat[j]; dL_dconcat[j] += dpre_o * W_o[i][j];
                grad_W_c[i][j] += dpre_c * L.concat[j]; dL_dconcat[j] += dpre_c * W_c[i][j];
            }
        }

        for(int j=0; j<EMBED_DIM; j++) grad_W_embed[L.input_char][j] += dL_dconcat[j];
        for(int j=0; j<H_DIM; j++) dL_dh_prev[j] = dL_dconcat[EMBED_DIM+j];

        if(dL_dh_prev_out) for(int i=0; i<H_DIM; i++) dL_dh_prev_out[i] = dL_dh_prev[i];
        if(dL_dc_prev_out) for(int i=0; i<H_DIM; i++) dL_dc_prev_out[i] = dL_dc_prev[i];
    }

    void clip(double thresh) {
        double ns = 0;
        for(int v=0;v<VOCAB;v++) {
            ns+=grad_b_out[v]*grad_b_out[v];
            for(int d=0;d<EMBED_DIM;d++) ns+=grad_W_embed[v][d]*grad_W_embed[v][d];
            for(int i=0;i<H_DIM;i++) ns+=grad_W_out[v][i]*grad_W_out[v][i];
        }
        for(int i=0;i<H_DIM;i++) {
            ns+=grad_b_i[i]*grad_b_i[i]+grad_b_f[i]*grad_b_f[i]+grad_b_o[i]*grad_b_o[i]+grad_b_c[i]*grad_b_c[i];
            for(int j=0;j<LSTM_CONCAT;j++) {
                ns+=grad_W_i[i][j]*grad_W_i[i][j]+grad_W_f[i][j]*grad_W_f[i][j]+grad_W_o[i][j]*grad_W_o[i][j]+grad_W_c[i][j]*grad_W_c[i][j];
            }
        }
        double n=std::sqrt(ns); if(n>thresh) {
            double s=thresh/n; 
            for(int v=0;v<VOCAB;v++) { grad_b_out[v]*=s; for(int d=0;d<EMBED_DIM;d++) grad_W_embed[v][d]*=s; for(int i=0;i<H_DIM;i++) grad_W_out[v][i]*=s; }
            for(int i=0;i<H_DIM;i++) {
                grad_b_i[i]*=s; grad_b_f[i]*=s; grad_b_o[i]*=s; grad_b_c[i]*=s;
                for(int j=0;j<LSTM_CONCAT;j++) { grad_W_i[i][j]*=s; grad_W_f[i][j]*=s; grad_W_o[i][j]*=s; grad_W_c[i][j]*=s; }
            }
        }
    }

    void update(double lr) {
        for(int v=0;v<VOCAB;v++) { b_out[v]+=lr*grad_b_out[v]; for(int d=0;d<EMBED_DIM;d++) W_embed[v][d]+=lr*grad_W_embed[v][d]; for(int i=0;i<H_DIM;i++) W_out[v][i]+=lr*grad_W_out[v][i]; }
        for(int i=0;i<H_DIM;i++) {
            b_i[i]+=lr*grad_b_i[i]; b_f[i]+=lr*grad_b_f[i]; b_o[i]+=lr*grad_b_o[i]; b_c[i]+=lr*grad_b_c[i];
            for(int j=0;j<LSTM_CONCAT;j++) { W_i[i][j]+=lr*grad_W_i[i][j]; W_f[i][j]+=lr*grad_W_f[i][j]; W_o[i][j]+=lr*grad_W_o[i][j]; W_c[i][j]+=lr*grad_W_c[i][j]; }
        }
        zero_gradients();
    }

    int count_params() {
        return VOCAB*EMBED_DIM + VOCAB*H_DIM + VOCAB +
               4 * H_DIM * LSTM_CONCAT + 4 * H_DIM;
    }
};

int main() {
    LSTMBlock b;
    std::mt19937 b_gen(42); 
    std::cout << "--- BASELINE LSTM v1.0 (Phase 6 Kill Switch Test) ---\n";
    std::cout << "Parameters: " << b.count_params() << "\n";

    std::ifstream file("tiny_shakespeare.txt");
    if(!file.is_open()) { std::cerr << "Failed to open dataset.\n"; return 1; }
    std::string full_text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    int DATASET_SIZE = 10000;
    if(full_text.size() < DATASET_SIZE) DATASET_SIZE = full_text.size();
    
    std::vector<int> dataset;
    for(int i=0; i<DATASET_SIZE; i++) dataset.push_back((unsigned char)full_text[i]);

    int SEQ_LEN = 64; 
    double lr = 0.002; 
    int EPOCHS = 101; 

    for(int epoch = 0; epoch < EPOCHS; epoch++) {
        double epoch_loss = 0;
        int num_chunks = 0;

        double h_state[H_DIM]; for(int i=0;i<H_DIM;i++) h_state[i]=0;
        double c_state[H_DIM]; for(int i=0;i<H_DIM;i++) c_state[i]=0;

        for(int i=0; i < DATASET_SIZE - SEQ_LEN; i += SEQ_LEN) {
            std::vector<LSTMCache> batch;
            
            for(int t=0; t<SEQ_LEN; t++) {
                LSTMCache c = b.forward(dataset[i+t], h_state, c_state);
                batch.push_back(c);
            }

            double dL_dh[H_DIM]; for(int j=0;j<H_DIM;j++) dL_dh[j]=0;
            double dL_dc[H_DIM]; for(int j=0;j<H_DIM;j++) dL_dc[j]=0;
            b.zero_gradients();

            double chunk_loss = 0;
            for(int t=SEQ_LEN-1; t>=0; t--) {
                int target = dataset[i+t+1]; 
                double prob = batch[t].probs[target];
                if(prob < 1e-10) prob = 1e-10;
                chunk_loss += -std::log(prob);

                double dL_dh_prev[H_DIM], dL_dc_prev[H_DIM];
                b.backward(batch[t], target, dL_dh, dL_dc, dL_dh_prev, dL_dc_prev);
                for(int j=0; j<H_DIM; j++) { dL_dh[j] = dL_dh_prev[j]; dL_dc[j] = dL_dc_prev[j]; }
            }

            b.clip(5.0);
            b.update(lr);

            epoch_loss += chunk_loss;
            num_chunks++;
        }

        double avg_loss = epoch_loss / (num_chunks * SEQ_LEN);
        double bpc = avg_loss / std::log(2.0);

        if(epoch % 5 == 0 || epoch == EPOCHS - 1) {
            std::cout << "Epoch " << std::setw(3) << epoch << " | Loss: " << std::fixed << std::setprecision(4) << avg_loss 
                      << " | BPC: " << bpc << " | PPL: " << std::exp(avg_loss) << "\n";
            std::cout << "\n--- GENERATION ---\n";
            int curr = 'F'; 
            std::cout << (char)curr;
            double h_gen[H_DIM]; for(int j=0;j<H_DIM;j++) h_gen[j]=0;
            double c_gen[H_DIM]; for(int j=0;j<H_DIM;j++) c_gen[j]=0;
            std::uniform_real_distribution<double> dist_samp(0.0, 1.0);
            for(int step=0; step<200; step++) {
                LSTMCache c = b.forward(curr, h_gen, c_gen);
                double sum = 0;
                double scaled_probs[VOCAB];
                for(int v=0; v<VOCAB; v++) {
                    scaled_probs[v] = std::pow(c.probs[v], 1.0/0.8);
                    sum += scaled_probs[v];
                }
                double r = dist_samp(b_gen); 
                double acc = 0.0;
                int best = 0;
                for(int v=0; v<VOCAB; v++) {
                    acc += scaled_probs[v] / sum;
                    if(r <= acc) { best = v; break; }
                }
                std::cout << (char)best;
                curr = best;
            }
            std::cout << "\n------------------\n\n";
        }
    }
    return 0;
}
