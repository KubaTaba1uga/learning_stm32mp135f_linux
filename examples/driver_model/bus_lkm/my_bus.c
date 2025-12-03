/*
 * my_bus
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

/*
 * @match:	Called, perhaps multiple times, whenever a new device or driver
 *		is added for this bus. It should return a positive value if the
 *		given device can be handled by the given driver and zero
 *		otherwise. It may also return error code if determining that
 *		the driver supports the device is not possible. In case of
 *		-EPROBE_DEFER it will queue the device for deferred probing.
 */
static int my_bus_match(struct device *dev, struct device_driver *drv) {
  dev_err(dev, "my_bus_match\n");
  return 1; // we always match
}

static int my_bus_uevent(const struct device *dev,
                         struct kobj_uevent_env *env) {
  dev_err(dev, "my_bus_uevent\n");
  add_uevent_var(env, "MODALIAS=MY BUS IS AWESOME");
  return 0;
}

struct bus_type my_bus = {
    .name = "my_bus",
    .match = my_bus_match,
    .uevent = my_bus_uevent,
};

static int __init my_bus_module_init(void) {
  pr_info("Inserted: %s\n", __func__);

  /* Registering a bus means it appear in:
      /sys/bus
   */  
  int ret = bus_register(&my_bus);
  if (ret) {
    pr_err("my_bus: could not register bus for my_bus\n");
  }

  return 0;
}

static void __exit my_bus_module_exit(void) {
  pr_info("Removed: %s\n", __func__);
  bus_unregister(&my_bus);
}

module_init(my_bus_module_init);
module_exit(my_bus_module_exit);
EXPORT_SYMBOL_GPL(my_bus);
