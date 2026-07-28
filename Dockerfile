# Snooker Player 3.0 - Dockerfile
# Build multi-stage : compilation + image legere finale

# --- Stage 1 : Build ---
FROM ubuntu:24.04 AS builder

# Eviter les interactions pendant apt
ENV DEBIAN_FRONTEND=noninteractive

# Installer les outils de compilation
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    && rm -rf /var/lib/apt/lists/*

# Copier les sources
WORKDIR /src
COPY . .

# Compiler
RUN mkdir -p build && cd build \
    && cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DSNOOKER_BUILD_TESTS=ON \
        -DSNOOKER_BUILD_SERVER=ON \
    && cmake --build . --parallel

# Lancer les tests
RUN cd build && ctest --output-on-failure

# --- Stage 2 : Image finale (legere) ---
FROM ubuntu:24.04 AS runtime

# Copier les binaires depuis le stage de build
COPY --from=builder /src/build/snooker_sim    /usr/local/bin/snooker_sim
COPY --from=builder /src/build/snooker_server /usr/local/bin/snooker_server

# Port du serveur API
EXPOSE 8080

# Variables d'environnement
ENV SNOOKER_PORT=8080

# Point d'entree par defaut : serveur API
ENTRYPOINT ["/usr/local/bin/snooker_server"]
CMD ["8080"]

# --- Labels ---
LABEL maintainer="Snooker Player 3.0"
LABEL description="Snooker Player 3.0 - Moteur snooker + API REST + Systeme cameras"
LABEL version="3.0"

# Utilisation :
#   docker build -t snooker-player .
#   docker run -p 8080:8080 snooker-player
#   docker run snooker-player /usr/local/bin/snooker_sim
