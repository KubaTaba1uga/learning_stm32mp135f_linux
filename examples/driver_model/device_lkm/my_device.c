/*
 * my_device
 ****************************************************************
 * Brief Description:
 *   One of the basic blocks of driver model, the device.
 */
#include <linux/device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

MODULE_AUTHOR("Jakub Buczynski");
MODULE_DESCRIPTION("A dummy device");
MODULE_LICENSE("Dual MIT/GPL");

extern struct bus_type my_bus;

static void my_device_release(struct device *dev) {
  pr_info("%s\n", __func__);  
}

struct device my_device = {
    .init_name = "my_device",
    .release = my_device_release,    
};

static int __init my_device_module_init(void) {
  pr_info("%s\n", __func__);

  /* Registering a device means it appear in:
      /sys/devices
   */
  my_device.bus = &my_bus;  
  int ret = device_register(&my_device);
  if (ret < 0) {
    pr_err("my_device: could not register my_device\n");
    put_device(&my_device);
    return ret;
  }

  return 0;
}

static void __exit my_device_module_exit(void) {
  pr_info("%s\n", __func__);
  device_unregister(&my_device);
}

module_init(my_device_module_init);
module_exit(my_device_module_exit);
