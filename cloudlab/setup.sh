#!/bin/bash

# Function to check if running as root
check_root() {
    if [ "$EUID" -ne 0 ]; then 
        echo "Please run as root"
        exit 1
    fi
}

# Function to check if a port is in use
check_port_in_use() {
    local port=$1
    # Check if port is actively in use using ss (modern replacement for netstat)
    if ss -tuln | grep -q ":${port}"; then
        echo "ERROR: Port ${port} is already in use"
        echo "Active connection details:"
        ss -tuln | grep ":${port}"
        return 1
    fi
    # Additional check for processes binding to the port
    if lsof -i :${port} >/dev/null 2>&1; then
        echo "ERROR: Port ${port} is bound to a process"
        echo "Process details:"
        lsof -i :${port}
        return 1
    fi
    return 0
}

# Function to check and configure huge pages
check_huge_pages() {
    # Get list of NUMA nodes
    numa_nodes=$(numactl --hardware | grep "available:" | cut -d " " -f 2)
    
    for node in $(seq 0 $((numa_nodes-1))); do
        current_pages=$(cat /sys/devices/system/node/node${node}/hugepages/hugepages-2048kB/nr_hugepages)
        if [ "$current_pages" -lt 1024 ]; then
            echo "Node ${node} has insufficient huge pages (${current_pages}). Setting to 1024..."
            echo 1024 > /sys/devices/system/node/node${node}/hugepages/hugepages-2048kB/nr_hugepages
        else
            echo "Node ${node} has sufficient huge pages (${current_pages})"
        fi
    done
}

# Function to check and configure SHM limits
check_shm_limits() {
    # Check current SHM limits
    current_shmmax=$(cat /proc/sys/kernel/shmmax)
    current_shmall=$(cat /proc/sys/kernel/shmall)
    
    # Set high limits if needed
    echo "kernel.shmmax=18446744073709551615" >> /etc/sysctl.conf
    echo "kernel.shmall=18446744073709551615" >> /etc/sysctl.conf
    sysctl -p
}


#Check if a port is in use using ss only
check_port() {
    local port=$1
    
    # Check using ss
    if ss -tuln | grep -q ":${port}"; then
        echo "WARNING: Port ${port} is already in use:"
        ss -tuln | grep ":${port}"
        return 1
    fi
    
    echo "Port ${port} is available"
    return 0
}
check_root

echo "Checking and configuring huge pages..."
check_huge_pages

echo "Checking and configuring SHM limits..."
check_shm_limits

echo "Checking 4 eRPC required ports..."
base_port=31850
for offset in {0..3}; do  # Check exactly 4 ports
    port=$((base_port + offset))
    check_port $port
done


echo "Configuration complete. Please verify the changes are as expected."

