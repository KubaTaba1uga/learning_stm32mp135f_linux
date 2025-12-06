/*
 * btn_lkm_irq
 ****************************************************************
 * Brief Description:
 * A simple module which intercept GPIO button press and expose it via sysfs.
 */

#include "linux/device.h"
#include "linux/gfp_types.h"
#include "linux/gpio.h"
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
};

static irqreturn_t irq_handler(int irq, void *data) {
  PR_INFO("%s\n", __func__);

  return IRQ_HANDLED;
};

static int probe(struct platform_device *device) {
  PR_INFO("%s\n", __func__);
  struct btn_lkm_irq_driver *driver_instance =
      devm_kzalloc(&device->dev, sizeof(struct btn_lkm_irq_driver), GFP_KERNEL);
  if (!driver_instance) {
    PR_ERR("No driver instance\n");
    return ENOMEM;
  }

  // GPIOD to work need to find property `X-gpios` in the node.
  //  more info about this behaviour in driver-api/gpio/board.  
  struct gpio_desc *gpio = gpiod_get(&device->dev, "abc", GPIOD_IN); 
  if (IS_ERR(gpio)) {
    devm_kfree(&device->dev, driver_instance);
    PR_ERR("No GPIO descriptor\n");
    return ENOMEM;
  }

  // If IRQ is properly configured for the GPIOD we can obtain IRQ number
  int irq = gpiod_to_irq(gpio);
  if (irq < 0) {
    devm_kfree(&device->dev, driver_instance);
    PR_ERR("No IRQ number\n");
    return irq;
  }

  int irqflags = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;
  int err = devm_request_irq(&device->dev, irq, irq_handler, irqflags,
                             "btn_lkm_irq", driver_instance);
  if (err < 0) {
    devm_kfree(&device->dev, driver_instance);
    PR_ERR("Cannot request IRQ\n");
    return err;
  }

  dev_set_drvdata(&device->dev, driver_instance);
  driver_instance->irq = irq;

  return 0;
};

static int remove(struct platform_device *device) {
  PR_INFO("%s\n", __func__);

  struct btn_lkm_irq_driver *driver_instance = dev_get_drvdata(&device->dev);
  if (driver_instance) {
    devm_free_irq(&device->dev, driver_instance->irq, NULL);
    devm_kfree(&device->dev, driver_instance);
  }

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
