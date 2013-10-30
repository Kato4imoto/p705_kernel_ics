/*
 * arch/arm/mach-msm/lge/lge_nv.c
 *
 * Copyright (C) 2012 LGE, Inc
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/platform_device.h>
#include <asm/io.h>
#include <linux/slab.h>
#include <mach/board_lge.h>

static int msm_nv_frststatus_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    unsigned char frststatus;

    frststatus = (unsigned char)lge_get_nv_frststatus();

    return sprintf(buf, "%d\n", frststatus);
}

static int msm_nv_frststatus_store(struct device *dev,
                    struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned char frststatus = (unsigned char)simple_strtoul(buf, NULL, 10);

    lge_set_nv_frststatus(frststatus);

    return count;
}
static DEVICE_ATTR(frststatus, 0660, msm_nv_frststatus_show, msm_nv_frststatus_store);

static int __init lge_nv_device_probe(struct platform_device *pdev)
{
    int err;

    err = device_create_file(&pdev->dev, &dev_attr_frststatus);
    if (err < 0)
        printk("%s : Cannot create the sysfs FRSTSTATUS\n", __func__);

    return err;
}

static struct platform_device lgenv_device = {
    .name = "lgenv",
    .id     = -1,
};

static struct platform_driver this_driver __refdata = {
    .probe = lge_nv_device_probe,
    .driver = {
        .name = "lgenv",
    },
};

int __init lge_nvdevice_init(void)
{
    printk("%s\n", __func__);
    platform_device_register(&lgenv_device);

    return platform_driver_register(&this_driver);
}

module_init(lge_nvdevice_init);
MODULE_DESCRIPTION("read and write nv items not using qcril");
MODULE_AUTHOR("Dongwon Choi<dongwon.choi@lge.com>");
MODULE_LICENSE("GPL");
                                                                 
