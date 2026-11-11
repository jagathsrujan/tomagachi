#!/bin/sh
# Builds the host preview from the sketch's own UI headers, renders every screen to
# preview/*.png (240x240 with the round mask), and prints the layout checks and load figures.
set -e
cd "$(dirname "$0")"
mkdir -p /tmp/agent_link_preview_build
g++ -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter host.cpp -o /tmp/agent_link_preview_build/host
/tmp/agent_link_preview_build/host /tmp/agent_link_preview_build > report.txt || true
cat report.txt
python3 -I topng.py /tmp/agent_link_preview_build .
