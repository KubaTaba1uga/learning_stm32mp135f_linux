/*
 * btn_lkm_irq
 ****************************************************************
 * Brief Description:
 * LKM intercept GPIO button press and expose it via input subsystem.

 TO-DO: allow setting keycode via an attribute

 */

#include "linux/container_of.h"
#include "linux/device.h"
#include "linux/device/driver.h"
#include "linux/gfp_types.h"
#include "linux/gpio/consumer.h"
#include "linux/input-event-codes.h"
#include "linux/input.h"
#include "linux/interrupt.h"
#include "linux/irqreturn.h"
#include "linux/printk.h"
#include "linux/spinlock.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/sprintf.h>

/*
 |===============================================================|
 |                                                               |
 |                         Attributes                            |
 |                                                               |
 |===============================================================|
 */
static int btn_lkm_irq_debug = 1;

static ssize_t btn_lkm_irq_debug_show(struct device_driver *dev, char *buf) {
  (void)btn_lkm_irq_debug_show;
  return sprintf(buf, "%d", btn_lkm_irq_debug);
};

static ssize_t btn_lkm_irq_debug_store(struct device_driver *dev,
                                       const char *buf, size_t count) {
  (void)btn_lkm_irq_debug_store;
  if (strncmp("0", buf, count) == 0 || strncmp("0\n", buf, count) == 0) {
    btn_lkm_irq_debug = 0;
  } else {
    btn_lkm_irq_debug = 1;
  };
  return 1;
}

static DRIVER_ATTR_RW(btn_lkm_irq_debug);

/*
 |===============================================================|
 |                                                               |
 |                           Macros                              |
 |                                                               |
 |===============================================================|
 */
#define PR_ERR(...) pr_err("btn_lkm_irq: " __VA_ARGS__)
#define PR_INFO(...)                                                           \
  if (btn_lkm_irq_debug == 1) {                                                \
    pr_info("btn_lkm_irq: " __VA_ARGS__);                                      \
  }

/*
 |===============================================================|
 |                                                               |
 |                             API                               |
 |                                                               |
 |===============================================================|
 */
struct btn_lkm_irq_drvdata {
  spinlock_t lock;
  int irq;
  struct gpio_desc *gpiod;     // GPIO descriptor.
                               // We need it to interact with gpiolib.c.
  struct input_dev *input_dev; // Input device.
                               // We need it to inetract with input.c.
  bool btn_is_pressed;
};

static irqreturn_t btn_lkm_irq_irs(int irq, void *data) {
  PR_INFO("%s\n", __func__);

  struct btn_lkm_irq_drvdata *drvdata = data;

  int pin_value = gpiod_get_value(drvdata->gpiod);
  if (pin_value == drvdata->btn_is_pressed) {
    return IRQ_HANDLED;
  }

  drvdata->btn_is_pressed = pin_value;
  input_report_key(drvdata->input_dev, KEY_A, drvdata->btn_is_pressed);
  input_sync(drvdata->input_dev);

  PR_INFO("%s: %s\n", __func__,
          pin_value ? "Button pressed" : "Button realesed");

  return IRQ_HANDLED;
};

static int btn_lkm_irq_probe(struct platform_device *device) {
  PR_INFO("%s\n", __func__);
  int err;
  
  // According devres.rst:
  //   `all devres entries are released on driver detach`
  // So we do not need to free drvdata manually in remove nor in probe
  // errors paths.
  struct btn_lkm_irq_drvdata *drvdata = devm_kzalloc(
      &device->dev, sizeof(struct btn_lkm_irq_drvdata), GFP_KERNEL);
  if (!drvdata) {
    PR_ERR("Cannot allocate memory for driver instance\n");
    return -ENOMEM;
  }
  spin_lock_init(&drvdata->lock);
  spin_lock(&drvdata->lock);

  // GPIOD to work need to find property `X-gpios` in the DT node.
  // more info about this behaviour in driver-api/gpio/board and
  // driver-api/gpio/consumer. DT example:
  //         button-b1-enter {
  //		compatible = "btn_lkm_irq";
  //		abc-gpios = <&gpioa 14 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
  //      };
  //
  // This gpio descriptor is assigned to the device so we do not
  // need to free it manually.
  struct gpio_desc *gpio = devm_gpiod_get(&device->dev, "abc", GPIOD_IN);
  if (IS_ERR(gpio)) {
    PR_ERR("Cannot get GPIO descriptor\n");
    err = PTR_ERR(gpio);
    goto err_lock_cleanup;
  }

  // If IRQ is properly configured for the GPIOD we can obtain IRQ number.
  int irq = err = gpiod_to_irq(gpio);
  if (err < 0) {
    PR_ERR("Cannot obtain IRQ number from GPIO descriptor\n");
    goto err_lock_cleanup;
  }

  // Register into input subsystem.
  struct input_dev *input_dev = devm_input_allocate_device(&device->dev);
  if (!input_dev) {
    PR_ERR("Cannot allocate memory for input_dev in driver instance\n");
    err = -ENOMEM;
    goto err_lock_cleanup;
  }

  input_set_capability(input_dev, EV_KEY, KEY_A);

  err = input_register_device(input_dev);
  if (err) {
    PR_ERR("Cannot register input_dev in input subsystem\n");
    goto err_lock_cleanup;
  };

  drvdata->irq = irq;
  drvdata->gpiod = gpio;
  drvdata->input_dev = input_dev;
  dev_set_drvdata(&device->dev, drvdata);

  // This function actually create IRQ in the kernel. You can
  // observe in /proc/irq/ how new entity appears during probe.
  int irqflags = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;
  err = devm_request_irq(&device->dev, irq, btn_lkm_irq_irs, irqflags,
                         "btn_lkm_irq", drvdata);
  if (err < 0) {
    PR_ERR("Cannot request IRQ\n");
    goto err_input_cleanup;
  }

  spin_unlock(&drvdata->lock);

  return 0;

err_input_cleanup:
  input_unregister_device(drvdata->input_dev);
err_lock_cleanup:
  spin_unlock(&drvdata->lock);
  return err;
};

static int btn_lkm_irq_remove(struct platform_device *device) {
  PR_INFO("%s\n", __func__);

  struct btn_lkm_irq_drvdata *drvdata = dev_get_drvdata(&device->dev);
  input_unregister_device(drvdata->input_dev);

  return 0;
};

static const struct of_device_id btn_lkm_irq_dt_ids[] = {
    {
        .compatible = "btn_lkm_irq",
    },
    {/* sentinel */}};
MODULE_DEVICE_TABLE(of, btn_lkm_irq_dt_ids);

static struct platform_driver btn_lkm_irq = {
    .probe = btn_lkm_irq_probe,
    .remove = btn_lkm_irq_remove,
    .driver =
        {
            .name = "btn-lkm-irq",
            .of_match_table = btn_lkm_irq_dt_ids,
        },
};

static int __init btn_lkm_irq_init(void) {
  pr_info("Inserted: %s\n", __func__);

  int err = platform_driver_register(&btn_lkm_irq);
  if (err) {
    PR_ERR("Cannot register platform driver\n");
    return err;
  }

  err = driver_create_file(&btn_lkm_irq.driver, &driver_attr_btn_lkm_irq_debug);
  if (err) {
    PR_ERR("Cannot create btn_lkm_irq_debug attribute\n");
    return err;
  }

  return 0;
}

static void __exit btn_lkm_irq_exit(void) {
  platform_driver_unregister(&btn_lkm_irq);
}

module_init(btn_lkm_irq_init);
module_exit(btn_lkm_irq_exit);

MODULE_AUTHOR("Jakub Buczynski");
MODULE_DESCRIPTION("GPIO button driver, exposing press via input subsystem");
MODULE_LICENSE("Dual MIT/GPL");
