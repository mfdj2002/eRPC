#!/bin/bash

#Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Detect OS
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
else
    echo "Cannot detect OS"
    exit 1
fi

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "Please run as root"
    exit 1
fi

echo "=== Installing DPDK and dependencies ==="

# Install dependencies based on OS
install_dependencies() {
    case $OS in
        "ubuntu"|"debian")
            apt-get update
            apt-get install -y build-essential libnuma-dev python3 python3-pip pkg-config \
                             libpcap-dev meson ninja-build libbsd-dev libelf-dev \
                             libjson-c-dev rdma-core ibverbs-utils perftest \
                             python3-pyelftools python3-setuptools
            ;;
        "centos"|"rhel"|"fedora")
            dnf groupinstall -y "Development Tools"
            dnf install -y numactl-devel python3 python3-pip pkgconfig libpcap-devel \
                         meson ninja-build libbsd-devel elfutils-libelf-devel \
                         json-c-devel rdma-core libibverbs perftest \
                         python3-pyelftools python3-setuptools
            ;;
        *)
            echo "Unsupported OS: $OS"
            exit 1
            ;;
    esac

    # Install pyelftools using pip as backup
    pip3 install pyelftools

    # Verify elftools installation
    if python3 -c "import elftools" 2>/dev/null; then
        echo -e "${GREEN}✓ elftools is installed correctly${NC}"
    else
        echo -e "${RED}✗ Failed to install elftools${NC}"
        exit 1
    fi
}


# Install DPDK
install_dpdk() {
    echo "=== Installing DPDK ==="
    
    # Use the latest stable version
    DPDK_VERSION="21.11"
    DPDK_DIR="dpdk-$DPDK_VERSION"
    
    cd /tmp
    if [ ! -f "${DPDK_DIR}.tar.xz" ]; then
        wget "https://fast.dpdk.org/rel/dpdk-${DPDK_VERSION}.tar.xz"
    fi
    
    tar xf "${DPDK_DIR}.tar.xz"
    cd "$DPDK_DIR"
    
    # Build using meson and ninja
    meson build
    cd build
    ninja
    ninja install
    
    # Update ldconfig
    ldconfig
    
    # Add DPDK binaries to PATH
    if ! grep -q "DPDK" /etc/profile.d/dpdk.sh 2>/dev/null; then
        echo 'export PATH=$PATH:/usr/local/bin' > /etc/profile.d/dpdk.sh
        chmod +x /etc/profile.d/dpdk.sh
    fi
}

# Configure system for DPDK
configure_system() {
    echo "=== Configuring System for DPDK ==="
    
    # not required for mlx nics?
    # Load required modules 
    # modprobe uio
    # modprobe uio_pci_generic
    
    # # Make modules load at boot
    # if ! grep -q "uio" /etc/modules 2>/dev/null; then
    #     echo "uio" >> /etc/modules
    #     echo "uio_pci_generic" >> /etc/modules
    # fi
    
    # Enable IOMMU if available
    if [ -d "/sys/class/iommu" ]; then
        if ! grep -q "intel_iommu=on" /etc/default/grub; then
            sed -i 's/GRUB_CMDLINE_LINUX="/GRUB_CMDLINE_LINUX="intel_iommu=on /' /etc/default/grub
            case $OS in
                "ubuntu"|"debian")
                    update-grub
                    ;;
                "centos"|"rhel"|"fedora")
                    grub2-mkconfig -o /boot/grub2/grub.cfg
                    ;;
            esac
            echo -e "${YELLOW}System needs to be rebooted to enable IOMMU${NC}"
        fi
    fi
}

# Main installation process
echo "Installing dependencies..."
install_dependencies

echo "Installing DPDK..."
install_dpdk

echo "Configuring system..."
configure_system

echo -e "\n${GREEN}=== DPDK Installation Complete ===${NC}"
echo -e "${YELLOW}Note: You may need to reboot your system for all changes to take effect${NC}"
echo "
Post-installation steps:
1. Reboot the system if IOMMU was enabled
2. Run 'dpdk-devbind --status' to see available network interfaces
3. To bind a network interface to DPDK:
   dpdk-devbind -b uio_pci_generic <PCI_ID>
   (replace <PCI_ID> with the PCI ID of your network card)
"

