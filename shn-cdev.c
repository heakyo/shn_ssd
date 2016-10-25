#include"shn-cdev.h"

static int shn_cdev_open(struct inode *inode, struct file *filp)
{
	printk("%s\n",__func__);
	return 0;
}

static int shn_cdev_release(struct inode *inode, struct file *filp)
{
	printk("%s\n",__func__);
	return 0;
}

static ssize_t shn_cdev_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	printk("%s\n",__func__);
	
	printk("buf:%s, size:%ld\n", buf, size);

	return size;
}

static ssize_t shn_cdev_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
	char str[] = "shn_cdev_read_test";
	int ret = 0;

	printk("%s\n",__func__);

	if (copy_to_user(buf, str, size)) {
		ret = -EFAULT;
	} else {
		ret = size;
	}
	printk("buf:%s, size:%ld ret:%d\n", buf, size, ret);

	return ret;
}

static const struct file_operations shn_cdev_fops = {
	.owner = THIS_MODULE,
	.open  = shn_cdev_open,
	.release = shn_cdev_release,
	.write = shn_cdev_write,
	.read = shn_cdev_read,
};

static int register_shn_cdev(struct shn_cdev *cdev)
{
	int ret = 0;
	dev_t devno;

	ret = alloc_chrdev_region(&devno, 0, 1, cdev->shn_cdev_name);	
	if (ret < 0)
		goto out;

	cdev->major = MAJOR(devno);
	cdev->minor = MINOR(devno);
	printk("major:%d minor:%d\n", cdev->major, cdev->minor);

	cdev_init(&cdev->cdev, &shn_cdev_fops);
	cdev->cdev.owner = THIS_MODULE;
	ret = cdev_add(&cdev->cdev, devno, 1);
	if (ret) {
		printk("add shannon cdev failed!\n");
		goto err_alloc;
	}

	return 0;

err_alloc:
	unregister_chrdev_region(MKDEV(cdev->major, cdev->minor), 1);

out:
	return ret;
}

static void unregister_shn_cdev(struct shn_cdev *cdev)
{
	cdev_del(&cdev->cdev);
	unregister_chrdev_region(MKDEV(cdev->major, cdev->minor), 1);
}

static void shn_tasklet(unsigned long data)
{
	printk("%s: success\n", __func__);
}

static irqreturn_t shn_cdev_interrupt(int irq, void *dev_id)
{
	struct shn_cdev *cdev = (struct shn_cdev *)dev_id;
	printk("%s\n",__func__);

	if(!cdev->shn_irq)
		return IRQ_NONE;
		
	tasklet_schedule(&cdev->tasklet);
	
	return IRQ_HANDLED;
}

static int shn_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int status = 0;
	int bar = 0;
	u8 pci_irq_line;
	u8 pci_irq_pin;
	

	struct shn_cdev *cdev;

	printk("%s start\n",__func__);
	
	// allocate struct shn_cdev room space
	cdev = kzalloc(sizeof(*cdev), GFP_KERNEL);
	if (!cdev) {
		status = -ENOMEM;
		goto err_kzalloc_cdev;
	}
	cdev->pci_dev = pdev;

	pci_set_drvdata(pdev, cdev);
	strcpy(cdev->shn_cdev_name, "shn_cdev");

	status = pci_enable_device(pdev);
	if (status)
		goto err_enable_pci_dev;

	// set pci device to a master device
	pci_set_master(pdev);

	// need to be modified
	status = pci_request_region(pdev, bar, cdev->shn_cdev_name);
	if (status)
		goto err_req_region;

	cdev->pci_bar0_phyaddr = pci_resource_start(pdev, bar);
	cdev->pci_bar0_len = pci_resource_len(pdev, bar);
	printk("pci_bar0_phyaddr: %llx pci_bar0_len: %lx\n", cdev->pci_bar0_phyaddr, cdev->pci_bar0_len);

	cdev->pci_bar0_viraddr = ioremap(cdev->pci_bar0_phyaddr, cdev->pci_bar0_len);
	if (!cdev->pci_bar0_viraddr) {
		status = -ENOMEM;
		goto err_bar0_ioremap;
	}

	cdev->luns_ce = (ioread32(cdev->pci_bar0_viraddr)>>12 & 0xf) + 1;
	cdev->nchannels = ioread32(cdev->pci_bar0_viraddr)>>16 & 0xff;
	cdev->threads_channel = ioread32(cdev->pci_bar0_viraddr)>>24 & 0xf;
	cdev->luns_thread = (ioread32(cdev->pci_bar0_viraddr)>>28 & 0xf) + 1;
	printk("luns_ce:%d, nchannels:%d, threads_channel:%d, luns_thread:%d\n", 
		cdev->luns_ce, cdev->nchannels, cdev->threads_channel, cdev->luns_thread);

	iowrite32((ioread32(cdev->pci_bar0_viraddr+0xC0) & (~0x02000000)), cdev->pci_bar0_viraddr+0xC0);
	printk("DMA queue flag: %x\n",ioread32(cdev->pci_bar0_viraddr+0xC0)>>25 & 0x1);

	// need to set dma mask
	// ToDo

	//pci_enable_msi(pdev);
	cdev->irqno = pdev->irq;
	printk("irq no:%d\n", cdev->irqno);

	printk("cdev:%p\n", cdev);
	printk("shn_cdev_name: %s\n",cdev->shn_cdev_name);
	status = request_irq(cdev->irqno, shn_cdev_interrupt, IRQF_SHARED, cdev->shn_cdev_name, cdev);
	status = 0;
	if (status) {
		printk("request irq failed!--status:%d\n", status);
		goto err_req_irq;
	}

#if 1
	if(pci_read_config_byte(cdev->pci_dev, PCI_INTERRUPT_LINE, &pci_irq_line) || 
		pci_read_config_byte(cdev->pci_dev, PCI_INTERRUPT_PIN, &pci_irq_pin)) {
		status = -EIO;
		goto err_read_config;
	}
	printk("pci_irq_line:%d pci_irq_pin:%d\n", pci_irq_line, pci_irq_pin);
#endif		

	// init shn_cdev
	if(register_shn_cdev(cdev)) {
		printk("init shannon cdev failed!\n");
		goto err_init_shn_cdev;
	}
	
	tasklet_init(&cdev->tasklet, shn_tasklet, (unsigned long)cdev);

	printk("%s: success\n", __func__);
	
	return 0;

err_init_shn_cdev:
err_read_config:
	free_irq(cdev->irqno, cdev);
	//pci_disable_msi(pdev);

err_req_irq:
	iounmap(cdev->pci_bar0_viraddr);
	
err_bar0_ioremap:
	pci_release_regions(pdev);

err_req_region:
	pci_disable_device(pdev);

err_enable_pci_dev:
	kfree(cdev);
	
err_kzalloc_cdev:

	return status;
}

static void shn_remove(struct pci_dev *pdev)
{
	struct shn_cdev *cdev = NULL;
	
	printk("%s\n",__func__);

	cdev = (struct shn_cdev *)pci_get_drvdata(pdev);
	
	unregister_shn_cdev(cdev);

	printk("irq no:%d\n", cdev->irqno);
	free_irq(cdev->irqno, cdev);
	//pci_disable_msi(pdev);
	iounmap(cdev->pci_bar0_viraddr);
	pci_release_regions(pdev);
	pci_disable_device(pdev);
	kfree(cdev);

	printk("%s: success\n", __func__);
}

static const struct pci_device_id shn_pci_tbl[] = {
	{0x1CB0, 0x0275, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0UL},
	{0,}
};

static struct pci_driver shn_cdev_pci_driver = {
	.name		= "shn_cdev",
	.id_table	= shn_pci_tbl,
	.probe		= shn_probe,
	.remove		= shn_remove,
};

static int __init shannon_init(void)
{
	printk("%s\n",__func__);

	pci_register_driver(&shn_cdev_pci_driver);	
	return 0;
}

static void __exit shannon_exit(void)
{
	printk("%s\n",__func__);
	pci_unregister_driver(&shn_cdev_pci_driver);	
}

module_init(shannon_init);
module_exit(shannon_exit);

MODULE_AUTHOR("Marvin");
MODULE_LICENSE("GPL");
