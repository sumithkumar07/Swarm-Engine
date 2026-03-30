#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <random>
#include <string>

// --- SYSTEM UTILS ---
double relu(double x) { return x > 0 ? x : 0; }
double relu_derivative(double x) { return x > 0 ? 1.0 : 0.0; }
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

// --- DIMENSIONS ---
static const int VOCAB = 256;
static const int EMBED_DIM = 8; 
static const int GRU_IN = EMBED_DIM; 
static const int H_DIM = 64;
static const int GRU_CONCAT = GRU_IN + H_DIM;
static const int FFN_IN = EMBED_DIM + H_DIM;
static const int HIDDEN = 64;

// --- MASTER BRAIN (MATH) ---
struct SovereignCache {
    int input_char; 
    double gru_in[EMBED_DIM];
    double concat_zh[GRU_CONCAT], pre_z[H_DIM], z[H_DIM];
    double pre_r[H_DIM], r[H_DIM], rh[H_DIM], concat_rh[GRU_CONCAT];
    double pre_hc[H_DIM], h_cand[H_DIM], h_prev[H_DIM], h_new[H_DIM];
    double ff_in[FFN_IN], z1[HIDDEN], a1[HIDDEN];
    double logits[VOCAB], probs[VOCAB];
};

struct SovereignBlock {
    double W_embed[VOCAB][EMBED_DIM];
    double W_z[H_DIM][GRU_CONCAT], b_z[H_DIM];
    double W_r[H_DIM][GRU_CONCAT], b_r[H_DIM];
    double W_h[H_DIM][GRU_CONCAT], b_h[H_DIM];
    double w1[HIDDEN][FFN_IN], b1[HIDDEN];
    double W_out[VOCAB][HIDDEN], W_out_highway[VOCAB][H_DIM], b_out[VOCAB];

    double grad_W_embed[VOCAB][EMBED_DIM];
    double grad_W_z[H_DIM][GRU_CONCAT], grad_b_z[H_DIM];
    double grad_W_r[H_DIM][GRU_CONCAT], grad_b_r[H_DIM];
    double grad_W_h[H_DIM][GRU_CONCAT], grad_b_h[H_DIM];
    double grad_w1[HIDDEN][FFN_IN], grad_b1[HIDDEN];
    double grad_W_out[VOCAB][HIDDEN], grad_W_out_highway[VOCAB][H_DIM], grad_b_out[VOCAB];

    SovereignBlock() {
        std::mt19937 gen(101);
        std::uniform_real_distribution<double> dis(-0.05, 0.05);
        for(int v=0; v<VOCAB; v++) {
            b_out[v] = 0;
            for(int i=0; i<EMBED_DIM; i++) W_embed[v][i] = dis(gen);
            for(int i=0; i<HIDDEN; i++) W_out[v][i] = dis(gen);
            for(int i=0; i<H_DIM; i++) W_out_highway[v][i] = dis(gen);
        }
        for(int i=0; i<H_DIM; i++) {
            b_z[i]=-2.0; b_r[i]=0; b_h[i]=0;
            for(int j=0; j<GRU_CONCAT; j++) { W_z[i][j]=dis(gen); W_r[i][j]=dis(gen); W_h[i][j]=dis(gen); }
        }
        for(int i=0; i<HIDDEN; i++) { 
            b1[i]=0.01; 
            for(int j=0; j<FFN_IN; j++) w1[i][j]=dis(gen); 
        }
        zero_gradients();
    }

    void zero_gradients() {
        for(int v=0; v<VOCAB; v++) {
            grad_b_out[v] = 0;
            for(int i=0; i<EMBED_DIM; i++) grad_W_embed[v][i] = 0;
            for(int i=0; i<HIDDEN; i++) grad_W_out[v][i] = 0;
            for(int i=0; i<H_DIM; i++) grad_W_out_highway[v][i] = 0;
        }
        for(int i=0; i<H_DIM; i++) {
            grad_b_z[i]=grad_b_r[i]=grad_b_h[i]=0;
            for(int j=0; j<GRU_CONCAT; j++) { grad_W_z[i][j]=grad_W_r[i][j]=grad_W_h[i][j]=0; }
        }
        for(int i=0; i<HIDDEN; i++) { grad_b1[i]=0; for(int j=0; j<FFN_IN; j++) grad_w1[i][j]=0; }
    }

    SovereignCache forward(int x, double* h_state) {
        SovereignCache c; c.input_char = x;
        for(int d=0; d<EMBED_DIM; d++) c.gru_in[d] = W_embed[x][d];
        for(int i=0; i<H_DIM; i++) c.h_prev[i] = h_state[i];
        for(int j=0; j<EMBED_DIM; j++) c.concat_zh[j] = c.gru_in[j];
        for(int j=0; j<H_DIM; j++) c.concat_zh[EMBED_DIM+j] = c.h_prev[j];

        for(int i=0; i<H_DIM; i++) {
            c.pre_z[i]=b_z[i]; for(int j=0; j<GRU_CONCAT; j++) c.pre_z[i]+=W_z[i][j]*c.concat_zh[j];
            c.z[i]=sigmoid(c.pre_z[i]);
            c.pre_r[i]=b_r[i]; for(int j=0; j<GRU_CONCAT; j++) c.pre_r[i]+=W_r[i][j]*c.concat_zh[j];
            c.r[i]=sigmoid(c.pre_r[i]);
            c.rh[i]=c.r[i]*c.h_prev[i];
        }

        for(int j=0; j<EMBED_DIM; j++) c.concat_rh[j] = c.gru_in[j];
        for(int j=0; j<H_DIM; j++) c.concat_rh[EMBED_DIM+j] = c.rh[j];
        
        for(int i=0; i<H_DIM; i++) {
            c.pre_hc[i]=b_h[i]; for(int j=0; j<GRU_CONCAT; j++) c.pre_hc[i]+=W_h[i][j]*c.concat_rh[j];
            c.h_cand[i]=std::tanh(c.pre_hc[i]);
            c.h_new[i]=(1.0-c.z[i])*c.h_prev[i] + c.z[i]*c.h_cand[i];
            h_state[i]=c.h_new[i];
        }

        for(int j=0; j<EMBED_DIM; j++) c.ff_in[j] = c.gru_in[j];
        for(int j=0; j<H_DIM; j++) c.ff_in[EMBED_DIM+j] = c.h_new[j];

        for(int i=0; i<HIDDEN; i++) {
            c.z1[i]=b1[i]; for(int j=0; j<FFN_IN; j++) c.z1[i]+=c.ff_in[j]*w1[i][j];
            c.a1[i]=relu(c.z1[i]);
        }
        for(int v=0; v<VOCAB; v++) {
            c.logits[v] = b_out[v];
            for(int i=0; i<HIDDEN; i++) c.logits[v] += c.a1[i] * W_out[v][i];
            for(int i=0; i<H_DIM; i++) c.logits[v] += c.h_new[i] * W_out_highway[v][i];
        }
        softmax(c.logits, c.probs, VOCAB);
        return c;
    }

    void backward(const SovereignCache& c, int target_char, double* dL_dh_inject, double* dL_dh_prev_out) {
        double dL_dlogits[VOCAB];
        for(int v=0; v<VOCAB; v++) {
            dL_dlogits[v] = -c.probs[v];
            if(v == target_char) dL_dlogits[v] += 1.0;
        }

        double dL_da1[HIDDEN]; for(int i=0; i<HIDDEN; i++) dL_da1[i]=0;
        double dL_dh_new[H_DIM]; 
        for(int i=0; i<H_DIM; i++) dL_dh_new[i] = dL_dh_inject ? dL_dh_inject[i] : 0;

        for(int v=0; v<VOCAB; v++) {
            double dl = dL_dlogits[v];
            grad_b_out[v] += dl;
            for(int i=0; i<HIDDEN; i++) { grad_W_out[v][i] += dl * c.a1[i]; dL_da1[i] += dl * W_out[v][i]; }
            for(int i=0; i<H_DIM; i++) { grad_W_out_highway[v][i] += dl * c.h_new[i]; dL_dh_new[i] += dl * W_out_highway[v][i]; }
        }

        double dL_dff_in[FFN_IN]; for(int j=0; j<FFN_IN; j++) dL_dff_in[j]=0;
        for(int i=0; i<HIDDEN; i++) {
            double dz = dL_da1[i] * relu_derivative(c.z1[i]);
            grad_b1[i] += dz;
            for(int j=0; j<FFN_IN; j++) { grad_w1[i][j] += dz * c.ff_in[j]; dL_dff_in[j] += dz * w1[i][j]; }
        }

        for(int i=0; i<H_DIM; i++) dL_dh_new[i] += dL_dff_in[EMBED_DIM + i];
        double dL_dembed[EMBED_DIM];
        for(int d=0; d<EMBED_DIM; d++) dL_dembed[d] = dL_dff_in[d];

        double dL_dz[H_DIM], dL_dh_cand[H_DIM], dL_dh_prev[H_DIM];
        for(int i=0; i<H_DIM; i++) {
            dL_dz[i] = dL_dh_new[i] * (c.h_cand[i] - c.h_prev[i]);
            dL_dh_cand[i] = dL_dh_new[i] * c.z[i];
            dL_dh_prev[i] = dL_dh_new[i] * (1.0 - c.z[i]); 
        }

        double dL_dpre_hc[H_DIM];
        for(int i=0; i<H_DIM; i++) dL_dpre_hc[i] = dL_dh_cand[i] * (1.0 - c.h_cand[i]*c.h_cand[i]);

        for(int i=0; i<H_DIM; i++) {
            grad_b_h[i] += dL_dpre_hc[i];
            for(int j=0; j<GRU_CONCAT; j++) grad_W_h[i][j] += dL_dpre_hc[i]*c.concat_rh[j];
        }

        for(int j=0; j<EMBED_DIM; j++) {
            for(int i=0; i<H_DIM; i++) dL_dembed[j] += dL_dpre_hc[i] * W_h[i][j];
        }

        double dL_drh[H_DIM];
        for(int i=0; i<H_DIM; i++) {
            dL_drh[i]=0; for(int ii=0; ii<H_DIM; ii++) dL_drh[i] += dL_dpre_hc[ii]*W_h[ii][EMBED_DIM+i];
        }
        double dL_dr[H_DIM];
        for(int i=0; i<H_DIM; i++) {
            dL_dr[i] = dL_drh[i] * c.h_prev[i];
            dL_dh_prev[i] += dL_drh[i] * c.r[i];
        }

        double dL_dpre_z[H_DIM];
        for(int i=0; i<H_DIM; i++) dL_dpre_z[i] = dL_dz[i] * c.z[i] * (1.0-c.z[i]);
        for(int i=0; i<H_DIM; i++) {
            grad_b_z[i] += dL_dpre_z[i];
            for(int j=0; j<GRU_CONCAT; j++) grad_W_z[i][j] += dL_dpre_z[i]*c.concat_zh[j];
        }
        for(int j=0; j<EMBED_DIM; j++) {
            for(int i=0; i<H_DIM; i++) dL_dembed[j] += dL_dpre_z[i] * W_z[i][j];
        }
        for(int i=0; i<H_DIM; i++)
            for(int ii=0; ii<H_DIM; ii++) dL_dh_prev[i] += dL_dpre_z[ii]*W_z[ii][EMBED_DIM+i];

        double dL_dpre_r[H_DIM];
        for(int i=0; i<H_DIM; i++) dL_dpre_r[i] = dL_dr[i] * c.r[i] * (1.0-c.r[i]);
        for(int i=0; i<H_DIM; i++) {
            grad_b_r[i] += dL_dpre_r[i];
            for(int j=0; j<GRU_CONCAT; j++) grad_W_r[i][j] += dL_dpre_r[i]*c.concat_zh[j];
        }
        for(int j=0; j<EMBED_DIM; j++) {
            for(int i=0; i<H_DIM; i++) dL_dembed[j] += dL_dpre_r[i] * W_r[i][j];
        }
        for(int i=0; i<H_DIM; i++)
            for(int ii=0; ii<H_DIM; ii++) dL_dh_prev[i] += dL_dpre_r[ii]*W_r[ii][EMBED_DIM+i];

        for(int d=0; d<EMBED_DIM; d++) { grad_W_embed[c.input_char][d] += dL_dembed[d]; }
        if(dL_dh_prev_out) for(int i=0; i<H_DIM; i++) dL_dh_prev_out[i] = dL_dh_prev[i];
    }

    void clip(double thresh) {
        double ns = 0;
        for(int v=0;v<VOCAB;v++) { ns+=grad_b_out[v]*grad_b_out[v];
            for(int i=0;i<HIDDEN;i++) ns+=grad_W_out[v][i]*grad_W_out[v][i];
            for(int i=0;i<H_DIM;i++) ns+=grad_W_out_highway[v][i]*grad_W_out_highway[v][i];
            for(int i=0;i<EMBED_DIM;i++) ns+=grad_W_embed[v][i]*grad_W_embed[v][i]; }
        for(int i=0;i<H_DIM;i++) { ns+=grad_b_z[i]*grad_b_z[i]+grad_b_r[i]*grad_b_r[i]+grad_b_h[i]*grad_b_h[i];
            for(int j=0;j<GRU_CONCAT;j++) ns+=grad_W_z[i][j]*grad_W_z[i][j]+grad_W_r[i][j]*grad_W_r[i][j]+grad_W_h[i][j]*grad_W_h[i][j]; }
        for(int i=0;i<HIDDEN;i++) { ns+=grad_b1[i]*grad_b1[i]; for(int j=0;j<FFN_IN;j++) ns+=grad_w1[i][j]*grad_w1[i][j]; }
        
        double n=std::sqrt(ns); if(n>thresh) {
            double s=thresh/n; 
            for(int v=0;v<VOCAB;v++) { grad_b_out[v]*=s;
                for(int i=0;i<HIDDEN;i++) grad_W_out[v][i]*=s;
                for(int i=0;i<H_DIM;i++) grad_W_out_highway[v][i]*=s;
                for(int i=0;i<EMBED_DIM;i++) grad_W_embed[v][i]*=s; }
            for(int i=0;i<H_DIM;i++) { grad_b_z[i]*=s; grad_b_r[i]*=s; grad_b_h[i]*=s; 
                for(int j=0;j<GRU_CONCAT;j++) { grad_W_z[i][j]*=s; grad_W_r[i][j]*=s; grad_W_h[i][j]*=s; } }
            for(int i=0;i<HIDDEN;i++) { grad_b1[i]*=s; for(int j=0;j<FFN_IN;j++) grad_w1[i][j]*=s; }
        }
    }

    void update(double lr) {
        for(int v=0;v<VOCAB;v++) {
            b_out[v]+=lr*grad_b_out[v];
            for(int i=0;i<HIDDEN;i++) W_out[v][i]+=lr*grad_W_out[v][i];
            for(int i=0;i<H_DIM;i++) W_out_highway[v][i]+=lr*grad_W_out_highway[v][i];
            for(int i=0;i<EMBED_DIM;i++) W_embed[v][i]+=lr*grad_W_embed[v][i]; }
        for(int i=0;i<H_DIM;i++) { b_z[i]+=lr*grad_b_z[i]; b_r[i]+=lr*grad_b_r[i]; b_h[i]+=lr*grad_b_h[i]; 
            for(int j=0;j<GRU_CONCAT;j++) { W_z[i][j]+=lr*grad_W_z[i][j]; W_r[i][j]+=lr*grad_W_r[i][j]; W_h[i][j]+=lr*grad_W_h[i][j]; } }
        for(int i=0;i<HIDDEN;i++) { b1[i]+=lr*grad_b1[i]; for(int j=0;j<FFN_IN;j++) w1[i][j]+=lr*grad_w1[i][j]; }
        zero_gradients();
    }
};

// --- MULTI-AGENT SWARM ARCHITECTURE ---

class SovereignAgent {
public:
    std::string name;
    SovereignBlock brain;
    double h_state[H_DIM];
    std::mt19937 dist_gen;

    // Load natively from Master brain
    SovereignAgent(std::string name, const SovereignBlock& master, int rseed) 
        : name(name), brain(master), dist_gen(rseed) {
        for(int i=0; i<H_DIM; i++) h_state[i] = 0;
    }

    void observe(const std::string& text) {
        for(char c : text) {
            brain.forward((unsigned char)c, h_state);
        }
    }

    std::string act(int max_chars, double temperature = 0.4) {
        std::string generated = "";
        int curr = '\n'; // Seed from the end of the prompt event
        std::uniform_real_distribution<double> dist_samp(0.0, 1.0);
        
        for(int step=0; step<max_chars; step++) {
            SovereignCache c = brain.forward(curr, h_state);
            double sum = 0; double scaled_probs[VOCAB];
            for(int v=0; v<VOCAB; v++) {
                scaled_probs[v] = std::pow(c.probs[v], 1.0/temperature);
                sum += scaled_probs[v];
            }
            double r = dist_samp(dist_gen);
            double acc = 0.0; int best = 0;
            for(int v=0; v<VOCAB; v++) {
                acc += scaled_probs[v] / sum;
                if(r <= acc) { best = v; break; }
            }
            if (best < 32 && best != '\n') best = '.';
            generated += (char)best;
            curr = best;
            if (best == '\n') break;
        }
        return generated;
    }
};

class SocialTimeline {
public:
    std::vector<std::string> posts;

    void broadcast(std::string agent_name, std::string text) {
        std::string msg = "[" + agent_name + "]: " + text;
        posts.push_back(msg);
        std::cout << msg; // msg already has \n from generation
    }
};

// --- DYNAMIC ACADEMY (MARKET LOGIC) ---
std::vector<int> generate_market_logic(int num_characters) {
    std::string data = "";
    std::mt19937 gen(42);
    while(data.size() < num_characters) {
        int r = gen() % 10;
        data += "[EVENT: PRICE IS ";
        data += std::to_string(r);
        data += "]\n[RESPONSE]\n";
        if (r > 5) {
            data += "[ACTION: SELL]\n";
        } else {
            data += "[ACTION: BUY]\n";
        }
    }
    std::vector<int> out;
    for(int i=0; i<num_characters; i++) out.push_back((unsigned char)data[i]);
    return out;
}

int main() {
    std::cout << "=== SOVEREIGN V9 SWARM EMERGENCE ===\n";
    
    // -------------------------------------------------------------
    // PHASE 1: THE ACADEMY (PRE-TRAINING THE MASTER BRAIN NATIVELY)
    // -------------------------------------------------------------
    std::cout << "[SYSTEM] Compiling Synthetic Micro-Market Dataset...\n";
    std::vector<int> dataset = generate_market_logic(4000);
    
    std::cout << "[SYSTEM] Instantiating Master Sovereign Block...\n";
    SovereignBlock master;
    
    std::cout << "[SYSTEM] Master Brain initiating TBPTT Training Loop...\n";
    int SEQ_LEN = 32; 
    double lr = 0.007; 
    int EPOCHS = 40; 
    double h_state[H_DIM]; 

    for(int epoch = 1; epoch <= EPOCHS; epoch++) {
        double epoch_loss = 0; int num_chunks = 0;
        for(int i=0;i<H_DIM;i++) h_state[i]=0; 

        for(int i=0; i < dataset.size() - SEQ_LEN; i += SEQ_LEN) {
            std::vector<SovereignCache> batch;
            for(int t=0; t<SEQ_LEN; t++) {
                SovereignCache c = master.forward(dataset[i+t], h_state);
                batch.push_back(c);
            }
            double dL_dh[H_DIM]; for(int j=0;j<H_DIM;j++) dL_dh[j]=0;
            master.zero_gradients();
            double chunk_loss = 0;
            for(int t=SEQ_LEN-1; t>=0; t--) {
                int target = dataset[i+t+1]; 
                double prob = batch[t].probs[target];
                if(prob < 1e-10) prob = 1e-10;
                chunk_loss += -std::log(prob);
                double dL_dh_prev[H_DIM];
                master.backward(batch[t], target, dL_dh, dL_dh_prev);
                for(int j=0; j<H_DIM; j++) dL_dh[j] = dL_dh_prev[j];
            }
            master.clip(5.0);
            master.update(lr);
            epoch_loss += chunk_loss;
            num_chunks++;
        }
        if (epoch % 10 == 0) {
            std::cout << "         Epoch " << epoch << " | Loss: " << (epoch_loss / (num_chunks * SEQ_LEN)) << "\n";
        }
    }
    std::cout << "[SYSTEM] Master Brain successfully hardcoded with Market Schema.\n\n";

    // -------------------------------------------------------------
    // PHASE 2: INDIVIDUATION (CLONING THE SWARM)
    // -------------------------------------------------------------
    std::cout << "[SYSTEM] Cloning Master Brain into 4 separate Agents...\n";
    std::vector<SovereignAgent*> swarm;
    swarm.push_back(new SovereignAgent("Alpha_Bull", master, 111));
    swarm.push_back(new SovereignAgent("Beta_Bear", master, 222));
    swarm.push_back(new SovereignAgent("Gamma_Algo", master, 333));
    swarm.push_back(new SovereignAgent("Delta_Algo", master, 444));
    
    // Inject psychological drift
    std::cout << "[SYSTEM] Injecting contextual prompt states...\n";
    swarm[0]->observe("You love buying. You are aggressive."); 
    swarm[1]->observe("You love selling. You are cautious.");
    swarm[2]->observe("Logical algorithmic follow.");
    swarm[3]->observe("Fast execution. Do the opposite of logic sometimes.");

    // -------------------------------------------------------------
    // PHASE 3: SWARM EMERGENCE (THE SOCIAL TIMELINE)
    // -------------------------------------------------------------
    std::cout << "\n[SYSTEM] Deploying Swarm into the Sandbox...\n";
    std::cout << "---------------------------------------------------------\n";
    SocialTimeline timeline;
    std::mt19937 sim_gen(2026);

    for(int round = 1; round <= 7; round++) {
        std::cout << "\n--- TIMELINE ROUND " << round << " ---\n";
        
        // Environment dynamically shifts price
        int price = sim_gen() % 10;
        std::string event = "[EVENT: PRICE IS " + std::to_string(price) + "]\n[RESPONSE]\n";
        timeline.broadcast("SYSTEM", event);
        
        // All agents observe the event
        for(auto agent : swarm) {
            agent->observe(event);
        }

        // Random agents react based on internal weights + individualized memory drift
        int a1_idx = sim_gen() % 4;
        int a2_idx = (a1_idx + 1) % 4;
        std::string reply1 = swarm[a1_idx]->act(40);
        timeline.broadcast(swarm[a1_idx]->name, reply1);
        
        // Let the second agent "observe" the first agent's action before acting
        swarm[a2_idx]->observe(reply1);
        std::string reply2 = swarm[a2_idx]->act(40);
        timeline.broadcast(swarm[a2_idx]->name, reply2);
    }

    std::cout << "---------------------------------------------------------\n";
    for(auto a : swarm) delete a;
    std::cout << "[SUCCESS] Sovereign Swarm Offline Engine completed flawlessly.\n";
    return 0;
}

// -------------------------------------------------------------
// PHASE 4: THE CLOUD BRIDGE (C-INTERAFCE FOR PYTHON/HUGGINGFACE)
// -------------------------------------------------------------

extern "C" {
    // Brain Factory: Create a persistent 1.5M Parameter Master Block
    void* sovereign_init_master() {
        return (void*)new SovereignBlock();
    }

    // Agent Factory: Clone from Master into a named personality
    void* sovereign_init_agent(const char* name, void* master_ptr, int seed) {
        if (!master_ptr) return nullptr;
        SovereignBlock* master = (SovereignBlock*)master_ptr;
        return (void*)new SovereignAgent(std::string(name), *master, seed);
    }

    // Input: Feed text into the agent's neural state
    void sovereign_agent_observe(void* agent_ptr, const char* text) {
        if (!agent_ptr || !text) return;
        SovereignAgent* agent = (SovereignAgent*)agent_ptr;
        agent->observe(std::string(text));
    }

    // Output: Sample generated response from the VRAM/RAM state
    const char* sovereign_agent_act(void* agent_ptr, int max_chars, double temp) {
        if (!agent_ptr) return "";
        SovereignAgent* agent = (SovereignAgent*)agent_ptr;
        
        // Static buffer for result return (thread-safe for single HF space)
        static std::string result;
        result = agent->act(max_chars, temp);
        return result.c_str();
    }

    // Cleanup
    void sovereign_free_agent(void* agent_ptr) {
        if (agent_ptr) delete (SovereignAgent*)agent_ptr;
    }
}
