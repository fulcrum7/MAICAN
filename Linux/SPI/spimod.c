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
		goto out_free_master;
	}
	pp->pd = pd;

	status = parport_claim(pd);
	if (status < 0)
		goto out_parport_unreg;

	/*
	 * Start SPI ...
	 */
	status = spi_bitbang_start(&pp->bitbang);
	if (status < 0) {
		printk(KERN_WARNING
			"%s: spi_bitbang_start failed with status %d\n",
			DRVNAME, status);
		goto out_off_and_release;
	}

	/*
	 * The modalias name MUST match the device_driver name
	 * for the bus glue code to match and subsequently bind them.
	 * We are binding to the generic drivers/hwmon/lm70.c device
	 * driver.
	 */
	strcpy(pp->info.modalias, "lm70");
	pp->info.max_speed_hz = 6 * 1000 * 1000;
	pp->info.chip_select = 0;
	pp->info.mode = SPI_3WIRE | SPI_MODE_0;

	/* power up the chip, and let the LM70 control SI/SO */
	parport_write_data(pp->port, lm70_INIT);

	/* Enable access to our primary data structure via
	 * the board info's (void *)controller_data.
	 */
	pp->info.controller_data = pp;
	pp->spidev_lm70 = spi_new_device(pp->bitbang.master, &pp->info);
	if (pp->spidev_lm70)
		dev_dbg(&pp->spidev_lm70->dev, "spidev_lm70 at %s\n",
				dev_name(&pp->spidev_lm70->dev));
	else {
		printk(KERN_WARNING "%s: spi_new_device failed\n", DRVNAME);
		status = -ENODEV;
		goto out_bitbang_stop;
	}
	pp->spidev_lm70->bits_per_word = 8;

	lm70llp = pp;
	return;

out_bitbang_stop:
	spi_bitbang_stop(&pp->bitbang);
out_off_and_release:
	/* power down */
	parport_write_data(pp->port, 0);
	mdelay(10);
	parport_release(pp->pd);
out_parport_unreg:
	parport_unregister_device(pd);
out_free_master:
	(void) spi_master_put(master);
}

static void spi_kontron_detach(struct parport *p)
{
	struct spi_lm70llp		*pp;

	if (!lm70llp || lm70llp->port != p)
		return;

	pp = lm70llp;
	spi_bitbang_stop(&pp->bitbang);

	/* power down */
	parport_write_data(pp->port, 0);

	parport_release(pp->pd);
	parport_unregister_device(pp->pd);

	(void) spi_master_put(pp->bitbang.master);

	lm70llp = NULL;
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
