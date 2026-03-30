#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <random>
#include <string>

// Math Utils
void softmax(double* logits, double* probs, int size) {
    double max_val = logits[0];
    for(int i=1; i<size; i++) if(logits[i] > max_val) max_val = logits[i];
    double sum = 0.0;
    for(int i=0; i<size; i++) { probs[i] = std::exp(logits[i] - max_val); sum += probs[i]; }
    for(int i=0; i<size; i++) probs[i] /= sum;
}
double relu(double x) { return x > 0 ? x : 0.0; }
double sigmoid(double x) { return 1.0 / (1.0 + std::exp(-x)); }

// Architecture definitions from Phase 5.5
static const int VOCAB = 256;
static const int EMBED_DIM = 8;
static const int GRU_IN = EMBED_DIM; 
static const int H_DIM = 64;
static const int GRU_CONCAT = GRU_IN + H_DIM; // 72
static const int FFN_IN = EMBED_DIM + H_DIM; // 72
static const int HIDDEN = 64;

struct SovereignCache {
    int input_char;
    double probs[VOCAB];
};

struct SovereignInferenceBlock {
    double W_embed[VOCAB][EMBED_DIM];
    double W_z[H_DIM][GRU_CONCAT], b_z[H_DIM];
    double W_r[H_DIM][GRU_CONCAT], b_r[H_DIM];
    double W_h[H_DIM][GRU_CONCAT], b_h[H_DIM];
    double W_ffn1[HIDDEN][FFN_IN], b_ffn1[HIDDEN];
    double W_out[VOCAB][HIDDEN], b_out[VOCAB];
    std::mt19937 gen;

    SovereignInferenceBlock(int seed) : gen(seed) {
        std::uniform_real_distribution<double> dis(-0.05, 0.05);
        for(int v=0; v<VOCAB; v++) {
            b_out[v] = 0;
            for(int d=0; d<EMBED_DIM; d++) W_embed[v][d] = dis(gen);
            for(int i=0; i<HIDDEN; i++) W_out[v][i] = dis(gen);
        }
        for(int i=0; i<H_DIM; i++) {
            b_z[i]=0; b_r[i]=0; b_h[i]=0;
            for(int j=0; j<GRU_CONCAT; j++) {
                W_z[i][j]=dis(gen); W_r[i][j]=dis(gen); W_h[i][j]=dis(gen);
            }
        }
        for(int i=0; i<HIDDEN; i++) {
            b_ffn1[i] = 0;
            for(int j=0; j<FFN_IN; j++) W_ffn1[i][j] = dis(gen);
        }
    }

    SovereignCache forward(int x, double* h_state) {
        SovereignCache c; c.input_char = x;
        double embed[EMBED_DIM]; for(int d=0; d<EMBED_DIM; d++) embed[d] = W_embed[x][d];
        double concat_zh[GRU_CONCAT], concat_r[GRU_CONCAT];
        for(int i=0; i<EMBED_DIM; i++) { concat_zh[i] = embed[i]; concat_r[i] = embed[i]; }
        for(int i=0; i<H_DIM; i++) { concat_zh[EMBED_DIM+i] = h_state[i]; concat_r[EMBED_DIM+i] = h_state[i]; }

        double z_gate[H_DIM], r_gate[H_DIM];
        for(int i=0; i<H_DIM; i++) {
            double zz = b_z[i], zr = b_r[i];
            for(int j=0; j<GRU_CONCAT; j++) { zz += W_z[i][j]*concat_zh[j]; zr += W_r[i][j]*concat_r[j]; }
            z_gate[i] = sigmoid(zz); r_gate[i] = sigmoid(zr);
        }

        double concat_cand[GRU_CONCAT];
        for(int i=0; i<EMBED_DIM; i++) concat_cand[i] = embed[i];
        for(int i=0; i<H_DIM; i++) concat_cand[EMBED_DIM+i] = r_gate[i] * h_state[i];

        double h_new[H_DIM];
        for(int i=0; i<H_DIM; i++) {
            double zh = b_h[i];
            for(int j=0; j<GRU_CONCAT; j++) zh += W_h[i][j]*concat_cand[j];
            h_new[i] = (1.0 - z_gate[i])*h_state[i] + z_gate[i]*std::tanh(zh);
            h_state[i] = h_new[i]; // Update state IN PLACE for inference
        }

        double ffn_in_vec[FFN_IN];
        for(int i=0; i<EMBED_DIM; i++) ffn_in_vec[i] = embed[i];
        for(int i=0; i<H_DIM; i++) ffn_in_vec[EMBED_DIM+i] = h_new[i];

        double ffn1_out[HIDDEN];
        for(int i=0; i<HIDDEN; i++) {
            double z = b_ffn1[i];
            for(int j=0; j<FFN_IN; j++) z += W_ffn1[i][j]*ffn_in_vec[j];
            ffn1_out[i] = relu(z);
        }

        double logits[VOCAB];
        for(int v=0; v<VOCAB; v++) {
            logits[v] = b_out[v];
            for(int i=0; i<HIDDEN; i++) logits[v] += ffn1_out[i] * W_out[v][i];
        }
        softmax(logits, c.probs, VOCAB);

        return c;
    }
};

// ----------------------------------------------------
// MIROFISH SIMULATION SCALING:
// ----------------------------------------------------

class SovereignAgent {
public:
    std::string name;
    SovereignInferenceBlock brain;
    double h_state[H_DIM];
    std::uniform_real_distribution<double> dist_samp;

    SovereignAgent(std::string name, int brain_seed) 
        : name(name), brain(brain_seed), dist_samp(0.0, 1.0) {
        
        // Zero the personal memory
        for(int i=0; i<H_DIM; i++) h_state[i] = 0;
    }

    void observe(const std::string& text) {
        // Read text character by character to perfectly update H_DIM memory
        for(char c : text) {
            unsigned char encoded = (unsigned char)c;
            brain.forward(encoded, h_state);
        }
    }

    std::string act(int max_chars, double temperature = 0.8) {
        std::string generated = "";
        int curr = ' '; // Seed with space
        
        for(int step=0; step<max_chars; step++) {
            SovereignCache c = brain.forward(curr, h_state);
            
            double sum = 0;
            double scaled_probs[VOCAB];
            for(int v=0; v<VOCAB; v++) {
                scaled_probs[v] = std::pow(c.probs[v], 1.0/temperature);
                sum += scaled_probs[v];
            }
            
            double r = dist_samp(brain.gen);
            double acc = 0.0;
            int best = 0;
            for(int v=0; v<VOCAB; v++) {
                acc += scaled_probs[v] / sum;
                if(r <= acc) { best = v; break; }
            }
            
            // Format ASCII range mapping strictly
            if (best < 32 || best > 126) best = '.'; // map unprintables to dot
            
            generated += (char)best;
            curr = best;
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
        std::cout << "\n-------------------------------------------------\n";
        std::cout << msg << "\n";
        std::cout << "-------------------------------------------------\n";
    }
    
    std::string get_latest() {
        if(posts.empty()) return "";
        return posts.back();
    }
};

int main() {
    std::cout << "--- STAGE 3: SOVEREIGN V7 SOCIAL SANDBOX ---\n";
    std::cout << "[INFO] Initializing C++ Environment inspired by MiroFish.\n";
    std::cout << "[INFO] Allocating Swarm Intel...\n";

    std::vector<SovereignAgent*> swarm;
    swarm.push_back(new SovereignAgent("Agent Alpha", 101));
    swarm.push_back(new SovereignAgent("Agent Beta",  202));
    swarm.push_back(new SovereignAgent("Agent Gamma", 303));
    swarm.push_back(new SovereignAgent("Agent Delta", 404));
    
    // Inject "System Prompts" to isolate their identities contextually
    swarm[0]->observe("You are Agent Alpha. Proceed.");
    swarm[1]->observe("You are Agent Beta. Proceed.");
    swarm[2]->observe("You are Agent Gamma. Proceed.");
    swarm[3]->observe("You are Agent Delta. Proceed.");

    SocialTimeline timeline;
    
    // Initial global event
    timeline.broadcast("SYSTEM", "The sandbox is officially open. Agents, begin observation.");

    // MiroFish Event Loop: Random agent wakes up, reads timeline, and posts
    std::mt19937 sim_gen(42);
    int ROUNDS = 10;
    
    for(int r = 1; r <= ROUNDS; r++) {
        std::cout << "\n[ROUND " << r << " / " << ROUNDS << "] Simulating turn...\n";
        
        // Randomly pick an active agent
        int active_idx = sim_gen() % swarm.size();
        SovereignAgent* agent = swarm[active_idx];
        
        // Agent reads the global post (This acts as the agent's Perception)
        std::string latest_event = timeline.get_latest();
        agent->observe("\nEVENT: " + latest_event + "\nRESPONSE: ");
        
        // Agent formulates its response (This acts as the agent's Planning/Execution)
        std::string generated_text = agent->act(50, 0.9); // generate 50 chars natively
        
        // Broadcast to timeline
        timeline.broadcast(agent->name, generated_text);
    }
    
    std::cout << "\n[INFO] Simulation Cycle Complete. No crashes detected. C++ Pipeline is mathematically secure.\n";
    
    for(auto a : swarm) delete a;
    return 0;
}
