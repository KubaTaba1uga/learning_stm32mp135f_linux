/*
 * my_driver
 ****************************************************************
 * Brief Description:
 * Goal of a driver is to handle a device.
 */
#include <linux/device.h>
#include <linux/device/driver.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

MODULE_AUTHOR("Jakub Buczynski");
MODULE_DESCRIPTION("");
MODULE_LICENSE("Dual MIT/GPL");

extern struct bus_type my_bus;

struct my_driver_device_data {
  int number;
};

/*
  probe holds the driver-specific logic to bind the driver to a given device.
  That includes verifying that the device is present, that it’s a version the
  driver can handle, that driver data structures can be allocated and
  initialized, and that any hardware can be initialized. Drivers often store a
  pointer to their state with dev_set_drvdata(). When the driver has
  successfully bound itself to that device, then probe() returns zero and the
  driver model code will finish its part of binding the driver to that device.

  A driver’s probe() may return a negative errno value to indicate that the
  driver did not bind to this device, in which case it should have released all
  resources it allocated.
*/
static int my_driver_probe(struct device *dev) {
  struct my_driver_device_data *dev_data =
      kmalloc(sizeof(struct my_driver_device_data), GFP_KERNEL);
  if (!dev_data) {
    pr_err("Unable to malloc %s\n", __func__);
    return -ENOMEM;
  }

  dev_data->number = 99;
  dev_set_drvdata(dev, dev_data);

  pr_info("%s: %d\n", __func__, dev_data->number);
  return 0;
}

/*
  remove is called to unbind a driver from a device. This may be called if a
  device is physically removed from the system, if the driver module is being
  unloaded, during a reboot sequence, or in other cases.
*/
static int my_driver_remove(struct device *dev) {
  struct my_driver_device_data *dev_data = dev_get_drvdata(dev);
  
  pr_info("%s: %d\n", __func__, dev_data->number);
  
  kfree(dev_data);
  return 0;
}

static struct device_driver my_driver = {
    .name = "my_driver",
    .bus = &my_bus,
    .probe = my_driver_probe,
    .remove = my_driver_remove,
};

static int __init my_driver_module_init(void) {
  pr_info("Inserted: %s\n", __func__);

  /* Registering a driver means it appear in:
       /sys/bus/my_bus/drivers
   */
  int ret = driver_register(&my_driver);
  if (ret) {
    pr_err("my_driver: could not register my_driver\n");
  }

  return 0;
}

static void __exit my_driver_module_exit(void) {
  pr_info("Removed: %s\n", __func__);
  driver_unregister(&my_driver);
}

module_init(my_driver_module_init);
module_exit(my_driver_module_exit);
