/*
 * btn_lkm_irq
 ****************************************************************
 * Brief Description:
 * A simple module which intercept button press and expose it via sysfs.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/device/driver.h>
#include <linux/of.h>

MODULE_AUTHOR("Jakub Buczynski");
MODULE_DESCRIPTION("Button driver exposing press on button");
MODULE_LICENSE("Dual MIT/GPL");

static int my_bus_match(struct device *dev, struct device_driver *drv) {
        dev_err(dev, "my_bus_match\n");    
	return 0;
}

static int my_bus_uevent(const struct device *dev,
                         struct kobj_uevent_env *env) {
        dev_err(dev, "my_bus_uevent\n");  
	add_uevent_var(env, "MODALIAS=MY BUS IS AWESOME");
	return 0;
}

static int my_bus_probe(struct device *dev)
{
        dev_err(dev, "my_bus_probe\n");
	return 0;
}

static    void my_bus_release(struct device *dev){
  dev_err(dev,"%s\n", __func__);


  }


  struct device my_bus = {
      .init_name = "my_bus",
      .release =       my_bus_release,
};

struct bus_type my_bus_type = {
    .name = "my_bus_type",
    .match = my_bus_match,
    .probe = my_bus_probe,
    .uevent= my_bus_uevent,    
};



static int __init btn_lkm_irq_init(void) {
  pr_info("Inserted: %s\n", __func__);

  /* Registering a device means it appear in:
      /sys/devices
   */ 
  int ret = device_register(&my_bus);
  if (ret < 0) {
    pr_err("my_bus: could not register my_bus\n");
          put_device(&my_bus);    
	  return ret;
  }

  ret =  bus_register(&my_bus_type);
  if (ret) {
	  pr_err("my_bus: could not register bus for my_bus\n");
    
	  device_unregister(&my_bus);    
  }


  
  return 0;
}

static void __exit btn_lkm_irq_exit(void) {
  pr_info("Removed: %s\n", __func__);
  device_unregister(&my_bus);
  bus_unregister(&my_bus_type)  ;
}

module_init(btn_lkm_irq_init);
module_exit(btn_lkm_irq_exit);



