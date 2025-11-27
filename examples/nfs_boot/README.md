Set ip address for usb gadget, i'm doing it via /etc/network/interfaces:
```txt
# STM32MP135F-DK2 Board
allow-hotplug enxf8dc7a000001
iface enxf8dc7a000001 inet static
    address 192.168.7.1/24
    gateway 192.168.7.1
    hwaddress ether f8:dc:7a:00:00:01
```

Bind nfs-kernel-server to host ip:
```
❯ cat /etc/default/nfs-kernel-server
# Runtime priority of server (see nice(1))
RPCNFSDPRIORITY=0

# Do you want to start the svcgssd daemon? It is only required for Kerberos
# exports. Valid alternatives are "yes" and "no"; the default is "no".
NEED_SVCGSSD=""
RPCMOUNTDOPTS="--port 20048 --bind 192.168.7.1"
```

Share directory in nfs-kernel-server:
```
❯ cat /etc/exports
/srv/nfs/ 192.168.7.2(rw,sync,no_root_squash,no_subtree_check)
```

Once buildroot finish up building rootfs you can extract it to nfs share:
```
sudo tar xvf third_party/buildroot/output/images/rootfs.tar -C /srv/nfs/
```
