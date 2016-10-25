#ifndef __MARVIN_CDEV_H
#define __MARVIN_CDEV_H

// include file
#include<linux/init.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/pci.h>
#include<linux/cdev.h>
#include<linux/interrupt.h>
#include<linux/fs.h>

// define
#define SHN_NAME_LEN 128

#define PCI_ANY_ID (~0)

// struct
struct shn_cdev {
	struct cdev cdev;
	struct pci_dev *pci_dev;

	char shn_cdev_name[SHN_NAME_LEN];
	int major;
	int minor;

	resource_size_t pci_bar0_phyaddr;
	unsigned long pci_bar0_len;
	u32 __iomem *pci_bar0_viraddr;

	int luns_ce;
	int nchannels;
	int threads_channel;
	int luns_thread;

	unsigned int irqno;
	int shn_irq;

	struct tasklet_struct tasklet;
};

//function

#endif
