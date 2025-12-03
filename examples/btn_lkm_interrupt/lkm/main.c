/*
 * ads7830_soil_humid driver
 ****************************************************************
 * Brief Description:
 * A simple module which implements driver for ADS7830 and implements coversion
 *  for simple soil humid driver. Each channel is occupied by one soild humid
 *  sensor. Sensors values can be accessed under sysfs.
 */
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <linux/moduleparam.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

MODULE_AUTHOR("Jakub Buczynski");
MODULE_DESCRIPTION("Custom soil humidity driver utilising ADS7830");
MODULE_LICENSE("Dual MIT/GPL");

