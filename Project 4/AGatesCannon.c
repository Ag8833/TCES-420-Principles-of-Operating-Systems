#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>       // Required for the GPIO functions
#include <linux/kobject.h>    // Using kobjects for the sysfs bindings

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew Gates");
MODULE_DESCRIPTION("Linux LKM for Cannon Firing");
MODULE_VERSION("0.1");

static unsigned int gpioCannonFireTrigger = 17;           ///< Default GPIO is 49
module_param(gpioCannonFireTrigger, uint, S_IRUGO);       ///< Param desc. S_IRUGO can be read/not changed
MODULE_PARM_DESC(gpioCannonFireTrigger, "Cannon Fire Trigger");         ///< parameter description

static unsigned int gpioCannon = 26;           ///< Default GPIO is 49
module_param(gpioCannon, uint, S_IRUGO);       ///< Param desc. S_IRUGO can be read/not changed
MODULE_PARM_DESC(gpioCannon, " GPIO LED number (default=49)");         ///< parameter description

static char gpioName[8] = "gpioXXX";      ///< Null terminated default string -- just in case
static int missileCount = 6;

static ssize_t FIRE_ONE_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
   if(missileCount == 0)
   {
      return sprintf(buf, "NO MISSLES LEFT\n");
   }
   else
   {
      while(gpio_get_value(gpioCannonFireTrigger) == 1)
      {
         gpio_set_value(gpioCannon, 1);
      }
      missileCount--;
      gpio_set_value(gpioCannon, 0);
      printk(KERN_INFO "AGatesCannon: FIRING ONE\n");
      return sprintf(buf, "FIRING ONE, MISSLES LEFT: %d\n", missileCount);
   }
}

static ssize_t FIRE_ALL_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
   missileCount = 6;
   if(missileCount == 0)
   {
      return sprintf(buf, "NO MISSLES LEFT\n");
   }
   else
   {
	  while(missileCount > 0)
	  {
		 gpio_set_value(gpioCannon, 1); 
         if(gpio_get_value(gpioCannonFireTrigger) == 0)
         {
			while(gpio_get_value(gpioCannonFireTrigger) == 0)
		    {
			   //sleep(1);
			}	
            missileCount--;
            if(missileCount == 0)
            {
			   gpio_set_value(gpioCannon, 0); 
			   printk(KERN_INFO "AGatesCannon: FIRING ALL\n");
	           return sprintf(buf, "FIRING ALL, MISSLES LEFT: %d\n", missileCount);
		    }
         }
      }
      return sprintf(buf, "FIRING ALL, MISSLES LEFT: %d\n", missileCount);
   }
}

static ssize_t NR_MISSLES_REMAINING_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
   printk(KERN_INFO "AGatesCannon: NUMBER OF MISSLES REMAINING: %d\n", missileCount);
   return sprintf(buf, "NUMBER OF MISSLES REMAINING: %d\n", missileCount);
}

static ssize_t SET_NR_MISSLES_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
   return sprintf(buf, "RELOAD AND ECHO IN NUBMER OF MISSLES INSERTED\n");
}

static ssize_t SET_NR_MISSLES_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
   unsigned int temp;
   sscanf(buf, "%du", &temp);                // use a temp varable for correct int->bool
   missileCount = temp;
   printk(KERN_INFO "AGatesCannon: UPDATED MISSLE COUNT TO: %d\n", missileCount);
   return sprintf(buf, "UPDATED MISSLE COUNT TO: %d\n", missileCount);
}

static struct kobj_attribute SET_NR_MISSLES_attr = __ATTR(SET_NR_MISSLES, 0660, SET_NR_MISSLES_show, SET_NR_MISSLES_store);
static struct kobj_attribute FIRE_ONE_attr  = __ATTR_RO(FIRE_ONE);
static struct kobj_attribute FIRE_ALL_attr  = __ATTR_RO(FIRE_ALL);
static struct kobj_attribute NR_MISSLES_REMAINING_attr  = __ATTR_RO(NR_MISSLES_REMAINING);

static struct attribute *Cannon_attrs[] = 
{
      &FIRE_ONE_attr.attr,
      &FIRE_ALL_attr.attr,
      &NR_MISSLES_REMAINING_attr.attr,
      &SET_NR_MISSLES_attr.attr,
      NULL,
};

static struct attribute_group attr_group = 
{
      .name  = gpioName,                 ///< The name is generated in Cannon_init()
      .attrs = Cannon_attrs,                ///< The attributes array defined just above
};

static struct kobject *Cannon_kobj;

static int __init Cannon_init(void)
{
   int result = 0;

   printk(KERN_INFO "Initializing the Cannon LKM\n");
   sprintf(gpioName, "gpio%d", gpioCannon);           // Create the gpio26 name for /sys/Cannon/gpio26

   // create the kobject sysfs entry at /sys/Cannon -- probably not an ideal location!
   Cannon_kobj = kobject_create_and_add("Cannon", kernel_kobj->parent); // kernel_kobj points to /sys/kernel
   if(!Cannon_kobj)
   {
      printk(KERN_ALERT "AGatesCannon: failed to create kobject mapping\n");
      return -ENOMEM;
   }
   // add the attributes to /sys/Cannon/ -- for example, /sys/Cannon/gpio26/FIRE_ALL
   result = sysfs_create_group(Cannon_kobj, &attr_group);
   if(result) 
   {
      printk(KERN_ALERT "AGatesCannon: failed to create sysfs group\n");
      kobject_put(Cannon_kobj);                          // clean up -- remove the kobject sysfs entry
      return result;
   }
   
   gpio_request(gpioCannon, "sysfs");          // gpioCannon is hardcoded to 26, request it
   gpio_direction_output(gpioCannon, 0);       // Set the gpio to be in output mode and on
   gpio_export(gpioCannon, false);             // Causes gpio26 to appear in /sys/class/gpio

   gpio_request(gpioCannonFireTrigger, "sysfs");       // Set up the gpioCannonFireTrigger
   gpio_direction_input(gpioCannonFireTrigger);        // Set the button GPIO to be an input
   gpio_export(gpioCannonFireTrigger, false);

   // Perform a quick test to see that the button is working as expected on LKM load
   printk(KERN_INFO "AGatesCannon: The cannon state is currently: %d\n", gpio_get_value(gpioCannon));
         
   gpio_set_value(gpioCannon, 0);
   missileCount = 6;
   return result;
}

static void __exit Cannon_exit(void)
{
   kobject_put(Cannon_kobj);                   // clean up -- remove the kobject sysfs entry
   
   gpio_set_value(gpioCannon, 0);              // Turn the LED off, makes it clear the device was unloaded
   gpio_unexport(gpioCannon);                  // Unexport the LED GPIO
   gpio_unexport(gpioCannonFireTrigger);
   
   gpio_free(gpioCannon);                      // Free the LED GPIO
   gpio_free(gpioCannonFireTrigger);
   
   printk(KERN_INFO "AGatesCannon: Goodbye from the AGatesCannon LKM!\n");
}

// This next calls are  mandatory -- they identify the initialization function
// and the cleanup function (as above).
module_init(Cannon_init);
module_exit(Cannon_exit);
