///#include <sys/types.h>
//#include <linux/types.h>
//#include <linux/socket.h>
//#include <linux/can/raw.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/can/error.h>
#include <linux/sockios.h>
#include <string.h>
#include <stdio.h>
int main(void)
{

	int sfd;
	int proc;
	int stat;
	struct sockaddr_can addr;
	struct ifreq ifr;
	char buffer[1024];
    // Frame to send (example)
    struct can_frame frame_wr = {
        .can_id=0x264,
        .can_dlc=2,
        .data = { 0x11, 0x22 },
    };
  	  ssize_t nbytes=0;


	sfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	strcpy(ifr.ifr_name,"can0");
	ioctl(sfd,SIOCGIFINDEX, &ifr);

    	addr.can_family = AF_CAN;
    	addr.can_ifindex = ifr.ifr_ifindex;

   	 // Bind the socket
   	 if(bind(sfd, (struct sockaddr *)&addr, sizeof(addr))<0)
	{
     	   perror("bind ERROR: ");
        	close(sfd);
        	return -1;
   	}
printf("Let's read /proc/spi\n");
proc=open("/proc/spi",O_RDWR);
stat=read(proc,buffer,sizeof(buffer));
write(1,buffer,stat);
close(proc);
printf("\n**************************************************\n");
printf("Try to write data\n");
nbytes = write(sfd, &frame_wr, sizeof(struct can_frame)); // Send a CAN frame 
printf("Successfully written %d bytes\n",nbytes);
printf("\n**************************************************\n");
printf("Let's read /proc/spi\n");
proc=open("/proc/spi",O_RDWR);
stat=read(proc,buffer,sizeof(buffer));
write(1,buffer,stat);
close(proc);
close(sfd);








/*777*/
	return 0;
}
