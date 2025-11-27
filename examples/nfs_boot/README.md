## Configure NIC

Set a static IP for the USB gadget interface using `/etc/network/interfaces`:
```txt
# STM32MP135F-DK2 Board USB gadget
allow-hotplug enxf8dc7a000001
iface enxf8dc7a000001 inet static
    address 192.168.7.1/24
    gateway 192.168.7.1
    hwaddress ether f8:dc:7a:00:00:01
```

## Configure NFS server

Install the NFS server package:
```bash
apt-get install nfs-kernel-server
```

Bind the NFS server to the host-side USB IP:
```txt
# /etc/default/nfs-kernel-server
RPCNFSDPRIORITY=0
NEED_SVCGSSD=""
RPCMOUNTDOPTS="--port 20048 --bind 192.168.7.1"
```

Export the NFS share to the board (client IP `192.168.7.2`):
```txt
# /etc/exports
/srv/nfs/ 192.168.7.2(rw,sync,no_root_squash,no_subtree_check)
```

After Buildroot finishes, unpack the generated rootfs into the NFS directory:
```bash
sudo tar xvf third_party/buildroot/output/images/rootfs.tar -C /srv/nfs/
```
