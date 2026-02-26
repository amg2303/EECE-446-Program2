# EECE-446 Program 2

## Project Overview
Network programming assignment for EECE-446 (Computer Networks).
A C++ TCP client that connects to a target HTTP server using POSIX socket APIs.

## Location
`C:\Users\Sam\Desktop\Networks Class\Program 2\EECE-446-Program2\`

## Files
- `program2.c++` — Main source file (TCP client skeleton)
- `README.md` — Repo readme

## Key Details
- Language: C++ (compiled on Linux/WSL2)
- Target server: `www.ecst.csuchico.edu:80`
- Core function: `lookup_and_connect(host, service)` — resolves hostname and establishes TCP connection
- Uses: `getaddrinfo`, `socket`, `connect` (POSIX networking)

## Build & Run (WSL2)
```bash
g++ -o program2 program2.c++
./program2
```

## Notes
- POSIX socket headers (`netdb.h`, `sys/socket.h`) require Linux/WSL2 — won't compile natively on Windows
- Use WSL2 path: `/mnt/c/Users/Sam/Desktop/Networks Class/Program 2/EECE-446-Program2/`
