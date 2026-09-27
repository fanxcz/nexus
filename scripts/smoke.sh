#!/usr/bin/env bash
set -euo pipefail
./build/nexus check examples/hello.nx
./build/nexus run examples/hello.nx
./build/nexus run examples/factorial.nx
./build/nexus run examples/control_flow.nx
