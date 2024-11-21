on cloudlab, use any node that has mellanox nic (preferably with nvme SSD)
first install rdma-core and dpdk 21.11 as usual, documented in the original README.md
then change the default parameter phy_port in 
Rpc(Nexus *nexus, void *context, uint8_t rpc_id, sm_handler_t sm_handler,
    uint8_t phy_port = 0);

into the actual NIC that's active and that your ssh connection is not attached to. 
This is usually the same as the index of the row of the NIC when you run dpdk-devbind.py --status.

For example:
$ dpdk-devbind.py --status

Network devices using kernel driver
===================================
0000:01:00.0 'MT27800 Family [ConnectX-5] 1017' if=eno33 drv=mlx5_core unused=vfio-pci,uio_pci_generic *Active*
0000:01:00.1 'MT27800 Family [ConnectX-5] 1017' if=eno34 drv=mlx5_core unused=vfio-pci,uio_pci_generic 
0000:41:00.0 'MT27800 Family [ConnectX-5] 1017' if=ens1f0 drv=mlx5_core unused=vfio-pci,uio_pci_generic *Active*
0000:41:00.1 'MT27800 Family [ConnectX-5] 1017' if=ens1f1 drv=mlx5_core unused=vfio-pci,uio_pci_generic

in this case you find that setting phy_port as 2 connects you to the nic on 0000:41:00.0, which is the one you want to use, 
since ssh uses the phy_port 0


to run the hello world, you also don't need to make under that directory but use the build/hello_server(or client)


the helloworld example sometimes also couldn't get the right environment variable LD_LIBRARY_PATH and LIBRARY PATH, so 
it is recommended to pass in the LD_LIBRARY_PATH when you run the helloworld exmaples.