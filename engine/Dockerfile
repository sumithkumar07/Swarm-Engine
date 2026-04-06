# Stage 1: Build the Optimized swarm C++ Engine
FROM python:3.11-slim as builder

# Install C++ compiler
RUN apt-get update && apt-get install -y g++ && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy Engine Source
COPY swarm_v10_swarm.cpp .
COPY tokenizer.h .

# Compile the swarm swarm Native Engine (.so)
# Optimized for high-throughput interaction on Cloud CPUs
RUN g++ -O3 -fPIC -shared swarm_v10_swarm.cpp -o libswarm.so

# Stage 2: Final Production Image
FROM python:3.11-slim

# Create a non-root user (Hugging Face best practice)
RUN useradd -m -u 1000 user
USER user
ENV PATH="/home/user/.local/bin:$PATH"

WORKDIR /app

# Copy built engine components
COPY --from=builder /app/libswarm.so .
COPY --chown=user vocab.txt .
COPY --chown=user master_brain.bin .
COPY --chown=user personalities.json .

# Copy Python Bridge and requirements
COPY --chown=user requirements.txt .
COPY --chown=user main.py .

# Install dependencies (FastAPI, Uvicorn, etc.)
RUN pip install --no-cache-dir --upgrade -r requirements.txt

# Start the swarm swarm Swarm on Hugging Face
EXPOSE 7860
CMD ["python", "main.py"]
