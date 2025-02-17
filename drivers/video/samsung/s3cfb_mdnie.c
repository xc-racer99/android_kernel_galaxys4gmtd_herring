/* linux/drivers/video/samsung/s3cfb_mdnie.c
 *
 * Register interface file for Samsung MDNIE driver
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/ctype.h>
#include <linux/miscdevice.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fb.h>


#include <asm/io.h>
#include <mach/map.h>
#include <plat/clock.h>
#include <plat/fb.h>
#include <plat/regs-fb.h>
#include <plat/pm.h>
#include "s3cfb.h"
#include "s3cfb_mdnie.h"
#include "s3cfb_ielcd.h"



static struct resource *s3c_mdnie_mem;
static void __iomem *s3c_mdnie_base;

#define C110_MDNIE_ADDR	0xfae00000

#define s3c_mdnie_readl(addr)             __raw_readl((s3c_mdnie_base + addr))
#define s3c_mdnie_writel(val,addr)        __raw_writel(val,(s3c_mdnie_base + addr))


static char banner[] __initdata = KERN_INFO "S3C MDNIE Driver, (c) 2010 Samsung Electronics\n";

struct clk		*mdnie_clock;

// #define MDNIE_TUNING

/*********** for debug **********************************************************/
#if 0 
#define gprintk(fmt, x... ) printk( "%s(%d): " fmt, __FUNCTION__ ,__LINE__, ## x)
#else
#define gprintk(x...) do { } while (0)
#endif
/*******************************************************************************/


#define END_SEQ		0xffff

#define TRUE 1
#define FALSE 0

static u16 pre_0x0100 = 0;

#if defined(CONFIG_ARIES_LATONA)
extern void on_cabc(void);
#endif

static DEFINE_MUTEX(mdnie_use);


typedef struct {
	u16 addr;
	u16 data;
} mDNIe_data_type;

struct class *mdnieset_ui_class;
struct device *switch_mdnieset_ui_dev;
struct class *mdnieset_outdoor_class;
struct device *switch_mdnieset_outdoor_dev;

#ifdef MDNIE_TUNING
#include "s3cfb_mdnie_tune_value.h"
#else
#include "s3cfb_mdnie_value.h"
#endif

Lcd_mDNIe_UI current_mDNIe_UI = mDNIe_UI_MODE; // mDNIe Set Status Checking Value.
u8 current_mDNIe_OutDoor_OnOff = FALSE;

int mDNIe_Tuning_Mode = FALSE;

#ifdef CONFIG_FB_S3C_MDNIE_TUNINGMODE_FOR_BACKLIGHT
#if 0
u16 mDNIe_data_ui[50] = {0};
u16 mDNIe_data_300cd_level1[50] = {0};
u16 mDNIe_data_300cd_level2[50] = {0};
u16 mDNIe_data_ui_down[50] = {0};
#else

extern int pre_val;

u16 mDNIe_data_ui[]=
{
#if defined(CONFIG_ARIES_LATONA)
	0x0084,0x0020, //algorithm selection + mcm
	0x0090,0x0000, //decontour th.
	0x0094,0x0fff, //directional th.
	0x0098,0x005c, //simplicity th.
	0x009c,0x0ff0, //de 127, ce off
	0x00ac,0x0200, //skin detect off, cs 512
	0x00b4,0x0100, //de th.
	0x0120,0x0064, //MCM 10000K
	0x0140,0x8d00, //cb
	0x0148,0x0073, //cr
	0x0134,0xFFF8, //LSF 248
	END_SEQ, 0x0000,
#else
	0x0084, 0x0040,
	0x0090, 0x0000,
	0x0094, 0x0fff,
	0x0098, 0x005C,
	0x009C, 0x0ff0,
	0x00AC, 0x0080,
	0x00B4, 0x0180,
	0x00C0, 0x0400,
	0x00C4, 0x7200,
	0x00C8, 0x008D,
	0x00D0, 0x00C0,
	0x0100, 0x0000,
	END_SEQ, 0x0000,
#endif
};

u16 mDNIe_data_300cd_level1[]=
{
	0x0084, 0x0080,
	0x0090, 0x0000,
	0x0094, 0x0fff,
	0x0098, 0x005C,
	0x009C, 0x0ff0,
	0x00AC, 0x0080,
	0x00B4, 0x0180,
	0x00C0, 0x0400,
	0x00C4, 0x7200,
	0x00C8, 0x008D,
	0x00D0, 0x00C0,
	0x0100, 0x4020,
	END_SEQ, 0x0000,
};
#if 0
u16 mDNIe_data_300cd_level2[]=
{
	0x0084, 0x0080,
	0x0090, 0x0000,
	0x0094, 0x0fff,
	0x0098, 0x005C,
	0x009C, 0x0ff0,
	0x00AC, 0x00E0,
	0x00B4, 0x01C2,
	0x00C0, 0x0400,
	0x00C4, 0x7200,
	0x00C8, 0x008D,
	0x00D0, 0x0100,
	0x0100, 0x6060,
	END_SEQ, 0x0000,
};
#endif
u16 mDNIe_data_ui_down[]=
{
	0x0084, 0x0080,
	0x0090, 0x0000,
	0x0094, 0x0fff,
	0x0098, 0x005C,
	0x009C, 0x0ff0,
	0x00AC, 0x0080,
	0x00B4, 0x0180,
	0x00C0, 0x0400,
	0x00C4, 0x7200,
	0x00C8, 0x008D,
	0x00D0, 0x00C0,
	0x0100, 0x0000,
	0x0084, 0x0040,
	END_SEQ, 0x0000,
};
#endif

u16 *pmDNIe_Gamma_set[] = {                         
	mDNIe_data_ui,//0
	mDNIe_data_300cd_level1,//01
	//mDNIe_data_300cd_level2,//02
	mDNIe_data_ui_down,//02
 };

#endif


#ifdef MDNIE_TUNING
u16 light_step = 0;
u16 saturation_step = 0;
u16 cs_step = 0;

u16 adc_level_formDNIe[6] = {0};
u16 mDNIe_data_level0[100] = {0};
u16 mDNIe_data_level1[100] = {0};
u16 mDNIe_data_level2[100] = {0};
u16 mDNIe_data_level3[100] = {0};
u16 mDNIe_data_level4[100] = {0};
u16 mDNIe_data_level5[100] = {0};

int mDNIe_data_level0_cnt = 0;
int mDNIe_data_level1_cnt = 0;
int mDNIe_data_level2_cnt = 0;
int mDNIe_data_level3_cnt = 0;
int mDNIe_data_level4_cnt = 0;
int mDNIe_data_level5_cnt = 0;


u16 mDNIe_data[100] = {0};


unsigned short *test[1];
//extern unsigned short *test[1];
int mdnie_tuning_load = 0;
EXPORT_SYMBOL(mdnie_tuning_load);

	
void mDNIe_txtbuf_to_parsing_for_lightsensor(void)
{
	int i = 0;
	int cnt;
		
	light_step = test[0][0];
	saturation_step = test[0][1];
	cs_step = test[0][2];

/*1 level */
	adc_level_formDNIe[0] = test[0][3];

	for(i=0; (test[0][i+4] != END_SEQ); i++)
	{
		if(test[0][i+4] != END_SEQ)
			mDNIe_data_level0[i] = test[0][i+4];
	}
	mDNIe_data_level0[i] = END_SEQ;
	mDNIe_data_level0_cnt = i;
	cnt = i+5;

/*2 level */
	adc_level_formDNIe[1] = test[0][cnt];
	cnt++;
	for(i=0; (test[0][cnt+i] != END_SEQ); i++)
	{
		if(test[0][cnt+i] != END_SEQ)
			mDNIe_data_level1[i] = test[0][cnt+i];
	}
	mDNIe_data_level1[i] = END_SEQ;
	mDNIe_data_level1_cnt = i;
	cnt += i+1;

/*3 level */
	adc_level_formDNIe[2] = test[0][cnt];
	cnt++;
	for(i=0; (test[0][cnt+i] != END_SEQ); i++)
	{
		if(test[0][cnt+i] != END_SEQ)
			mDNIe_data_level2[i] = test[0][cnt+i];
	}
	mDNIe_data_level2[i] = END_SEQ;
	mDNIe_data_level2_cnt = i;
	cnt += i+1;

/*4 level */
	adc_level_formDNIe[3] = test[0][cnt];
	cnt++;
	for(i=0; (test[0][cnt+i] != END_SEQ); i++)
	{
		if(test[0][cnt+i] != END_SEQ)
			mDNIe_data_level3[i] = test[0][cnt+i];
	}
	mDNIe_data_level3[i] = END_SEQ;
	mDNIe_data_level3_cnt = i;
	cnt += i+1;

/*5level */
	adc_level_formDNIe[4] = test[0][cnt];
	cnt++;
	for(i=0; (test[0][cnt+i] != END_SEQ); i++)
	{
		if(test[0][cnt+i] != END_SEQ)
			mDNIe_data_level4[i] = test[0][cnt+i];
	}
	mDNIe_data_level4[i] = END_SEQ;
	mDNIe_data_level4_cnt = i;
	cnt += i+1;

/*6 level */
	adc_level_formDNIe[5] = test[0][cnt];
	cnt++;
	for(i=0; (test[0][cnt+i] != END_SEQ); i++)
	{
		if(test[0][cnt+i] != END_SEQ)
			mDNIe_data_level5[i] = test[0][cnt+i];
	}
	mDNIe_data_level5[i] = END_SEQ;
	mDNIe_data_level5_cnt = i;
	cnt += i+1;

	
	mdnie_tuning_load = 1;
}
EXPORT_SYMBOL(mDNIe_txtbuf_to_parsing_for_lightsensor);

#endif


int s3c_mdnie_hw_init(void)
{
	printk("MDNIE  INIT ..........\n");

	printk(banner);

        s3c_mdnie_mem = request_mem_region(S3C_MDNIE_PHY_BASE,S3C_MDNIE_MAP_SIZE,"mdnie");
        if(s3c_mdnie_mem == NULL) {
                printk(KERN_ERR "MDNIE: failed to reserved memory region\n");
                return -ENOENT;
        }

        s3c_mdnie_base = ioremap(S3C_MDNIE_PHY_BASE,S3C_MDNIE_MAP_SIZE);
        if(s3c_mdnie_base == NULL) {
                printk(KERN_ERR "MDNIE failed ioremap\n");
                return -ENOENT;
        }

	/* clock */

	mdnie_clock = clk_get(NULL,"sclk_mdnie"); //to change sclk
	if (IS_ERR(mdnie_clock)) {
		printk("failed to get mdnie clock source\n");
		return -EINVAL;
	}

	printk("MDNIE  INIT SUCCESS Addr : 0x%p\n",s3c_mdnie_base);

	return 0;

}


int s3c_mdnie_mask(void)
{
	unsigned int mask;

	mask = s3c_mdnie_readl(S3C_MDNIE_rR1);
	mask |= S3C_MDNIE_REG_MASK;
	s3c_mdnie_writel(mask,S3C_MDNIE_rR1);
	
	return 0;
}

int s3c_mdnie_unmask(void)
{
	unsigned int mask;

	mask = s3c_mdnie_readl(S3C_MDNIE_rR1);
	mask &= ~S3C_MDNIE_REG_MASK;
	s3c_mdnie_writel(mask,S3C_MDNIE_rR1);
	
	return 0;
}

int s3c_mdnie_select_mode(int algo, int mcm, int lpa)
{
	s3c_mdnie_writel(0x0000,S3C_MDNIE_rR1);
	s3c_mdnie_writel(0x0000,S3C_MDNIE_rR36);
	s3c_mdnie_writel(0x0FFF,S3C_MDNIE_rR37);
	s3c_mdnie_writel(0x005c,S3C_MDNIE_rR38);

	s3c_mdnie_writel(0x0ff0,S3C_MDNIE_rR39);
	s3c_mdnie_writel(0x0064,S3C_MDNIE_rR43);
	s3c_mdnie_writel(0x0364,S3C_MDNIE_rR45);

	return 0;
}


int s3c_mdnie_set_size(unsigned int hsize, unsigned int vsize)
{
	
	unsigned int size;

	size = s3c_mdnie_readl(S3C_MDNIE_rR2);
	size &= ~S3C_MDNIE_SIZE_MASK;
	size |= hsize;
	s3c_mdnie_writel(size,S3C_MDNIE_rR2);
	
	size = s3c_mdnie_readl(S3C_MDNIE_rR3);
	size &= ~S3C_MDNIE_SIZE_MASK;
	size |= vsize;
	s3c_mdnie_writel(size,S3C_MDNIE_rR3);
	
	return 0;
}

int s3c_mdnie_setup(void)
{

	s3c_mdnie_hw_init();
	s3c_ielcd_hw_init();

	clk_enable(mdnie_clock);
	return 0;

}

void mDNIe_Mode_Change(mDNIe_data_type *mode)
{
	mutex_lock(&mdnie_use);

	if(mDNIe_Tuning_Mode == TRUE)
	{
		gprintk("mDNIe_Mode_Change [mDNIe_Tuning_Mode = TRUE, API is Return] \n");
		//return;
	}
	else
	{
		s3c_mdnie_mask();
		while ( mode->addr != END_SEQ)
		{
			s3c_mdnie_writel(mode->data, mode->addr);
			gprintk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n",mode->addr, mode->data);	
			mode++;
		}
		s3c_mdnie_unmask();
	}

	mutex_unlock(&mdnie_use);
}

void mDNIe_Set_Mode(Lcd_mDNIe_UI mode, u8 mDNIe_Outdoor_OnOff)
{
	if(mDNIe_Outdoor_OnOff)
	{
		switch(mode)
		{
			case mDNIe_UI_MODE:
				mDNIe_Mode_Change(mDNIe_UI);
			break;

			case mDNIe_VIDEO_MODE:
				mDNIe_Mode_Change(mDNIe_Outdoor_Mode);
			break;

			case mDNIe_VIDEO_WARM_MODE:
				mDNIe_Mode_Change(mDNIe_Video_WO_Mode);
			break;

			case mDNIe_VIDEO_COLD_MODE:
				mDNIe_Mode_Change(mDNIe_Video_CO_Mode);
			break;
			
			case mDNIe_CAMERA_MODE:
				mDNIe_Mode_Change(mDNIe_Camera_Outdoor_Mode);
			break;

			case mDNIe_NAVI:
				mDNIe_Mode_Change(mDNIe_Outdoor_Mode);
			break;
#if defined(CONFIG_ARIES_LATONA)
			case mDNIe_GALLERY:
				mDNIe_Mode_Change(mDNIe_Gallery);
			break;
#endif
		}

		current_mDNIe_UI = mode;

		if(current_mDNIe_UI == mDNIe_UI_MODE)
			current_mDNIe_OutDoor_OnOff = FALSE;
		else
			current_mDNIe_OutDoor_OnOff = TRUE;
	}
	else
	{
		switch(mode)
		{
			case mDNIe_UI_MODE:
				mDNIe_Mode_Change(mDNIe_UI);
			break;

			case mDNIe_VIDEO_MODE:
				mDNIe_Mode_Change(mDNIe_Video);
			break;

			case mDNIe_VIDEO_WARM_MODE:
				mDNIe_Mode_Change(mDNIe_Video_Warm);
			break;

			case mDNIe_VIDEO_COLD_MODE:
				mDNIe_Mode_Change(mDNIe_Video_Cold);
			break;
			
			case mDNIe_CAMERA_MODE:
				mDNIe_Mode_Change(mDNIe_Camera);
			break;

			case mDNIe_NAVI:
				mDNIe_Mode_Change(mDNIe_UI);
			break;
#if defined(CONFIG_ARIES_LATONA)
			case mDNIe_GALLERY:
				mDNIe_Mode_Change(mDNIe_Gallery);
			break;
#endif
		}
		
		current_mDNIe_UI = mode;
		current_mDNIe_OutDoor_OnOff = FALSE;
	}	

#if defined(CONFIG_ARIES_LATONA)
	on_cabc();
#endif


	pre_0x0100 = 0;
#ifdef CONFIG_FB_S3C_MDNIE_TUNINGMODE_FOR_BACKLIGHT
#if defined(CONFIG_S5PC110_DEMPSEY_BOARD)
        //extern int capella_pre_val;

        //capella_pre_val = -1;

#else
	pre_val = -1;		//Subhransu revisit for dempsey
#endif
#endif	/* CONFIG_FB_S3C_MDNIE_TUNINGMODE_FOR_BACKLIGHT */

	gprintk("[mDNIe] mDNIe_Set_Mode: current_mDNIe_UI(%d), current_mDNIe_OutDoor_OnOff(%d)  \n",current_mDNIe_UI, current_mDNIe_OutDoor_OnOff);	
}
EXPORT_SYMBOL(mDNIe_Set_Mode);

void mDNIe_Mode_Set(void)
{
	mDNIe_Set_Mode(current_mDNIe_UI, current_mDNIe_OutDoor_OnOff);
}
EXPORT_SYMBOL(mDNIe_Mode_Set);

static ssize_t mdnieset_ui_file_cmd_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	printk("called %s \n",__func__);

	int mdnie_ui = 0;

	switch(current_mDNIe_UI)
	{
		case mDNIe_UI_MODE:
		default:	
			mdnie_ui = 0;
			break;

		case mDNIe_VIDEO_MODE:
			mdnie_ui = 1;
			break;

		case mDNIe_VIDEO_WARM_MODE:
			mdnie_ui = 2;
			break;

		case mDNIe_VIDEO_COLD_MODE:
			mdnie_ui = 3;
			break;
		
		case mDNIe_CAMERA_MODE:
			mdnie_ui = 4;
			break;

		case mDNIe_NAVI:
			mdnie_ui = 5;
                        break;

#if defined(CONFIG_ARIES_LATONA)
		case mDNIe_GALLERY:
			mdnie_ui = 6;
			break;
#endif
	}
	return sprintf(buf,"%u\n",mdnie_ui);
}

static ssize_t mdnieset_ui_file_cmd_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
	int value;
	
    sscanf(buf, "%d", &value);

	//printk(KERN_INFO "[mdnie set] in mdnieset_ui_file_cmd_store, input value = %d \n",value);

	switch(value)
	{
		case SIG_MDNIE_UI_MODE:
			current_mDNIe_UI = mDNIe_UI_MODE;
			break;

		case SIG_MDNIE_VIDEO_MODE:
			current_mDNIe_UI = mDNIe_VIDEO_MODE;
			break;

		case SIG_MDNIE_VIDEO_WARM_MODE:
			current_mDNIe_UI = mDNIe_VIDEO_WARM_MODE;
			break;

		case SIG_MDNIE_VIDEO_COLD_MODE:
			current_mDNIe_UI = mDNIe_VIDEO_COLD_MODE;
			break;
		
		case SIG_MDNIE_CAMERA_MODE:
			current_mDNIe_UI = mDNIe_CAMERA_MODE;
			break;

		case SIG_MDNIE_NAVI:
			current_mDNIe_UI = mDNIe_NAVI;
			break;
#if defined(CONFIG_ARIES_LATONA)
		case SIG_MDNIE_GALLERY:
			current_mDNIe_UI = mDNIe_GALLERY;
			break;
#endif			
		default:
			printk("\nmdnieset_ui_file_cmd_store value is wrong : value(%d)\n",value);
			break;
	}

	mDNIe_Set_Mode(current_mDNIe_UI, current_mDNIe_OutDoor_OnOff);
		
	return size;
}

static DEVICE_ATTR(mdnieset_ui_file_cmd,0664, mdnieset_ui_file_cmd_show, mdnieset_ui_file_cmd_store);

static ssize_t mdnieset_outdoor_file_cmd_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	printk("called %s \n",__func__);

	return sprintf(buf,"%u\n",current_mDNIe_OutDoor_OnOff);
}

static ssize_t mdnieset_outdoor_file_cmd_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
	int value;
	
    sscanf(buf, "%d", &value);

	//printk(KERN_INFO "[mdnie set] in mdnieset_outdoor_file_cmd_store, input value = %d \n",value);

	if(value)
	{
		current_mDNIe_OutDoor_OnOff = TRUE;
	}
	else
	{
		current_mDNIe_OutDoor_OnOff = FALSE;
	}

	mDNIe_Set_Mode(current_mDNIe_UI, current_mDNIe_OutDoor_OnOff);
			
	return size;
}

static DEVICE_ATTR(mdnieset_outdoor_file_cmd,0664, mdnieset_outdoor_file_cmd_show, mdnieset_outdoor_file_cmd_store);

void init_mdnie_class(void)
{
	mdnieset_ui_class = class_create(THIS_MODULE, "mdnieset_ui");
	if (IS_ERR(mdnieset_ui_class))
		pr_err("Failed to create class(mdnieset_ui_class)!\n");

	switch_mdnieset_ui_dev = device_create(mdnieset_ui_class, NULL, 0, NULL, "switch_mdnieset_ui");
	if (IS_ERR(switch_mdnieset_ui_dev))
		pr_err("Failed to create device(switch_mdnieset_ui_dev)!\n");

	if (device_create_file(switch_mdnieset_ui_dev, &dev_attr_mdnieset_ui_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_mdnieset_ui_file_cmd.attr.name);

	mdnieset_outdoor_class = class_create(THIS_MODULE, "mdnieset_outdoor");
	if (IS_ERR(mdnieset_outdoor_class))
		pr_err("Failed to create class(mdnieset_outdoor_class)!\n");

	switch_mdnieset_outdoor_dev = device_create(mdnieset_outdoor_class, NULL, 0, NULL, "switch_mdnieset_outdoor");
	if (IS_ERR(switch_mdnieset_outdoor_dev))
		pr_err("Failed to create device(switch_mdnieset_outdoor_dev)!\n");

	if (device_create_file(switch_mdnieset_outdoor_dev, &dev_attr_mdnieset_outdoor_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_mdnieset_outdoor_file_cmd.attr.name);
}
EXPORT_SYMBOL(init_mdnie_class);



#ifdef MDNIE_TUNING
static u16 pre_0x00AC = 0;

static int pre_adc_level = 0;
static int cur_adc_level = 0;


static int mdnie_level;
static int init_mdnie = 0;

void mDNIe_Mode_set_for_lightsensor(u16 *buf)
{
	u32 i = 0;
	int cnt = 0;

	s3c_mdnie_mask();
if(cur_adc_level >= pre_adc_level)	//0 => END_SEQ
{
	while ( (*(buf+i)) != END_SEQ)
	{
		if((*(buf+i)) == 0x0100)
		{
			if(init_mdnie == 0)
			{
				pre_0x0100 = (*(buf+(i+1)));
			}
			if(pre_0x0100 < (*(buf+(i+1)))){
				while ((pre_0x0100 < (*(buf+(i+1))))&&(pre_0x0100 <= 0x8080)&&(pre_0x0100 >= 0x0000))
				{
					s3c_mdnie_writel(pre_0x0100, (*(buf+i)));
					printk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n",(*(buf+i)),pre_0x0100);
					pre_0x0100 = ((pre_0x0100 & 0xff00) + (light_step<<8)) | ((pre_0x0100 & 0x00ff) + (saturation_step));
				}
			}
			else if(pre_0x0100 > (*(buf+(i+1)))){
				while (pre_0x0100 > (*(buf+(i+1)))&&(pre_0x0100 >= 0x0000)&&(pre_0x0100 <= 0x8080))
				{
					s3c_mdnie_writel(pre_0x0100, (*(buf+i)));
					printk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n",(*(buf+i)),pre_0x0100);
					pre_0x0100 = ((pre_0x0100 & 0xff00) - (light_step<<8)) | ((pre_0x0100 & 0x00ff) - (saturation_step));
				}
			}
			s3c_mdnie_writel((*(buf+i+1)), (*(buf+i)));
			pre_0x0100 = (*(buf+i+1));
		}
		else if((*(buf+i)) == 0x00AC)
		{
			if(init_mdnie == 0)
			{
				pre_0x00AC = (*(buf+(i+1)));
			}
			if(pre_0x00AC < (*(buf+(i+1)))){
				while (pre_0x00AC < (*(buf+(i+1)))&&(pre_0x00AC <= 0x03ff)&&(pre_0x00AC >= 0x0000))
				{
					s3c_mdnie_writel(pre_0x00AC, (*(buf+i)));
					printk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n",(*(buf+i)),pre_0x00AC);
					pre_0x00AC +=(cs_step);
				}
			}
			else if(pre_0x00AC > (*(buf+(i+1)))){
				while (pre_0x00AC > (*(buf+(i+1)))&&(pre_0x00AC >= 0x0000)&&(pre_0x00AC <= 0x03ff))
				{
					s3c_mdnie_writel(pre_0x00AC, (*(buf+i)));
					printk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n",(*(buf+i)),pre_0x00AC);
					pre_0x00AC -=(cs_step);
				}
			}
			s3c_mdnie_writel((*(buf+i+1)), (*(buf+i)));
			pre_0x00AC = (*(buf+i+1));
		}
		else
		{
			s3c_mdnie_writel((*(buf+i+1)), (*(buf+i)));
		}
		printk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n",(*(buf+i)),(*(buf+(i+1))));	
		i+=2;
	}
}
else // if(cur_adc_level < pre_adc_level)  //END_SEQ => 0
{
		switch (cur_adc_level) {
		case 0:
			cnt = mDNIe_data_level0_cnt;
			break;
		case 1:
			cnt = mDNIe_data_level1_cnt;
			break;
		case 2:
			cnt = mDNIe_data_level2_cnt;
			break;
		case 3:
		default:
			cnt = mDNIe_data_level3_cnt;
			break;
		case 4:
			cnt = mDNIe_data_level4_cnt;
			break;
		case 5:
			cnt = mDNIe_data_level5_cnt;
			break;
		}

		cnt--;	//remove END_SEQ

		while ( cnt > 0)	
		{
			if((*(buf+cnt-1)) == 0x0100)
			{
				if(init_mdnie == 0)
				{
					pre_0x0100 = (*(buf+cnt));
				}
				if(pre_0x0100 < (*(buf+cnt))){
					while ((pre_0x0100 < (*(buf+cnt)))&&(pre_0x0100 <= 0x8080)&&(pre_0x0100 >= 0x0000))
					{
						s3c_mdnie_writel(pre_0x0100, (*(buf+cnt-1)));
						printk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n",(*(buf+cnt-1)),pre_0x0100);
						pre_0x0100 = ((pre_0x0100 & 0xff00) + (light_step<<8)) | ((pre_0x0100 & 0x00ff) + (saturation_step));
					}
				}
				else if(pre_0x0100 > (*(buf+cnt))){
					while (pre_0x0100 > (*(buf+cnt))&&(pre_0x0100 >= 0x0000)&&(pre_0x0100 <= 0x8080))
					{
						s3c_mdnie_writel(pre_0x0100, (*(buf+cnt-1)));
						printk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n",(*(buf+cnt-1)),pre_0x0100);
						pre_0x0100 = ((pre_0x0100 & 0xff00) - (light_step<<8)) | ((pre_0x0100 & 0x00ff) - (saturation_step));
					}
				}
				s3c_mdnie_writel((*(buf+cnt)), (*(buf+cnt-1)));
				pre_0x0100 = (*(buf+cnt));
			}
			else if((*(buf+cnt-1)) == 0x00AC)
			{
				if(init_mdnie == 0)
				{
					pre_0x00AC = (*(buf+cnt));
				}
				if(pre_0x00AC < (*(buf+cnt))){
					while (pre_0x00AC < (*(buf+cnt))&&(pre_0x00AC <= 0x03ff)&&(pre_0x00AC >= 0x0000))
					{
						s3c_mdnie_writel(pre_0x00AC, (*(buf+cnt-1)));
						printk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n",(*(buf+cnt-1)),pre_0x00AC);
						pre_0x00AC +=(cs_step);
					}
				}
				else if(pre_0x00AC > (*(buf+cnt))){
					while (pre_0x00AC > (*(buf+cnt))&&(pre_0x00AC >= 0x0000)&&(pre_0x00AC <= 0x03ff))
					{
						s3c_mdnie_writel(pre_0x00AC, (*(buf+cnt-1)));
						printk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n",(*(buf+cnt-1)),pre_0x00AC);
						pre_0x00AC -=(cs_step);
					}
				}
				s3c_mdnie_writel((*(buf+cnt)), (*(buf+cnt-1)));
				pre_0x00AC = (*(buf+cnt));
			}
			else
			{
				//s3c_mdnie_writel((*(buf+i+1)), (*(buf+i)));
				s3c_mdnie_writel((*(buf+cnt)), (*(buf+cnt-1)));
			}
			
			printk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n",(*(buf+cnt-1)),(*(buf+cnt)));

			cnt -=2;
		}
}
	s3c_mdnie_unmask();
}

int mdnie_lock = 0;

void mDNIe_Set_Register_for_lightsensor(int adc)
{

	if(init_mdnie == 0)
		pre_adc_level = cur_adc_level;
	
	if(!mdnie_lock){
		mdnie_lock = 1;
			
		if((adc >= adc_level_formDNIe[0])&&(adc < adc_level_formDNIe[1]))
		{
			cur_adc_level = 0;
			
			if(mdnie_level != 0)
				mDNIe_Mode_set_for_lightsensor(mDNIe_data_level0);

			mdnie_level = 0;
		}
		else if((adc >= adc_level_formDNIe[1])&&(adc < adc_level_formDNIe[2]))
		{
			cur_adc_level = 1;
			
			if(mdnie_level != 1)
				mDNIe_Mode_set_for_lightsensor(mDNIe_data_level1);
			
			mdnie_level = 1;
		}
		else if((adc >= adc_level_formDNIe[2])&&(adc < adc_level_formDNIe[3]))
		{
			cur_adc_level = 2;
			
			if(mdnie_level != 2)	
				mDNIe_Mode_set_for_lightsensor(mDNIe_data_level2);
			
			mdnie_level = 2;
		}
		else if((adc >= adc_level_formDNIe[3])&&(adc < adc_level_formDNIe[4]))
		{
			cur_adc_level = 3;
			
			if(mdnie_level != 3)
				mDNIe_Mode_set_for_lightsensor(mDNIe_data_level3);
			
			mdnie_level = 3;
		}
		else if((adc >= adc_level_formDNIe[4])&&(adc < adc_level_formDNIe[5]))
		{
			cur_adc_level = 4;
			
			if(mdnie_level != 4)
				mDNIe_Mode_set_for_lightsensor(mDNIe_data_level4);
			
			mdnie_level = 4;
		}
		else if(adc >= adc_level_formDNIe[5])
		{
			cur_adc_level = 5;
			
			if(mdnie_level != 5)
				mDNIe_Mode_set_for_lightsensor(mDNIe_data_level5);
			
			mdnie_level = 5;
		}

		pre_adc_level = cur_adc_level;
		
		init_mdnie = 1;

		mdnie_lock = 0;
	}
	else
		printk("[mDNIe] mDNIe_tuning -  mdnie_lock(%d) \n",mdnie_lock);	
}
EXPORT_SYMBOL(mDNIe_Set_Register_for_lightsensor);


void mDNIe_tuning_set(void)
{
	u32 i = 0;

	s3c_mdnie_mask();
	while ( mDNIe_data[i] != END_SEQ)
	{
		s3c_mdnie_writel(mDNIe_data[i+1], mDNIe_data[i]);
		printk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n",mDNIe_data[i], mDNIe_data[i+1]);	
		i+=2;
	}
	s3c_mdnie_unmask();
}

#if defined(CONFIG_ARIES_LATONA)
static int parse_text(char * src, int len)
{
	int i,count, ret;
	int index=0;
	char * str_line[100];
	char * sstart;
	char * c;
	unsigned int data1, data2;

	c = src;
	count = 0;
	sstart = c;
    
	for(i=0; i<len; i++,c++) 
    {
		char a = *c;
		if(a=='\r' || a=='\n') 
        {
			if(c > sstart) 
            {
				str_line[count] = sstart;
				count++;
			}
			*c='\0';
			sstart = c+1;
		}
	}
    
	if(c > sstart) 
    {
		str_line[count] = sstart;
		count++;
	}

	printk("----------------------------- Total number of lines:%d\n", count);

	for(i=0; i<count; i++) 
    {
		printk("line:%d, [start]%s[end]\n", i, str_line[i]);
		ret = sscanf(str_line[i], "0x%x,0x%x\n", &data1, &data2);
		printk("Result => [0x%2x 0x%4x] %s\n", data1, data2, (ret==2)?"Ok":"Not available");
		if(ret == 2) 
        {   
			mDNIe_data[index++] = (u16)data1;
			mDNIe_data[index++]  = (u16)data2;
		}
	}
	return index;
}

static int parse_text2(char * src, int len)
{
	int i,count, ret, real_count;
	int index=0;
	char * str_line[1000];
	char * sstart;
	char * c;
	char mode_name[20];
	unsigned int data1, data2;
	mDNIe_data_type *mode;

	c = src;
	count = 0;
	sstart = c;
	real_count = 0;
    
	for(i=0; i<len; i++,c++) 
    {
		char a = *c;
		if(a=='\r' || a=='\n') 
        {
			if(c > sstart) 
            {
				str_line[count] = sstart;
				count++;
			}
			*c='\0';
			sstart = c+1;
			
			real_count++;
		}
	}
    
	if(c > sstart) 
    {
		str_line[count] = sstart;
		count++;
	}

	printk("----------------------------- Total number of lines:%d\n", count);
	
	for(i=0; i<count; i++) 
    {
		ret = sscanf(str_line[i], "mDNIe_data_type %s[]= \n", mode_name);
		if(ret == 1)
		{
			*(strchr(mode_name,'[')) = 0;
			
			if(strcmp(mode_name,"mDNIe_Video")==0)
			{
				mode = mDNIe_Video;
			}
			else if(strcmp(mode_name,"mDNIe_Camera")==0)
			{
				mode = mDNIe_Camera;
			}
			else if(strcmp(mode_name,"mDNIe_Camera_Outdoor_Mode")==0)
			{
				mode = mDNIe_Camera_Outdoor_Mode;
			}
			else if(strcmp(mode_name,"mDNIe_UI")==0)
			{
				mode = mDNIe_UI;
			}
			else if(strcmp(mode_name,"mDNIe_Video_Warm")==0)
			{
				mode = mDNIe_Video_Warm;
			}
			else if(strcmp(mode_name,"mDNIe_Video_WO_Mode")==0)
			{
				mode = mDNIe_Video_WO_Mode;
			}
			else if(strcmp(mode_name,"mDNIe_Video_Cold")==0)
			{
				mode = mDNIe_Video_Cold;
			}
			else if(strcmp(mode_name,"mDNIe_Video_CO_Mode")==0)
			{
				mode = mDNIe_Video_CO_Mode;
			}
			else if(strcmp(mode_name,"mDNIe_Outdoor_Mode")==0)
			{
				mode = mDNIe_Outdoor_Mode;
			}
			else if(strcmp(mode_name,"mDNIe_Gallery")==0)
			{
				mode = mDNIe_Gallery;
			}
			else
			{
				mode = 0;
			}
			//printk("mode name : %s - %x\n",mode_name,mode);
		}

		while(*str_line[i] == '\t' || *str_line[i] == ' ')
		{
			str_line[i]++;
		}
		printk("line:%d, [start]%s[end]\n", i, str_line[i]);
		ret = sscanf(str_line[i], "0x%x,0x%x\n", &data1, &data2);
		printk("Result => [0x%2x 0x%4x] %s\n", data1, data2, (ret==2)?"Ok":"Not available");
		if(ret == 2 && mode != 0) 
        {
			mode->addr = (u16)data1;
			mode->data = (u16)data2;
			mode++;
		}

		if(str_line[i][0] == '}')
		{
			if(mode != 0)
			{
				mode->addr = (u16)END_SEQ;
				mode->data = (u16)0x0000;
			}
		}
	}
	return real_count+1;
}

#endif

void mDNIe_txtbuf_to_parsing(void)
{
	int i = 0;

	for(i=0; (test[0][i] != END_SEQ); i++)
	{
		if(test[0][i] != END_SEQ)
			mDNIe_data[i] = test[0][i];
	}
	mDNIe_data[i] = END_SEQ;

	mDNIe_tuning_set();

	mDNIe_Tuning_Mode = TRUE;
}
EXPORT_SYMBOL(mDNIe_txtbuf_to_parsing);

#if defined(CONFIG_ARIES_LATONA)
int mDNIe_txtbuf_to_parsing2(void)
{
	struct file *filp;
	char	*dp;
	long	l, i ;
	loff_t  pos;
	int     ret, num,type;
	mm_segment_t fs;

	printk("cmc623_load_data start!\n");

	fs = get_fs();
	set_fs(get_ds());

	filp = filp_open("sdcard/external_sd/s3cfb_mdnie_value.h", O_RDONLY, 0);
	type = 2;
	
	if(IS_ERR(filp))
	{
		filp = filp_open("sdcard/external_sd/mdnie_tune", O_RDONLY, 0);
		type = 1;

		if(IS_ERR(filp)) 
		{
			printk("file open error:%d\n", (s32)filp);

			return -1;
		}
	}

	l = filp->f_path.dentry->d_inode->i_size;
	printk("Size of the file : %ld(bytes)\n", l);

	//dp = kmalloc(l, GFP_KERNEL);
	dp = kmalloc(l+10, GFP_KERNEL);		// add cushion
	if(dp == NULL) 
    {
		printk("Out of Memory!\n");
		filp_close(filp, current->files);
		return -1;
	}
	pos = 0;
	memset(dp, 0, l);
    printk("== Before vfs_read ======\n");
	ret = vfs_read(filp, (char __user *)dp, l, &pos);   // P1_LSJ : DE08 : ?�기??죽음 
    printk("== After vfs_read ======\n");

	if(ret != l) 
    {
		printk("<CMC623> Failed to read file (ret = %d)\n", ret);
		kfree(dp);
		filp_close(filp, current->files);
		return -1;
	}

	filp_close(filp, current->files);

	set_fs(fs);
/*
	for(i=0; i<l; i++)
    {   
		printk("%x ", dp[i]);
    }
	printk("\n");
//*/	
	if(type == 1)
	{
		num = parse_text(dp, l);

		if(!num) 
		{
			printk("Nothing to parse!\n");
			kfree(dp);
			return -1;
		}
			
		printk("------ Jun Total number of parsed lines: %d\n", num);

		mDNIe_data[num] = END_SEQ;

		printk("read ok\n");
		mDNIe_tuning_set();
		printk("tuning set ok\n");

		mDNIe_Tuning_Mode = TRUE;
	}
	else if(type == 2)
	{
		num = parse_text2(dp, l);
		printk("read ok\n");
		mDNIe_Set_Mode(current_mDNIe_UI,current_mDNIe_OutDoor_OnOff);
		num *= 2;
		printk("tuning set ok\n");

		mDNIe_Tuning_Mode = FALSE;
	}

	kfree(dp);
	
	return num / 2;
}
EXPORT_SYMBOL(mDNIe_txtbuf_to_parsing2);
#endif

#endif

#if defined(CONFIG_FB_S3C_MDNIE_TUNINGMODE_FOR_BACKLIGHT)// && defined (CONFIG_S5PC110_DEMPSEY_BOARD)
int mdnie_tuning_backlight = 0;

extern int IsLDIEnabled(void);
void mDNIe_Mode_set_for_backlight(u16 *buf)
{
	u32 i = 0;
	int cnt = 0;

	if(IsLDIEnabled())
	{
		mutex_lock(&mdnie_use);

		//if(mdnie_tuning_backlight)
		{
			s3c_mdnie_mask();

			while ((*(buf+i)) != END_SEQ)
			{
				if((*(buf+i)) == 0x0100)
				{
					if(pre_0x0100 < (*(buf+(i+1)))){
						while ((pre_0x0100 < (*(buf+(i+1))))&&(pre_0x0100 <= 0x4020)&&(pre_0x0100 >= 0x0000))
						{
							s3c_mdnie_writel(pre_0x0100, (*(buf+i)));
							gprintk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n",(*(buf+i)),pre_0x0100);

							if((pre_0x0100 & 0x00ff) == 0x20)
							{
								pre_0x0100 = ((pre_0x0100 & 0xff00) + (0x8<<8)) | (0x20);
							}
							else
							{
								pre_0x0100 = ((pre_0x0100 & 0xff00) + (0x8<<8)) | ((pre_0x0100 & 0x00ff) + (0x4));
							}
							msleep(20);
						}
					}
					else if(pre_0x0100 > (*(buf+(i+1)))){
						while (pre_0x0100 > (*(buf+(i+1)))&&(pre_0x0100 >= 0x0000)&&(pre_0x0100 <= 0x4020))
						{
							s3c_mdnie_writel(pre_0x0100, (*(buf+i)));
							gprintk("[mDNIe] mDNIe_tuning_initialize: addr(0x%x), data(0x%x)  \n",(*(buf+i)),pre_0x0100);
							
							if((pre_0x0100 & 0x00ff) == 0x00)
							{
								pre_0x0100 = ((pre_0x0100 & 0xff00) - (0x8<<8)) | (0x00);
							}
							else
							{
								pre_0x0100 = ((pre_0x0100 & 0xff00) - (0x8<<8)) | ((pre_0x0100 & 0x00ff) - (0x4));
							}
							msleep(20);
						}
					}
					s3c_mdnie_writel((*(buf+i+1)), (*(buf+i)));
					pre_0x0100 = (*(buf+i+1));
				}
				else
				{
					s3c_mdnie_writel((*(buf+i+1)), (*(buf+i)));
				}
				gprintk("[mDNIe] mDNIe_Mode_set_for_backlight : addr(0x%x), data(0x%x)  \n",(*(buf+i)),(*(buf+(i+1))));	
				i+=2;
			}

			s3c_mdnie_unmask();
		}

		mutex_unlock(&mdnie_use);
	}	
}
EXPORT_SYMBOL(mDNIe_Mode_set_for_backlight);

#if 0
void mDNIe_txtbuf_to_parsing_for_backlight(void)
{
	int i = 0;
	int cnt = 0;


	for(i=0; (test[0][i] != END_SEQ); i++)
	{
		if(test[0][i] != END_SEQ)
			mDNIe_data_ui[i] = test[0][i];
	}
	mDNIe_data_ui[i] = END_SEQ;
	cnt = i+1;

	for(i=0; (test[0][cnt+i] != END_SEQ); i++)
	{
		if(test[0][cnt+i] != END_SEQ)
			mDNIe_data_300cd_level1[i] = test[0][cnt+i];
	}
	mDNIe_data_300cd_level1[i] = END_SEQ;
	cnt += i+1;

	for(i=0; (test[0][cnt+i] != END_SEQ); i++)
	{
		if(test[0][cnt+i] != END_SEQ)
			mDNIe_data_300cd_level2[i] = test[0][cnt+i];
	}
	mDNIe_data_300cd_level2[i] = END_SEQ;
	cnt += i+1;

	for(i=0; (test[0][cnt+i] != END_SEQ); i++)
	{
		if(test[0][cnt+i] != END_SEQ)
			mDNIe_data_ui_down[i] = test[0][cnt+i];
	}
	mDNIe_data_ui_down[i] = END_SEQ;
	cnt += i+1;

	mdnie_tuning_backlight = 1;

	mDNIe_Mode_set_for_backlight(pmDNIe_Gamma_set[0]);
}
EXPORT_SYMBOL(mDNIe_txtbuf_to_parsing_for_backlight);
#endif
#endif

int s3c_mdnie_init_global(struct s3cfb_global *s3cfb_ctrl)
{

	// clk enable
	clk_enable(mdnie_clock);

	s3c_mdnie_set_size(s3cfb_ctrl->lcd->width,s3cfb_ctrl->lcd->height);

	mDNIe_Set_Mode(current_mDNIe_UI, current_mDNIe_OutDoor_OnOff); //Add
	
	s3c_ielcd_logic_start();
	s3c_ielcd_init_global(s3cfb_ctrl);

	return 0;

}

int s3c_mdnie_start(struct s3cfb_global *ctrl)
{

	//s3c_ielcd_set_clock(ctrl);
	s3c_ielcd_start();

	return 0;

}

int s3c_mdnie_off(void)
{

	s3c_ielcd_logic_stop();
	clk_disable(mdnie_clock);

	return 0;
}


int s3c_mdnie_stop(void)
{

	s3c_ielcd_stop();

	return 0;

}


MODULE_AUTHOR("lsi");
MODULE_DESCRIPTION("S3C MDNIE Device Driver");
MODULE_LICENSE("GPL");
