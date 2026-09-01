# ─── Render.com Docker Deployment for ChessVerse (C/C++) ───
FROM gcc:12 AS builder

WORKDIR /app
COPY . .

# Compile C++ backend with pthread on Linux
RUN g++ -std=c++17 -O2 -o ChessVerse backend/main.cpp backend/chess/Board.cpp -lpthread

# Lightweight runtime stage
FROM ubuntu:22.04

WORKDIR /app
RUN apt-get update && apt-get install -y libstdc++6 ca-certificates && rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/ChessVerse .
COPY --from=builder /app/frontend ./frontend
COPY --from=builder /app/data ./data

# Render sets PORT dynamically (defaults to 8080 or 10000)
ENV PORT=8080
EXPOSE 8080

CMD ["./ChessVerse"]
