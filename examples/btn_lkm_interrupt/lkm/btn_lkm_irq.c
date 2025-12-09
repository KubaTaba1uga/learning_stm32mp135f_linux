/*
 * btn_lkm_irq
 ****************************************************************
 * Platform LKM that binds to a DT node, requests an IRQ from a GPIO
 * line, and reports button state changes through the Linux input
 * subsystem as EV_KEY events.
 *
 * Device Tree:
 * compatible = "btn_lkm_irq";
 * irq-gpios  = <&gpioX N FLAGS>;
 * code       = <KEY_* or BTN_*>;
 *
 */

#include "linux/device.h"
#include "linux/device/driver.h"
#include "linux/gfp_types.h"
#include "linux/gpio/consumer.h"
#include "linux/input-event-codes.h"
#include "linux/input.h"
#include "linux/interrupt.h"
#include "linux/irqreturn.h"
#include "linux/printk.h"
#include "linux/property.h"
#include "linux/spinlock.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/sprintf.h>

struct btn_lkm_irq_drvdata {
  spinlock_t lock;
  int irq;
  struct gpio_desc *gpiod;     // GPIO descriptor.
                               // We need it to interact with gpiolib.c.
  struct input_dev *input_dev; // Input device.
                               // We need it to interact with input.c.
  bool btn_is_pressed;
  uint32_t btn_value;
};

static irqreturn_t btn_lkm_irq_irs(int irq, void *data) {
  struct platform_device *device = data;
  struct btn_lkm_irq_drvdata *drvdata = platform_get_drvdata(data);
  spin_lock(&drvdata->lock);

  int pin_value = gpiod_get_value(drvdata->gpiod);
  if (pin_value == drvdata->btn_is_pressed) {
    goto out;
  }

  drvdata->btn_is_pressed = pin_value;

  input_report_key(drvdata->input_dev, drvdata->btn_value,
                   drvdata->btn_is_pressed);
  input_sync(drvdata->input_dev);

  dev_dbg(&device->dev, "%s\n",
          drvdata->btn_is_pressed ? "Button pressed" : "Button realesed");

out:
  spin_unlock(&drvdata->lock);

  return IRQ_HANDLED;
};

static int btn_lkm_irq_probe(struct platform_device *device) {
  int err;

  // According devres.rst:
  //   `all devres entries are released on driver detach`
  // So we do not need to free drvdata manually in remove nor in probe
  // errors paths.
  struct btn_lkm_irq_drvdata *drvdata = devm_kzalloc(
      &device->dev, sizeof(struct btn_lkm_irq_drvdata), GFP_KERNEL);
  if (!drvdata) {
    dev_err(&device->dev, "Cannot allocate memory for driver instance\n");
    return -ENOMEM;
  }
  spin_lock_init(&drvdata->lock);
  spin_lock(&drvdata->lock);

  // GPIOD to work need to find property `X-gpios` in the DT node.
  // more info about this behaviour in driver-api/gpio/board and
  // driver-api/gpio/consumer. DT example:
  //         button-b1-enter {
  //		compatible = "btn_lkm_irq";
  //		irq-gpios = <&gpioa 14 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
  // 		code = <KEY_ENTER>;
  //      };
  //
  // This gpio descriptor is assigned to the device so we do not
  // need to free it manually.
  struct gpio_desc *gpio = devm_gpiod_get(&device->dev, "irq", GPIOD_IN);
  if (IS_ERR(gpio)) {
    dev_err(&device->dev, "Cannot get `irq-gpios` GPIO descriptor\n");
    err = PTR_ERR(gpio);
    goto err_lock_cleanup;
  }

  // If IRQ is properly configured for the GPIOD we can obtain IRQ number.
  int irq = err = gpiod_to_irq(gpio);
  if (err < 0) {
    dev_err(&device->dev, "Cannot obtain IRQ number from GPIO descriptor\n");
    goto err_lock_cleanup;
  }

  // Register into input subsystem.
  struct input_dev *input_dev = devm_input_allocate_device(&device->dev);
  if (!input_dev) {
    dev_err(&device->dev,
            "Cannot allocate memory for input_dev in driver instance\n");
    err = -ENOMEM;
    goto err_lock_cleanup;
  }

  err = device_property_read_u32(&device->dev, "code", &drvdata->btn_value);
  if (err) {
    dev_err(&device->dev, "Cannot find `code` property in DT\n");
    drvdata->btn_value = BTN_0;
  };

  input_set_capability(input_dev, EV_KEY, drvdata->btn_value);

  err = input_register_device(input_dev);
  if (err) {
    dev_err(&device->dev, "Cannot register input_dev in input subsystem\n");
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
                         "btn_lkm_irq", device);
  if (err < 0) {
    dev_err(&device->dev, "Cannot request IRQ\n");
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
  int err = platform_driver_register(&btn_lkm_irq);
  if (err) {
    pr_err("btn_lkm_irq: Cannot register platform driver\n");
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
MODULE_DESCRIPTION("DT-bound GPIO button IRQ driver reporting EV_KEY via input subsystem");
MODULE_LICENSE("Dual MIT/GPL");
