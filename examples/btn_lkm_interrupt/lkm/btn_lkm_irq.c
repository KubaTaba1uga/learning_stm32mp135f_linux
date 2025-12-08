/*
 * btn_lkm_irq
 ****************************************************************
 * Brief Description:
 * A simple module which intercept GPIO button press and expose it via sysfs.
 */

#include "linux/device.h"
#include "linux/gfp_types.h"
#include "linux/gpio.h"
#include "linux/gpio/consumer.h"
#include "linux/input-event-codes.h"
#include "linux/input.h"
#include "linux/interrupt.h"
#include "linux/irq.h"
#include "linux/irqreturn.h"
#include "linux/printk.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#define PR_ERR(...) pr_err("btn_lkm_irq: " __VA_ARGS__)
#define PR_INFO(...) pr_info("btn_lkm_irq: " __VA_ARGS__)

MODULE_AUTHOR("Jakub Buczynski");
MODULE_DESCRIPTION("Button driver exposing press on a button");
MODULE_LICENSE("Dual MIT/GPL");

struct btn_lkm_irq_driver {
  int irq;
  struct gpio_desc *gpiod;     // GPIO descriptor.
                               // We need it to interact with gpiolib.c.
  struct input_dev *input_dev; // Input device.
                               // We need it to inetract with input.c.
  bool btn_value;
};

static irqreturn_t irq_handler(int irq, void *data) {
  PR_INFO("%s\n", __func__);

  struct btn_lkm_irq_driver *driver_instance = data;

  // Here we are in interrupt ctx so we shouldn't make any processing,
  // we should defer it later.
  int pin_value = gpiod_get_value(driver_instance->gpiod);
  if (pin_value == driver_instance->btn_value) {
    return IRQ_HANDLED;
  }

  if (pin_value) {
    PR_INFO("%s: %s\n", __func__, "Button pressed");
    driver_instance->btn_value = 1;
    input_report_key(driver_instance->input_dev, KEY_Q, 1);
  } else {
    PR_INFO("%s: %s\n", __func__, "Button realesed");
    driver_instance->btn_value = 0;
    input_report_key(driver_instance->input_dev, KEY_Q, 0);
  }
  input_sync(driver_instance->input_dev);
  return IRQ_HANDLED;
};

static int probe(struct platform_device *device) {
  PR_INFO("%s\n", __func__);

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
    return PTR_ERR(gpio);
  }

  // If IRQ is properly configured for the GPIOD we can obtain IRQ number.
  int irq = gpiod_to_irq(gpio);
  if (irq < 0) {
    PR_ERR("Cannot obtain IRQ number from GPIO descriptor\n");
    return irq;
  }

  struct input_dev *input_dev = input_allocate_device();
  if (!input_dev) {
    PR_ERR("Cannot allocate memory for input_dev in driver instance\n");
    return -ENOMEM;
  }

  input_set_capability(input_dev, EV_KEY, KEY_Q);

  if (input_register_device(input_dev) != 0) {
    PR_ERR("Cannot register input_dev in input subsystem\n");
    return -ENOMEM;
  };

  // According devres.rst:
  //   `all devres entries are released on driver detach`
  // So we do not need to free driver_instance manually in remove nor in probe
  // errors paths.
  struct btn_lkm_irq_driver *driver_instance =
      devm_kzalloc(&device->dev, sizeof(struct btn_lkm_irq_driver), GFP_KERNEL);
  if (!driver_instance) {
    PR_ERR("Cannot allocate memory for driver instance\n");
    return -ENOMEM;
  }

  driver_instance->irq = irq;
  driver_instance->gpiod = gpio;
  driver_instance->input_dev = input_dev;
  dev_set_drvdata(&device->dev, driver_instance);

  // This function actually create IRQ in the kernel. You can
  // observe in /proc/irq/ how new entity appears during probe.
  int irqflags = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;
  int err = devm_request_irq(&device->dev, irq, irq_handler, irqflags,
                             "btn_lkm_irq", driver_instance);
  if (err < 0) {
    PR_ERR("Cannot request IRQ\n");
    return err;
  }

  return 0;
};

static int remove(struct platform_device *device) {
  PR_INFO("%s\n", __func__);

  struct btn_lkm_irq_driver *driver_instance = dev_get_drvdata(&device->dev);
  input_free_device(driver_instance->input_dev);

  return 0;
};

static const struct of_device_id btn_lkm_irq_dt_ids[] = {
    {
        .compatible = "btn_lkm_irq",
    },
    {/* sentinel */}};
MODULE_DEVICE_TABLE(of, btn_lkm_irq_dt_ids);

static struct platform_driver btn_lkm_irq = {
    .probe = probe,
    .remove = remove,
    .driver =
        {
            .name = "btn-lkm-irq",
            .of_match_table = btn_lkm_irq_dt_ids,
        },
};

module_platform_driver(btn_lkm_irq);
