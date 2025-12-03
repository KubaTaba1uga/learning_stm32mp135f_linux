/*
 * my_driver
 ****************************************************************
 * Brief Description:
 * Goal of a bus is to match device and a driver into the pair.
 */
#include <linux/device.h>
#include <linux/device/driver.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>

MODULE_AUTHOR("Jakub Buczynski");
MODULE_DESCRIPTION("Button driver exposing press on button");
MODULE_LICENSE("Dual MIT/GPL");

extern struct bus_type my_bus;

static int my_driver_probe(struct device *dev)
{
	return 0;
}

static struct device_driver my_driver = {
	.name = "my_driver",
	.bus = &my_bus,
	.probe = my_driver_probe,
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
