#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <ctime>

struct DataPoint {
    float x, y, z;
};

// --- SYSTEM UTILS ---
float relu(float x) { return x > 0 ? x : 0; }
float relu_derivative(float x) { return x > 0 ? 1.0f : 0.0f; }

void softmax3(float* input, float* output) {
    float max_val = input[0];
    if (input[1] > max_val) max_val = input[1];
    if (input[2] > max_val) max_val = input[2];
    float sum = 0.0f;
    output[0] = std::exp(input[0] - max_val);
    output[1] = std::exp(input[1] - max_val);
    output[2] = std::exp(input[2] - max_val);
    sum = output[0] + output[1] + output[2];
    output[0] /= sum; output[1] /= sum; output[2] /= sum;
}

// --- SOVEREIGN BLOCK v2.3 (STABILIZED) ---
struct SovereignBlock {
    static const int HEADS = 3;

    // Parameters
    float wA[HEADS][3], wB[HEADS][3], wS[HEADS][3];
    float w1[8][3], b1[8];
    float w_feat[8], b_feat;

    SovereignBlock() {
        for (int k = 0; k < HEADS; k++) {
            for (int j = 0; j < 3; j++) {
                wA[k][j] = ((rand() % 100) / 100.0f) - 0.5f;
                wB[k][j] = ((rand() % 100) / 100.0f) - 0.5f;
                wS[k][j] = ((rand() % 100) / 100.0f) - 0.5f;
            }
        }
        for (int i = 0; i < 8; i++) {
            b1[i] = 0.1f;
            for (int j = 0; j < 3; j++) w1[i][j] = ((rand() % 100) / 100.0f) - 0.5f;
            w_feat[i] = ((rand() % 100) / 100.0f) - 0.5f;
        }
        b_feat = 0.0f;
    }

    // Dynamic state
    float in_cache[3], f1[HEADS], f2[HEADS], inter[HEADS], score[HEADS], alpha[HEADS], y_att;
    float z1[8], a1[8], y_feat_raw;

    void forward(float* in, float* out) {
        for(int i=0; i<3; i++) in_cache[i] = in[i];
        for (int k = 0; k < HEADS; k++) {
            f1[k] = 0; f2[k] = 0; score[k] = 0;
            for(int j=0; j<3; j++) {
                f1[k] += wA[k][j] * in[j]; f2[k] += wB[k][j] * in[j];
                score[k] += wS[k][j] * in[j];
            }
            inter[k] = f1[k] * f2[k];
        }
        softmax3(score, alpha);
        y_att = 0;
        for (int k = 0; k < HEADS; k++) y_att += alpha[k] * inter[k];

        float ff_in[3] = {in[0], in[1], y_att};
        for (int i = 0; i < 8; i++) {
            z1[i] = b1[i] + ff_in[0]*w1[i][0] + ff_in[1]*w1[i][1] + ff_in[2]*w1[i][2];
            a1[i] = relu(z1[i]);
        }
        y_feat_raw = b_feat;
        for (int i = 0; i < 8; i++) y_feat_raw += a1[i] * w_feat[i];

        // Signal Stabilization: x / (1 + |x|)
        float y_squashed = y_feat_raw / (1.0f + std::abs(y_feat_raw));
        out[0] = in[0]; out[1] = in[1]; out[2] = y_squashed;
    }

    void train(float* in, float dL_dout, float* dL_din, float lr) {
        // Derivative of x/(1+|x|) is 1 / (1+|x|)^2
        float denom = 1.0f + std::abs(y_feat_raw);
        float dL_dy_raw = dL_dout / (denom * denom);

        float dL_da1[8], dL_dw_feat[8];
        for (int i = 0; i < 8; i++) {
            dL_dw_feat[i] = dL_dy_raw * a1[i];
            dL_da1[i] = dL_dy_raw * w_feat[i];
            w_feat[i] += lr * dL_dw_feat[i]; // <--- In-place update bug
        }
        b_feat += lr * dL_dy_raw;

        float dL_dff_in[3] = {0,0,0};
        for (int i = 0; i < 8; i++) {
            float dL_dz1 = dL_da1[i] * relu_derivative(z1[i]);
            b1[i] += lr * dL_dz1;
            float ff_in[3] = {in[0], in[1], y_att};
            for (int j = 0; j < 3; j++) {
                w1[i][j] += lr * dL_dz1 * ff_in[j]; // <--- In-place update bug
                dL_dff_in[j] += dL_dz1 * w1[i][j];
            }
        }

        float dL_dy_att = dL_dff_in[2];
        float dL_din_total[3] = {dL_dff_in[0], dL_dff_in[1], 0};
        for (int k = 0; k < HEADS; k++) {
            float dL_dint_k = dL_dy_att * alpha[k];
            float dL_dS_k = dL_dy_att * alpha[k] * (inter[k] - y_att);
            float dL_df1 = dL_dint_k * f2[k], dL_df2 = dL_dint_k * f1[k];
            for (int j = 0; j < 3; j++) {
                wA[k][j] += lr * dL_df1 * in[j];
                wB[k][j] += lr * dL_df2 * in[j];
                wS[k][j] += lr * dL_dS_k * in[j];
                dL_din_total[j] += dL_df1 * wA[k][j] + dL_df2 * wB[k][j] + dL_dS_k * wS[k][j];
            }
        }
        for(int j=0;j<3;j++) dL_din[j] = dL_din_total[j];
    }
};

int main() {
    SovereignBlock b1, b2;
    float w_final[3] = {0.1f, 0.1f, 0.5f}, b_final = 0.0f;
    std::vector<DataPoint> data;
    for (float i = -1.0f; i <= 1.0f; i += 0.4f) {
        if (std::abs(i) < 0.1f) continue;
        for (float j = -1.0f; j <= 1.0f; j += 0.4f) {
            data.push_back({i, j, (i > 0) ? (i * i) : (i * j)});
        }
    }

    std::cout << "--- STABLE SOVEREIGN v2.3 (SIGNAL CONTROL) ---\n\n";

    float lr = 0.015f;
    for (int epoch = 0; epoch < 10001; epoch++) {
        float total_loss = 0;
        for (auto &p : data) {
            float in_b1[3] = {p.x, p.y, 0}, h1[3], tmp[3], h2[3];
            b1.forward(in_b1, h1);
            b2.forward(h1, tmp);
            for(int i=0; i<3; i++) h2[i] = tmp[i] + h1[i]; // Residual

            float pred = b_final;
            for(int i=0; i<3; i++) pred += h2[i] * w_final[i];
            float error = p.z - pred;
            total_loss += error * error;

            float dL_dh2[3];
            for(int i=0; i<3; i++) {
                float d_wf = error * h2[i];
                dL_dh2[i] = error * w_final[i];
                w_final[i] += lr * d_wf;
            }
            b_final += lr * error;

            float dL_dh1_back[3], dL_dh1_res[3], dL_din_b1[3];
            b2.train(h1, dL_dh2[2], dL_dh1_back, lr);
            for(int i=0; i<3; i++) dL_dh1_res[i] = dL_dh2[i] + dL_dh1_back[i];
            b1.train(in_b1, dL_dh1_res[2], dL_din_b1, lr);
        }
        if (epoch % 2000 == 0) std::cout << "Epoch " << std::setw(5) << epoch << " | MSE: " << total_loss / data.size() << "\n";
    }

    std::cout << "\n--- FINAL CHECK ---\n";
    float tx = 0.5f, ty = 0.5f, h1[3], h2[3], tmp[3], in[3]={tx, ty, 0};
    b1.forward(in, h1); b2.forward(h1, tmp);
    for(int i=0; i<3; i++) h2[i] = tmp[i] + h1[i];
    float p = b_final; for(int i=0; i<3; i++) p += h2[i] * w_final[i];
    std::cout << "Input (0.5, 0.5) | Pred: " << p << " | Goal: 0.25\n";

    return 0;
}
