/*
 * LED morse Trigger
 *
 * Copyright (C) 2006 Atsushi Nemoto <anemo@mba.ocn.ne.jp>
 *
 * Based on Richard Purdie's ledtrig-timer.c and some arch's
 * CONFIG_morse code.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/sched/loadavg.h>
#include <linux/leds.h>
#include <linux/reboot.h>
// #include <stdio.h>
#include "../leds.h"

static int panic_morses;

struct morse_trig_data {
	unsigned int phase;
	unsigned int period;
	struct timer_list timer;
	unsigned int invert;
	// unsigned int speed;
};

static const int message[21] = {
	500,	250,	500,	250,	500,	250,	1500, // S
	1500,	250,	1500,	250,	1500,	250, 	1500, // o
	500,	250,	500,	250,	500,	250,	1500, // S
};
int onOff = 0;
int myIndex = 0;

static void led_morse_function(unsigned long data)
{
	// printf("Hello, World\n");
	struct led_classdev *led_cdev = (struct led_classdev *) data;
	struct morse_trig_data *morse_data = led_cdev->trigger_data;
	unsigned long brightness = LED_OFF;
	unsigned long delay = 0;

	if (unlikely(panic_morses)) {
		led_set_brightness_nosleep(led_cdev, LED_OFF);
		return;
	}

	if (test_and_clear_bit(LED_BLINK_BRIGHTNESS_CHANGE, &led_cdev->work_flags))
		led_cdev->blink_brightness = led_cdev->new_blink_brightness;

	/* acts like an actual heart beat -- ie thump-thump-pause... */

	if (onOff == 0){
		onOff = 1;
	}
	else{
		onOff = 0;
	}
	brightness = onOff;
	if (myIndex == 21){
		myIndex = 0;
	}
	delay = msecs_to_jiffies(message[myIndex]);
	myIndex++;

	led_set_brightness_nosleep(led_cdev, brightness);
	mod_timer(&morse_data->timer, jiffies + delay);
}

static ssize_t led_invert_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct morse_trig_data *morse_data = led_cdev->trigger_data;

	return sprintf(buf, "%u\n", morse_data->invert);
}

static ssize_t led_invert_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct morse_trig_data *morse_data = led_cdev->trigger_data;
	unsigned long state;
	int ret;

	ret = kstrtoul(buf, 0, &state);
	if (ret)
		return ret;

	morse_data->invert = !!state;

	return size;
}

static DEVICE_ATTR(invert, 0644, led_invert_show, led_invert_store);
static DEVICE_ATTR(speed, 0644, led_invert_show, led_invert_store);
static void morse_trig_activate(struct led_classdev *led_cdev)
{
	struct morse_trig_data *morse_data;
	int rc;

	morse_data = kzalloc(sizeof(*morse_data), GFP_KERNEL);
	if (!morse_data)
		return;

	led_cdev->trigger_data = morse_data;
	rc = device_create_file(led_cdev->dev, &dev_attr_invert);
	device_create_file(led_cdev->dev, &dev_attr_speed);
	if (rc) {
		kfree(led_cdev->trigger_data);
		return;
	}

	setup_timer(&morse_data->timer,
		    led_morse_function, (unsigned long) led_cdev);
	morse_data->phase = 0;
	if (!led_cdev->blink_brightness)
		led_cdev->blink_brightness = led_cdev->max_brightness;
	led_morse_function(morse_data->timer.data);
	set_bit(LED_BLINK_SW, &led_cdev->work_flags);
	led_cdev->activated = true;
}

static void morse_trig_deactivate(struct led_classdev *led_cdev)
{
	struct morse_trig_data *morse_data = led_cdev->trigger_data;

	if (led_cdev->activated) {
		del_timer_sync(&morse_data->timer);
		device_remove_file(led_cdev->dev, &dev_attr_invert);
		kfree(morse_data);
		clear_bit(LED_BLINK_SW, &led_cdev->work_flags);
		led_cdev->activated = false;
	}
}

static struct led_trigger morse_led_trigger = {
	.name     = "morse",
	.activate = morse_trig_activate,
	.deactivate = morse_trig_deactivate,
};

static int morse_reboot_notifier(struct notifier_block *nb,
				     unsigned long code, void *unused)
{
	led_trigger_unregister(&morse_led_trigger);
	return NOTIFY_DONE;
}

static int morse_panic_notifier(struct notifier_block *nb,
				     unsigned long code, void *unused)
{
	panic_morses = 1;
	return NOTIFY_DONE;
}

static struct notifier_block morse_reboot_nb = {
	.notifier_call = morse_reboot_notifier,
};

static struct notifier_block morse_panic_nb = {
	.notifier_call = morse_panic_notifier,
};

static int __init morse_trig_init(void)
{
	int rc = led_trigger_register(&morse_led_trigger);

	if (!rc) {
		atomic_notifier_chain_register(&panic_notifier_list,
					       &morse_panic_nb);
		register_reboot_notifier(&morse_reboot_nb);
	}
	return rc;
}

static void __exit morse_trig_exit(void)
{
	unregister_reboot_notifier(&morse_reboot_nb);
	atomic_notifier_chain_unregister(&panic_notifier_list,
					 &morse_panic_nb);
	led_trigger_unregister(&morse_led_trigger);
}

module_init(morse_trig_init);
module_exit(morse_trig_exit);

MODULE_AUTHOR("Aidan Grimshaw <grimshaa@oregonstate.edu>");
MODULE_DESCRIPTION("Morse Code");
MODULE_LICENSE("GPL");
