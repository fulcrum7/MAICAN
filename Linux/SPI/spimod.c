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
#include <linux/workqueue.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/can/platform/mcp251x.h> //specific for mcp 






/******************************************************************************
*			Module INFO
******************************************************************************/

MODULE_AUTHOR("Alyautdin R.T.");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Spi contoller driver for mcp2515");

/******************************************************************************
*			Module PROC for debugging
******************************************************************************/
/*deb*/
#include <linux/proc_fs.h>
unsigned char procbuffer[550];
unsigned long countTX=0;
unsigned long countCS=0;
unsigned long countMOSI=0;
unsigned long countMISO=0;
#define PROC_NAME "spi_kontron"
int procfile_read(char *buf, char **start,
		  off_t offset,int count,int *eof, void *data)
{
	int len;
	printk(KERN_ALERT "PROC is called");
	len=sprintf(buf,"Jiffies=%lu\nCountTx=%lu\nCountCS=%lu\n\
			CountMOSI=%lu\nCountMISO=%lu\n",
			jiffies,countTX,countCS,countMOSI,countMISO);
	*eof=1;
	return len;
}

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

#define kontron_INIT	0xFF				
#define MISO		PARPORT_STATUS_BUSY		
#define nCS		0x04				
#define SCLK		0x02				
#define MOSI		0x01	 	 		

static struct mcp251x_platform_data mcp251x_info;       

/******************************************************************************
*			Module FUNCTIONS FOR SPI_BITBANG 
******************************************************************************/

/*______________________functions-helpers____________________________________*/

static inline struct spi_kontron  *spidev_to_pp(struct spi_device *spi)
{
	return spi->controller_data;
}



/* NOTE:  we don't actually need to reread the output values, since they'll
 * still be what we wrote before.  Plus, going through parport builds in
 * a ~1ms/operation delay; these SPI transfers could easily be faster.
 * 
 */

static inline void deassertCS(struct spi_kontron *pp)
{
	u8 data = parport_read_data(pp->port);
	parport_write_data(pp->port, data | nCS);
}

static inline void assertCS(struct spi_kontron *pp)
{
	u8 data = parport_read_data(pp->port);
	parport_write_data(pp->port, data & ~nCS);
}

static inline void clkHigh(struct spi_kontron *pp)
{
	u8 data = parport_read_data(pp->port);
	parport_write_data(pp->port, data | SCLK);
}

static inline void clkLow(struct spi_kontron *pp)
{
	u8 data = parport_read_data(pp->port);
	parport_write_data(pp->port, data & ~SCLK);
}

/*______________________define spi_bitbang inlines___________________________*/

static inline void spidelay(unsigned d)
{
	udelay(d);						
}

static inline void setsck(struct spi_device *s, int is_on)
{
	struct spi_kontron *pp = spidev_to_pp(s);

	if (is_on)
		clkHigh(pp);
	else
		clkLow(pp);
}

static inline void setmosi(struct spi_device *s, int is_on)
{
	struct spi_kontron *pp = spidev_to_pp(s);
	u8 data = parport_read_data(pp->port);
	/*deb*/countMOSI++;
	data=data & ~nCS;

	if (is_on)
	{
		parport_write_data(pp->port, data | MOSI);
	}
	else
	{	
		parport_write_data(pp->port, data & ~MOSI);
	}
}

/*			
 * This code depends on board wiring.  
 */
static inline int getmiso(struct spi_device *s)
{
	struct spi_kontron *pp = spidev_to_pp(s);
	/*deb*/countMISO++;
	return ((MISO == (parport_read_status(pp->port) & MISO)) ? 0 : 1 );
}
/*______________________providing bitbang routines____________________________*/

#include "spi_bitbang_txrx.h"

static void kontron_chipselect(struct spi_device *spi, int value) 
{
	struct spi_kontron *pp = spidev_to_pp(spi);
	/*deb*/countCS++;

	if (value)
		assertCS(pp);
	else
		deassertCS(pp);
}

/*
 * Our actual bitbanger routine.
 */
static u32 kontron_txrx(struct spi_device *spi,
		   	 unsigned nsecs, u32 word, u8 bits)
{
	/*deb*/countTX++;
	return bitbang_txrx_be_cpha0(spi, nsecs, 0, 0, word, bits);
}

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

	master->bus_num = -1;		/* dynamic alloc of a bus number */
	master->num_chipselect = 1;				

	/*
	 * SPI and bitbang hookup.
	 */
	pp->bitbang.master = spi_master_get(master);
	pp->bitbang.chipselect = kontron_chipselect;
	pp->bitbang.txrx_word[SPI_MODE_0] = kontron_txrx;	
	pp->bitbang.flags =  SPI_MODE_0;			

	/*
	 * Parport hookup
	 */
	pp->port = p;
	pd = parport_register_device(p, DRVNAME,NULL, NULL, NULL,0, pp); 
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
		parport_write_data(pp->port, 0x08);          /*REVISIT*/
		mdelay(10);
		parport_release(pp->pd);
		parport_unregister_device(pd);
		(void) spi_master_put(master);
		return;
	}

	/*
	 * The modalias name MUST match the device_driver name
	 * for the bus glue code to match and subsequently bind them.
	 * We are binding to the generic drivers/net/can/mcp251x.c device
	 * driver.
	 */
	mcp251x_info.oscillator_frequency=16000000; 

	strcpy(pp->info.modalias, "mcp2515");			
	pp->info.max_speed_hz =  10*1000*1000;		    
	pp->info.chip_select = 0;				
	pp->info.mode = SPI_MODE_0;  		

	pp->info.platform_data=&mcp251x_info; 


	/* power up the chip */
	parport_write_data(pp->port, kontron_INIT);

	/* Enable access to our primary data structure via
	 * the board info's (void *)controller_data.
	 */
	pp->info.controller_data = pp;
	pp->spidev_kontron = spi_new_device(pp->bitbang.master, &pp->info);
	if (pp->spidev_kontron)
	{
	 	dev_dbg(&pp->spidev_kontron->dev, "spidev_kontron at %s\n",
				dev_name(&pp->spidev_kontron->dev));
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
		parport_write_data(pp->port, 0x08);        /*REVISIT*/
		mdelay(10);
		parport_release(pp->pd);
		parport_unregister_device(pd);
		(void) spi_master_put(master);
		return;
		
	}
	pp->spidev_kontron->bits_per_word = 8;		

	pkontron = pp;
	return;
}

static void spi_kontron_detach(struct parport *p)
{
	struct spi_kontron		*pp;

	if (!pkontron || pkontron->port != p)
		return;

	pp = pkontron;
	spi_unregister_device(pp->spidev_kontron);
	spi_bitbang_stop(&pp->bitbang);

	/* power down */
	parport_write_data(pp->port, 0x08);                  /*REVISIT*/

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
	create_proc_read_entry(PROC_NAME,0,NULL,procfile_read,procbuffer);
	return parport_register_driver(&spi_kontron);
}


static void __exit cleanup_spi_kontron(void)
{
	remove_proc_entry(PROC_NAME,NULL);
	parport_unregister_driver(&spi_kontron);
}

module_init(init_spi_kontron);
module_exit(cleanup_spi_kontron);
