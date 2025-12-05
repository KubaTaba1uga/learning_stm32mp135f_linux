Let's start from bus, bus role is to connect a device and driver into a pair. When we insmod my_bus module we see that in `/sys/bus` appears new directory `my_bus`.
```bash
# ls /sys/bus/my_bus/
devices            drivers_autoprobe  uevent
drivers            drivers_probe
```

Each device registered into a bus will be visible under `devices` directory.
Each driver regstered into bus will be visible under `drivers` directory.

Underneath the `bus` abstract is represendted in linux kernel by `struct bus_type`:
```c
struct bus_type {
	const char		*name;
	const char		*dev_name;
	const struct attribute_group **bus_groups;
	const struct attribute_group **dev_groups;
	const struct attribute_group **drv_groups;

	int (*match)(struct device *dev, struct device_driver *drv);
	int (*uevent)(const struct device *dev, struct kobj_uevent_env *env);
	int (*probe)(struct device *dev);
	void (*sync_state)(struct device *dev);
	void (*remove)(struct device *dev);
	void (*shutdown)(struct device *dev);

	int (*online)(struct device *dev);
	int (*offline)(struct device *dev);

	int (*suspend)(struct device *dev, pm_message_t state);
	int (*resume)(struct device *dev);

	int (*num_vf)(struct device *dev);

	int (*dma_configure)(struct device *dev);
	void (*dma_cleanup)(struct device *dev);

	const struct dev_pm_ops *pm;

	const struct iommu_ops *iommu_ops;

	bool need_parent_lock;
};
```

Once we
```
# insmod my_bus.ko
[   28.366759] my_bus: loading out-of-tree module taints kernel.
[   28.371918] Inserted: my_bus_module_init
# ls /sys/bus/
amba            genpd           my_bus          serial-base
bcma            gpio            nvmem           serio
cec             hid             pci             soc
clockevents     host1x          pci-epf         spi
clocksource     host1x-context  pci_express     spmi
container       i2c             platform        sunxi-rsb
cpu             iio             rpmsg           tee
dp-aux          mdio_bus        scmi_protocol   ulpi
edac            mipi-dsi        scsi            usb
event_source    mmc             sdio            virtio
gadget          mmc_rpmb        serial          workqueue
# ls /sys/bus/my_bus/
devices            drivers_autoprobe  uevent
drivers            drivers_probe
# ls /sys/bus/my_bus/devices/
# ls /sys/bus/my_bus/drivers
# insmod my_driver.ko
[   59.544775] Inserted: my_driver_module_init
# ls /sys/bus/my_bus/drivers
my_driver
# ls /sys/bus/my_bus/drivers/my_driver/
bind    uevent  unbind
# insmod my_device.ko
[   71.847567] my_device_module_init
[   71.849679] device: 'my_device': device_add
[   71.853749] my_bus my_device: my_bus_uevent
[   71.857820] my_bus my_device: my_bus_match
[   71.862154] my_driver_probe
[   71.864761] my_driver my_device: my_bus_uevent
# ls /sys/bus/my_bus/devices/
my_device
# ls /sys/bus/my_bus/drivers/my_driver/
bind       my_device  uevent     unbind
# ls -alh /sys/devices/my_device/
total 0
drwxr-xr-x    3 root     root           0 Jan  1 00:02 .
drwxr-xr-x   16 root     root           0 Jan  1 00:00 ..
lrwxrwxrwx    1 root     root           0 Jan  1 00:02 driver -> ../../bus/my_bus/drivers/my_driver
drwxr-xr-x    2 root     root           0 Jan  1 00:02 power
lrwxrwxrwx    1 root     root           0 Jan  1 00:02 subsystem -> ../../bus/my_bus
-rw-r--r--    1 root     root        4.0K Jan  1 00:02 uevent

# rmmod my_device.ko
[  291.720349] my_device_module_exit
[  291.722438] my_driver_remove
[  291.725212] my_bus my_device: my_bus_uevent
[  291.729527] my_bus my_device: my_bus_uevent
[  291.733493] my_device_release
```
