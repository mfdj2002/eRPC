#!/bin/bash

set -x

# Function to check if running as root
check_root() {
    if [ "$EUID" -ne 0 ]; then 
        echo "Please run as root"
        exit 1
    fi
}

# Function to check and configure huge pages on node 0
configure_huge_pages() {
    bash -c "echo 2048 > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages"
    mkdir /mnt/huge
    mount -t hugetlbfs nodev /mnt/huge
}

# Function to check and configure SHM limits
configure_shm_limits() {
    # Check current SHM limits
    current_shmmax=$(cat /proc/sys/kernel/shmmax)
    current_shmall=$(cat /proc/sys/kernel/shmall)
    
    # Set high limits if needed
    echo "kernel.shmmax=18446744073709551615" >> /etc/sysctl.conf
    echo "kernel.shmall=18446744073709551615" >> /etc/sysctl.conf
    sysctl -p
}

check_root

bash install-rdma.sh
bash install-dpdk.sh

configure_huge_pages
configure_shm_limits

echo "Configuration complete. Please verify the changes are as expected."

