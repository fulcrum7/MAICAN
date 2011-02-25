/*
*	This code is strongly coupled with Linux main line code and uses much
*	from other spi master drivers. All kernel style comments and GPL links
*       will be added soon. Also "title"-style comments will be used in project.
*	
**/


#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/parport.h>
#include <linux/sysfs.h>                 //???
#include <linux/workqueue.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>







/******************************************************************************
*			Module INFO
******************************************************************************/

MODULE_AUTHOR("Alyautdin R.T.");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("BSP for Kontron SPI");



/******************************************************************************
*			Module GLOBAL VARIABLES
******************************************************************************/
#define DRVNAME "SPI_MOD"

struct spi_kontron {	
			struct spi_bitbang	bitbang;
			struct parport		*port;
			struct pardevice	*pd;
			struct spi_device	*spidev_kontron;
			struct spi_board_info	info;
						
		   };
		   
static struct spi_kontron *pkontron;
/******************************************************************************
*			Module FUNCTIONS FOR SPI_BITBANG 
******************************************************************************/


/******************************************************************************
*			Module PARPORT METHODS
******************************************************************************/
static void spi_kontron_attach(struct parport *p)
{
	struct pardevice	*pd;
	struct spi_kontron	*pp;
	struct spi_master	*master;
	int			status;

	if (pkontron)
	{
	      printk(KERN_WARNING
			 "%s: spi_kontron instance already loaded. Aborting.\n",
			 DRVNAME);
	      return;
	}

	

	master = spi_alloc_master(p->physport->dev, sizeof *pp);
	if (!master)
	{
		status = -ENOMEM;
		printk(KERN_WARNING
			 "%s: spi master isn't allocated \n",
			 DRVNAME);
		printk(KERN_WARNING
			 "%s: returns %d \n",
			 DRVNAME,status);
		return;
	}
	pp = spi_master_get_devdata(master);

	master->bus_num = -1;	/* dynamic alloc of a bus number */
	master->num_chipselect = 1;				/*REVISIT*/

	/*
	 * SPI and bitbang hookup.
	 */
	pp->bitbang.master = spi_master_get(master);
	pp->bitbang.chipselect = kontron_chipselect;
	pp->bitbang.txrx_word[SPI_MODE_0] = kontron_txrx;	/*REVISIT*/
	/*pp->bitbang.flags = SPI_3WIRE;*/			/*REVISIT*/

	/*
	 * Parport hookup
	 */
	pp->port = p;
	pd = parport_register_device(p, DRVNAME,
			NULL, NULL, NULL,
			PARPORT_FLAG_EXCL, pp);			/*REVISIT*/
	if (!pd)
	{
		status = -ENOMEM;
		(void) spi_master_put(master);
		printk(KERN_WARNING
			 "%s: can't register parport device \n",
			 DRVNAME);
		printk(KERN_WARNING
			 "%s: returns %d \n",
			 DRVNAME,status);
		return;	 
	}
	pp->pd = pd;

	status = parport_claim(pd);
	if (status < 0)
	{
		printk(KERN_WARNING
			 "%s: no access to parport \n",
			 DRVNAME);
		printk(KERN_WARNING
			 "%s: returns %d \n",
			 DRVNAME,status);
		parport_unregister_device(pd);
		(void) spi_master_put(master);
		return;
	}	

	/*
	 * Start SPI ...
	 */
	status = spi_bitbang_start(&pp->bitbang);
	if (status < 0)
	{
		printk(KERN_WARNING
			"%s: spi_bitbang_start failed with status %d\n",
			DRVNAME, status);
		
		/* power down */
		parport_write_data(pp->port, 0);
		mdelay(10);
		parport_release(pp->pd);
		parport_unregister_device(pd);
		(void) spi_master_put(master);
		return;
	}

	/*
	 * The modalias name MUST match the device_driver name
	 * for the bus glue code to match and subsequently bind them.
	 * We are binding to the generic drivers/hwmon/lm70.c device
	 * driver.
	 */
	strcpy(pp->info.modalias, "lm70");			/*REVISIT*/
	pp->info.max_speed_hz = 6 * 1000 * 1000;		/*REVISIT*/
	pp->info.chip_select = 0;				/*REVISIT*/
	/*pp->info.mode = SPI_3WIRE | SPI_MODE_0;*/  		/*REVISIT*/

	/* power up the chip, and let the LM70 control SI/SO */
	parport_write_data(pp->port, kontron_INIT);

	/* Enable access to our primary data structure via
	 * the board info's (void *)controller_data.
	 */
	pp->info.controller_data = pp;
	pp->spidev_kontron = spi_new_device(pp->bitbang.master, &pp->info);
	if (pp->spidev_kontron)
	{
	 /*	dev_dbg(&pp->spidev_kontron->dev, "spidev_kontron at %s\n",
				dev_name(&pp->spidev_kontron->dev));*/
	}
	else
	{
		printk(KERN_WARNING "%s: spi_new_device failed\n", DRVNAME);
		status = -ENODEV;
		spi_bitbang_stop(&pp->bitbang);
		printk(KERN_WARNING
			"%s: returns %d\n",
			DRVNAME, status);
		
		/* power down */
		parport_write_data(pp->port, 0);
		mdelay(10);
		parport_release(pp->pd);
		parport_unregister_device(pd);
		(void) spi_master_put(master);
		return;
		
	}
	pp->spidev_kontron->bits_per_word = 8;		/*REVISIT*/

	pkontron = pp;
	return;
}

static void spi_kontron_detach(struct parport *p)
{
	struct spi_kontron		*pp;

	if (!pkontron || pkontron->port != p)
		return;

	pp = pkontron;
	spi_bitbang_stop(&pp->bitbang);

	/* power down */
	parport_write_data(pp->port, 0);

	parport_release(pp->pd);
	parport_unregister_device(pp->pd);

	(void) spi_master_put(pp->bitbang.master);

	pkontron = NULL;
}

/******************************************************************************
*			Initialization and registration 
******************************************************************************/

static struct parport_driver spi_kontron = {
              					.name = DRVNAME,
						.attach = spi_kontron_attach,
						.detach = spi_kontron_detach,
					   };

static int __init init_spi_kontron(void)
{
	return parport_register_driver(&spi_kontron);
}


static void __exit cleanup_spi_kontron(void)
{
	parport_unregister_driver(&spi_kontron);
}

module_init(init_spi_kontron);
module_exit(cleanup_spi_kontron);
