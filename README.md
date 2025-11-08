# System_Monitor_Tool
Linux System Monitor Tool (C++ + Ncurses)
 Overview :
This project is a terminal-based system monitor built in C++ using the Linux /proc filesystem and ncurses library.
It displays real-time CPU usage, memory information, and running processes, and allows managing them (kill, suspend, resume, view details).

Features :
Real-time process monitoring (auto-refresh)
Displays:
PID
USER
CPU usage (%)
Memory usage (MB)
Process name
Kill, Suspend, and Resume processes
Sort by PID, CPU, or Memory
View detailed process info
Ncurses-based colorful UI with scroll
Automatic live refresh every 200ms

Requirements:
Run these commands before compiling:
sudo apt update
sudo apt install g++ make libncurses5-dev libncursesw5-dev

How to Compile and Run:
# Step 1: Go to project folder
cd ~/your_project_folder
# Step 2: Compile the program
make
# Step 3: Run the program (sudo required)
make run
# Step 4: Clean build files
make clean
