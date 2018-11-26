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
#include "../leds.h"

#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/ctype.h>

#define FIRST_MINOR 0
#define MINOR_CNT 1

static dev_t dev;
static struct cdev c_dev;
static struct class *cl;

static int panic_morses;

struct morse_trig_data {
	unsigned int phase;
	unsigned int period;
	struct timer_list timer;
	unsigned int invert;
	unsigned int speed;
	unsigned int mode;
};

int *delayMessage;
// adds delays for dots/dashes/spaces/character ending
void dot_delay( int idx){
	delayMessage[idx] = 500;
	idx++;
	delayMessage[idx] = 250;
}

void dash_delay( int idx){
	delayMessage[idx] = 1500;
	idx++;
	delayMessage[idx] = 250;
}

void space_delay( int idx){
	delayMessage[idx] = 3500;
}

void char_delay( int idx){
	delayMessage[idx] = 1500;
}

// Morse code alphabet
static char *alphabet[] = {
	".-",	// A .-
	"-...",	// B -...
	"-.-.",	// C -.-.
	"-..",	// D -..
	".",	// E .
	"..-.",	// F ..-.
	"--.",	// G --.
	"....",	// H ....
	"..",	// I ..
	".---",	// J .---
	"-.-",	// K -.-
	".-..",	// L .-..
	"--",	// M --
	"-.",	// N -.
	"---",	// O ---
	".--.",	// P .--.
	"--.-",	// Q --.-
	".-.",	// R .-.
	"...",	// S ...
	"-",	// T -
	"..-",	// U ..-
	"...-",	// V ...-
	".--",	// W .--
	"-..-",	// X -..-
	"-.--",	// Y -.--
	"--.."	// Z --..
};
int count = 0;
void convert(char *message){
	printk("CS444 convert %s\r\n", message);
	// printk("CS444 convert 1");
	// converts the message to lowercase
	int i = 0;	
	while(message[i] != '\0'){
		message[i] = tolower(message[i]);
		i++;
	}
	// printk("CS444 convert 2");
	// converts the message into morse code dots and dashes (./-)
	char *morse_message[i];
	i = 0;
	int dec; // decimal integer value of ascii character
	while(message[i] != '\0'){
		dec = (int)message[i];
		if(dec == 32){
			morse_message[i] = " ";
		}
		else{
			dec -= 97;
			morse_message[i] = alphabet[dec];
		}
		i++;
	}
	printk("CS444 convert 3");
	// counts how many delays will be needed for the message
	int len = i;	
	int j = 0;
	for(i = 0; i < len; i++){
		j = 0;
		while(morse_message[i][j] != '\0'){
			if(morse_message[i][j] == '.' || morse_message[i][j] == '-'){
				count += 2;
			}
			j++;
		}
	}
	printk("CS444 convert 4");
	// puts the delays into an array
	delayMessage = (int*)kmalloc(count*sizeof(int),GFP_USER); 
	// int idx = 0;
	// for(i = 0; i < len; i++){
	// 	j = 0;
	// 	while(morse_message[i][j] != '\0'){
	// 		if(morse_message[i][j] == '.'){
	// 			dot_delay(idx);
	// 			idx += 2;
	// 		} 
	// 		else if(morse_message[i][j] == '-'){
	// 			dash_delay(idx);
	// 			idx += 2;
	// 		}
	// 		else{
	// 			space_delay(idx-1);
	// 		}	
	// 		j++;
	// 	}
	// 	if(morse_message[i] != " "){
	// 		char_delay(idx-1);
	// 	}
	// }
	// space_delay(idx-1);
}

static const int message[18] = {
	500,	250,	500,	250,	500,	1750, // S
	1500,	250,	1500,	250,	1500,	1750, // o
	500,	250,	500,	250,	500,	1750, // S
};
int onOff = 0;
int myIndex = 0;
unsigned int ranThrough = 0;
unsigned int newChars = 0;
char *lcl_buf;

static void led_morse_function(unsigned long data)
{
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

	if (1 == 0){
			// if previous was off, new one is on
		if (onOff == 0){
			onOff = 1;
		}
		else{
			onOff = 0;
		}
		brightness = onOff;
		// wrap around message if in repeat mode
		if (myIndex == count){
			myIndex = 0;
			ranThrough = 1;
		}
		// if not in repeat mode and at end of message, turn off
		if (morse_data->mode == 1 && ranThrough == 1){
			brightness = LED_OFF;
		}
		if (morse_data->mode == 0){
			ranThrough = 0;
		}

		delay = msecs_to_jiffies(delayMessage[myIndex]);
		// if speed is selected, multiply delay by two to make morse print slower
		if (morse_data->speed > 0){
			// brightness = LED_OFF;
			delay = msecs_to_jiffies(message[myIndex]/morse_data->speed);
		}
		// otherwise normal speed
		
		myIndex++;
	}

	led_set_brightness_nosleep(led_cdev, brightness);
	mod_timer(&morse_data->timer, jiffies + delay);
}

static ssize_t led_speed_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct morse_trig_data *morse_data = led_cdev->trigger_data;

	return sprintf(buf, "%u\n", morse_data->speed);
}

static ssize_t led_speed_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct morse_trig_data *morse_data = led_cdev->trigger_data;
	unsigned long state;
	int ret;

	ret = kstrtoul(buf, 0, &state);
	if (ret)
		return ret;

	morse_data->speed = state;

	return size;
}
static ssize_t led_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct morse_trig_data *morse_data = led_cdev->trigger_data;

	return sprintf(buf, "%u\n", morse_data->mode);
}

static ssize_t led_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct morse_trig_data *morse_data = led_cdev->trigger_data;
	unsigned long state;
	int ret;

	ret = kstrtoul(buf, 0, &state);
	if (ret)
		return ret;

	morse_data->mode = !!state;

	return size;
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

static int my_open(struct inode *i, struct file *f)
{
    printk("CS444 Dummy driver open\r\n");
    return 0;
}
static int my_close(struct inode *i, struct file *f)
{
    printk("CS444 Dummy driver close\r\n");
    return 0;
}

static ssize_t dummy_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    printk("CS444 Dummy driver read\r\n");
    snprintf(buf, size, "Hey there, I'm a dummy!\r\n");
    return strlen(buf);
}

static ssize_t dummy_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
	// allocate local buffer the size of the one being passed in
    lcl_buf = (char *)kmalloc(strlen(buf)*sizeof(char)+1,GFP_USER);
	// printk("CS444 dummy write 1"); 
    
	memset(lcl_buf, 0, sizeof(strlen(buf)*sizeof(char)+1));
	// printk("CS444 dummy write 2"); 
    
	if (copy_from_user(lcl_buf, buf, size))
	{
		return -EACCES;
	}
	printk("CS444 dummy write 3"); 
	newChars = 1;
	convert(lcl_buf);
	printk("CS444 dummy write 4"); 
    printk("CS444 Dummy driver write %ld bytes: %s\r\n", size, lcl_buf);

    return size;
}

static struct file_operations dummy_fops =
{
    .owner = THIS_MODULE,
    .open = my_open,
    .read = dummy_read,
    .write = dummy_write,
    .release = my_close
};

static DEVICE_ATTR(invert, 0644, led_invert_show, led_invert_store);
static DEVICE_ATTR(speed, 0644, led_speed_show, led_speed_store);
static DEVICE_ATTR(mode, 0644, led_mode_show, led_mode_store);

static void morse_trig_activate(struct led_classdev *led_cdev)
{
	// creating character device start		
	alloc_chrdev_region(&dev, 0, 1, "cs444_dummy");

	cdev_init(&c_dev, &dummy_fops);
	cdev_add(&c_dev, dev, MINOR_CNT);

	cl = class_create(THIS_MODULE,"char");
	device_create(cl,NULL,dev,NULL,"morse");
	printk("CS444 Dummy Driver has been loaded!\r\n");
	// create character device end

	struct morse_trig_data *morse_data;
	int rc;

	morse_data = kmalloc(sizeof(*morse_data), GFP_KERNEL);
	if (!morse_data)
		return;

	led_cdev->trigger_data = morse_data;
	rc = device_create_file(led_cdev->dev, &dev_attr_invert);
	device_create_file(led_cdev->dev, &dev_attr_speed);
	device_create_file(led_cdev->dev, &dev_attr_mode);
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
	// destroy character device start
	device_destroy(cl, dev);
    class_destroy(cl);
    cdev_del(&c_dev);
    unregister_chrdev_region(dev, MINOR_CNT);
	// destroy character device end

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
