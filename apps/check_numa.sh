for node in $(ls /sys/devices/system/node/ | grep node); do
    echo "=== $node ==="
    # Count physical cores (unique core IDs) in this NUMA node
    echo "Physical cores:"
    cat /sys/devices/system/node/$node/cpulist | tr ',' '\n' | \
        xargs -I{} cat /sys/devices/system/cpu/cpu{}/topology/core_id | sort -n | uniq | wc -l
    
    # Count logical cores (total CPU threads) in this NUMA node
    echo "Logical cores (threads):"
    cat /sys/devices/system/node/$node/cpulist | tr ',' '\n' | wc -l
    
    # Show CPU list for this node
    echo "CPU list:"
    cat /sys/devices/system/node/$node/cpulist
done