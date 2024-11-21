#!/bin/bash

# Exit on any error
set -e

# Function to check if command executed successfully
check_status() {
    if [ $? -eq 0 ]; then
        echo "✓ $1 completed successfully"
    else
        echo "✗ Error during $1"
        exit 1
    fi
}

# Function to detect OS and install dependencies
install_dependencies() {
    if [ -f /etc/debian_version ]; then
        echo "Debian/Ubuntu-based system detected"
        sudo apt-get update
        sudo apt-get install -y \
            build-essential \
            cmake \
            gcc \
            libudev-dev \
            libnl-3-dev \
            libnl-route-3-dev \
            ninja-build \
            pkg-config \
            valgrind \
            python3-dev \
            python3-docutils \
            pandoc \
            git
    elif [ -f /etc/redhat-release ]; then
        echo "Red Hat-based system detected"
        sudo yum install -y \
            cmake \
            gcc \
            libudev-devel \
            libnl3-devel \
            ninja-build \
            pkg-config \
            valgrind \
            python3-devel \
            python3-docutils \
            pandoc \
            git
    else
        echo "Unsupported operating system"
        exit 1
    fi
    check_status "dependency installation"
}

# Main installation script
main() {
    echo "Starting RDMA-Core installation..."
    
    # Install dependencies
    echo "Installing dependencies..."
    install_dependencies
    
    # Create temporary directory for installation
    TEMP_DIR=$(mktemp -d)
    echo "Working in temporary directory: $TEMP_DIR"
    cd "$TEMP_DIR"
    
    # Clone repository
    echo "Cloning rdma-core repository..."
    git clone https://github.com/linux-rdma/rdma-core.git
    check_status "repository cloning"
    
    # Enter repository directory
    cd rdma-core
    
    # Checkout stable version
    echo "Checking out stable-v40..."
    git checkout stable-v40
    check_status "version checkout"
    
    # Build and install
    echo "Running cmake..."
    cmake .
    check_status "cmake configuration"
    
    echo "Installing rdma-core..."
    sudo make install
    check_status "make installation"
    
    # Verify installation
    if command -v rdma &> /dev/null; then
        echo "Installation completed successfully!"
        rdma --version
    else
        echo "Installation seems to have failed. 'rdma' command not found."
        exit 1
    fi
    
    # Clean up
    echo "Cleaning up..."
    cd
    rm -rf "$TEMP_DIR"
    check_status "cleanup"
}

# Run main function
main

echo "RDMA-Core installation process completed!"

sudo apt install make cmake g++ gcc libnuma-dev libgflags-dev numactl
sudo modprobe ib_uverbs
sudo modprobe mlx5_ib