#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <sys/vfs.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mount.h>

//#define SCULL_DEBUG
#ifdef SCULL_DEBUG
#	undef PDEBUG
#	ifdef __KERNEL__
#		define PDEBUG(fmt, args...)	printk(KERN_DEBUG "scull: " fmt, ## args)
#	else
#		define PDEBUG(fmt, args...)	fprintf(stderr, fmt, ## args)
#	endif
#else
#	define PDEBUG(fmt, args...)
#endif

/* Umount options */
#define MNT_DETACH		0x00000002	/* Just detach from the tree */

#define SYSMGR_DEV		"/dev/sysmgr2"
#define SYSMGR_IOC_MAGIC	'S'
#define SYSMGR_IOC_GET_NAND_DEV	_IOR(SYSMGR_IOC_MAGIC, 10, unsigned int)
#define SYSMGR_IOC_GET_SD_DEV	_IOR(SYSMGR_IOC_MAGIC, 11, unsigned int)

#define DEVICE_SD		0x00000001
#define DEVICE_NAND		0x00000002
#define DEVICE_USB		0x00000004
#define EVENT_INSERT		0x00010000
#define EVENT_REMOVE		0x00020000

#define ADD_SD_MSG		"add@/devices/sd_bus/card"
#define REMOVE_SD_MSG		"remove@/devices/sd_bus/card"
#define ADD_NAND_MSG		"add@/devices/nand_bus/card"
#define REMOVE_NAND_MSG		"remove@/devices/nand_bus/card"
#define ADD_USB_MSG		"add@/devices/platform/w55fa93-ohci/usb"
#define REMOVE_USB_MSG		"remove@/devices/platform/w55fa93-ohci/usb"

#define MOUNT_POINT_SD		"/mnt/sdcard"
#define MOUNT_POINT_SD2		"/mnt/sdcard2"
#define MOUNT_POINT_NAND	"/mnt/nandcard"
#define MOUNT_POINT_NAND2	"/mnt/nandcard2"
#define MOUNT_POINT_USB		"/mnt/usbdisk"
#define MOUNT_POINT_USB2	"/mnt/usbdisk2"

#define MAX_PARTITION		9
#define MAX_SCSI_DISK		6
#define MAX_HOT_DISK		6

#define SOCKET_BUFFER_SIZE	16 * 1024
#define UEVENT_BUFFER_SIZE	2048
#define RESERVED_SIZE		128

const char*	Scsi_Disk[MAX_SCSI_DISK] = {"sda", "sdb", "sdc", "sdd", "sde", "sdf"};

int	Device_Type[MAX_HOT_DISK];
char	Device_Node[MAX_HOT_DISK][16];
char	Mount_Point[MAX_HOT_DISK][32];

char	Event_Buf[UEVENT_BUFFER_SIZE*2];
int	hot_sock;
int	fd_sysmgr;


int init_hot_sock(void)
{
	struct		sockaddr_nl snl;
	const int	buffersize = SOCKET_BUFFER_SIZE;
	int		retval;

	memset(&snl, 0x00, sizeof(struct sockaddr_nl));
	snl.nl_family = AF_NETLINK;
	snl.nl_pid = getpid();
	snl.nl_groups = 1;
 
	int hot_sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	if (hot_sock == -1) {
		printf("error getting socket: %s", strerror(errno));
		return -1;
	}

	/* set receive buffersize */
	setsockopt(hot_sock, SOL_SOCKET, SO_RCVBUFFORCE, &buffersize, sizeof(buffersize));
	retval = bind(hot_sock, (struct sockaddr *) &snl, sizeof(struct sockaddr_nl));

	if (retval < 0) {
		printf("bind failed: %s", strerror(errno));
		close(hot_sock);
		hot_sock = -1;
		return -1;
	}

	return hot_sock;
}

int get_hot_event(int* event_list)
{
	fd_set	fdset;
	struct	timeval time;
	char	recv_buf[UEVENT_BUFFER_SIZE];
	char	str[32];
	char*	ptr;
	int	host_num[MAX_HOT_DISK];
	int	buf_len, i;
	int	event_num = 0, add_event = 0, match_num = 0;

	for (i = 0; i < MAX_HOT_DISK; i++)
		host_num[i] = -1;
	memset(Event_Buf, 0x0, sizeof(Event_Buf));
	while ((!event_num) || (!(add_event == match_num))) {
		while (1) {
			FD_ZERO(&fdset);
			FD_SET(hot_sock, &fdset);
			time.tv_sec = 0;
			time.tv_usec = 10000;
			if (select(hot_sock + 1, &fdset, NULL, NULL, &time) > 0) {
				if (FD_ISSET(hot_sock, &fdset)) {
					memset(recv_buf, 0x0, sizeof(recv_buf));
					recv(hot_sock, recv_buf, sizeof(recv_buf), 0);
					buf_len = strlen(Event_Buf);
					if (buf_len > UEVENT_BUFFER_SIZE*2 - RESERVED_SIZE) {
						memcpy(&Event_Buf[0], &Event_Buf[buf_len - RESERVED_SIZE], RESERVED_SIZE);
						memset(&Event_Buf[RESERVED_SIZE], 0x0, UEVENT_BUFFER_SIZE*2 - RESERVED_SIZE);
					}
					strcat(Event_Buf, recv_buf);
					break;
				}
			}
		}
	
		PDEBUG("Event_Buf: %s\n", Event_Buf);
		if (!event_num) {
			if (strstr(Event_Buf, "add@/class/scsi_device")) {
				if (ptr = strstr(Event_Buf, ADD_SD_MSG)) {
					event_list[event_num] = EVENT_INSERT | DEVICE_SD;
					PDEBUG("ptr = %s\n", ptr);
					ptr = strstr(ptr, "scsi_host/host");
					if (ptr) {
						ptr += (sizeof("scsi_host/host") - 1);
						sscanf(ptr, "%[0-9]s", str);
						PDEBUG("str = %s\n", str);
						host_num[event_num] = atoi(str);
						event_num++;
						add_event++;
					}
				}
				if (ptr = strstr(Event_Buf, ADD_NAND_MSG)) {
					event_list[event_num] = EVENT_INSERT | DEVICE_NAND;
					PDEBUG("ptr = %s\n", ptr);
					ptr = strstr(ptr, "scsi_host/host");
					if (ptr) {
						ptr += (sizeof("scsi_host/host") - 1);
						sscanf(ptr, "%[0-9]s", str);
						PDEBUG("str = %s\n", str);
						host_num[event_num] = atoi(str);
						event_num++;
						add_event++;
					}
				}
				if (ptr = strstr(Event_Buf, ADD_USB_MSG)) {
					event_list[event_num] = EVENT_INSERT | DEVICE_USB;
					PDEBUG("ptr = %s\n", ptr);
					ptr = strstr(ptr, "scsi_host/host");
					if (ptr) {
						ptr += (sizeof("scsi_host/host") - 1);
						sscanf(ptr, "%[0-9]s", str);
						PDEBUG("str = %s\n", str);
						host_num[event_num] = atoi(str);
					}
					else
						host_num[event_num] = 0;
					event_num++;
					add_event++;
				}
			}
			if (strstr(Event_Buf, REMOVE_SD_MSG))
					event_list[event_num++] = EVENT_REMOVE | DEVICE_SD;
			if (strstr(Event_Buf, REMOVE_NAND_MSG))
					event_list[event_num++] = EVENT_REMOVE | DEVICE_NAND;
			if (strstr(Event_Buf, REMOVE_USB_MSG))
					event_list[event_num++] = EVENT_REMOVE | DEVICE_USB;
		}
		if (add_event) {
			for (i = 0; i < MAX_HOT_DISK; i++) {
				if (host_num[i] >= 0) {
					PDEBUG("host_num[%d] = %d\n", i, host_num[i]);
					sprintf(str, "add@/class/scsi_device/%d", host_num[i]);
					if (strstr(Event_Buf, str)) {
						PDEBUG("match!\n");
						match_num++;
						host_num[i] = -1;
					}
				}
			}
		}
		if ((event_num) && (add_event == match_num)) {
			for (i = 0; i < event_num; i++)
				PDEBUG("event_list[%d] = %p\n", i, event_list[i]);
			PDEBUG("get_hot_event done!, add_event=%d, match_num=%d\n", add_event, match_num);
		}
	}

	return event_num;
}

int get_device_type(char* device_node)
{	   
	char	sd_char, nand_char;
	int	device_type;

	if (ioctl(fd_sysmgr, SYSMGR_IOC_GET_SD_DEV, &sd_char) < 0)
		fprintf(stderr, "SYSMGR_IOC_GET_SD_DEV ioctl failed: %s\n", strerror(errno));
	if (ioctl(fd_sysmgr, SYSMGR_IOC_GET_NAND_DEV, &nand_char) < 0)
		fprintf(stderr, "SYSMGR_IOC_GET_NAND_DEV ioctl failed: %s\n", strerror(errno));

	if (device_node[2] == sd_char)
		device_type = DEVICE_SD;
	else if (device_node[2] == nand_char)
		device_type = DEVICE_NAND;
	else
		device_type = DEVICE_USB;

	return device_type;
}   

int get_device_node(int event_list, int part_num, char* device_node)
{	   
	char	dev_node[16];
	char	dev_str[32];
	char	node_char, sd_char, nand_char;
	int	i, retval = 0;

	node_char = '-';
	if (ioctl(fd_sysmgr, SYSMGR_IOC_GET_SD_DEV, &sd_char) < 0)
		fprintf(stderr, "SYSMGR_IOC_GET_SD_DEV ioctl failed: %s\n", strerror(errno));
	if (ioctl(fd_sysmgr, SYSMGR_IOC_GET_NAND_DEV, &nand_char) < 0)
		fprintf(stderr, "SYSMGR_IOC_GET_NAND_DEV ioctl failed: %s\n", strerror(errno));
	PDEBUG("sd_char=%c, nand_char=%c\n", sd_char, nand_char);

	if (event_list & DEVICE_SD)
		node_char = sd_char;
	else if (event_list & DEVICE_NAND)
		node_char = nand_char;
	else if (event_list & DEVICE_USB) {
		memset(dev_node, 0x0, sizeof(dev_node));
		memset(dev_str, 0x0, sizeof(dev_str));
		for (i = 0; i < MAX_SCSI_DISK; i++) {
			if ((Scsi_Disk[i][2] == sd_char) || (Scsi_Disk[i][2] == nand_char))
				continue;
			if (part_num <= MAX_PARTITION)
				sprintf(dev_node, "/dev/%s%d", Scsi_Disk[i], part_num);
			else
				sprintf(dev_node, "/dev/%s", Scsi_Disk[i]);
			if (check_mounted_disk(dev_node) > 0) {
				PDEBUG("device node %s had been mounted yet!\n", dev_node);
				continue;
			}
			if (part_num <= MAX_PARTITION)
				sprintf(dev_str, "%s@/block/%s/%s%d", (event_list & EVENT_INSERT) ? "add" : "remove", 
					Scsi_Disk[i], Scsi_Disk[i], part_num);
			else
				sprintf(dev_str, "%s@/block/%s", (event_list & EVENT_INSERT) ? "add" : "remove", Scsi_Disk[i]);
			if (strstr(Event_Buf, dev_str)) {
				node_char = Scsi_Disk[i][2];
				if (part_num <= MAX_PARTITION)
					sprintf(device_node, "/dev/sd%c%d", node_char, part_num);
				else
					sprintf(device_node, "/dev/sd%c", node_char);
				PDEBUG("device_node in event is %s\n", device_node);
				return 1;
			}
		}
		PDEBUG("dev_str: %s\n", dev_str);
	}

	if ((node_char < 'a') || (node_char >= ('a' + MAX_SCSI_DISK))) {
		PDEBUG("can not find node name for device type %d!\n", event_list & 0xF);
		retval = -1;
	} else {
		memset(dev_str, 0x0, sizeof(dev_str));
		if (event_list & EVENT_INSERT)
			if (part_num <= MAX_PARTITION)
				sprintf(dev_str, "add@/block/sd%c/sd%c%d", node_char, node_char, part_num);
			else
				sprintf(dev_str, "add@/block/sd%c", node_char);
		else if (event_list & EVENT_REMOVE)
			if (part_num <= MAX_PARTITION)
				sprintf(dev_str, "remove@/block/sd%c/sd%c%d", node_char, node_char, part_num);
			else
				sprintf(dev_str, "remove@/block/sd%c", node_char);
		else
			printf("unknown event type!\n");
		PDEBUG("dev_str: %s\n", dev_str);
		if (strstr(Event_Buf, dev_str)) {
			if (part_num <= MAX_PARTITION)
				sprintf(device_node, "/dev/sd%c%d", node_char, part_num);
			else
				sprintf(device_node, "/dev/sd%c", node_char);
			PDEBUG("device_node in event is %s\n", device_node);
			retval = 1;
		}
	}

	return retval;
}   

int check_mounted_disk(char *string)
{
	FILE	*fp;
	char	*line = NULL;
	size_t	len;
	int	retval = 0;

	if (!(fp = fopen("/proc/mounts", "rb"))) {
		perror("open /proc/mounts failed!\n");
		return -1;
	}

	while (getline(&line, &len, fp) != -1) {
		if (strstr(line, string)) {
			retval = 1;
			break;
		}
	}

	fclose(fp);
	if (line)
		free(line);

	return retval;
}

int get_device_type_num(int device_type)
{	   
	int	disk_num = 0;
	int	i;

	for (i = 0; i < MAX_HOT_DISK; i++)
		if (Device_Type[i] == device_type)
			disk_num++;

	return disk_num;
}   

int get_mount_point(int device_type, char* mount_point)
{
	int	disk_num;

	disk_num = get_device_type_num(device_type);
	if (disk_num > 1) {
		printf("too many device type %d had been mounted!\n", device_type);
		return -1;
	}

	switch (device_type) {
	case DEVICE_SD:
		if (disk_num == 0)
			strcpy(mount_point, MOUNT_POINT_SD);
		else
			strcpy(mount_point, MOUNT_POINT_SD2);
		break;
	case DEVICE_NAND:
		if (disk_num == 0)
			strcpy(mount_point, MOUNT_POINT_NAND);
		else
			strcpy(mount_point, MOUNT_POINT_NAND2);
		break;
	case DEVICE_USB:
		if (disk_num == 0)
			strcpy(mount_point, MOUNT_POINT_USB);
		else
			strcpy(mount_point, MOUNT_POINT_USB2);
		break;
	default:
		printf("unknown device type %d!\n", device_type);
		return -1;
	}

	return 0;
}

int do_mount(int device_type, char* device_node)
{
	char	mount_point[32];
	char	fs_type[] = "vfat";
	char	mount_options[] = "umask=0000,shortname=mixed,utf8";
	unsigned long	mount_flags;
	int	i, retval = 0;

	// wait for mounting embedded storage
	if (device_type != DEVICE_USB)
		usleep(100000);
	if (check_mounted_disk(device_node) > 0) {
		printf("device node %s had been mounted yet!\n", device_node);
		return -1;
	}
	memset(mount_point, 0x0, sizeof(mount_point));
	if (get_mount_point(device_type, mount_point) < 0) {
		printf("find mount point for device type %d node %s failed!\n", device_type, device_node);
		return -1;
	}

	mkdir(mount_point, 0777);

	// fix read SD super block error in mounting
	//usleep(1000);
	mount_flags = MS_NOATIME | MS_NODIRATIME;
	if ((retval = mount(device_node, mount_point, fs_type, mount_flags, mount_options)) < 0) {
		// probably read-only file system, try to mount again
		mount_flags = MS_RDONLY;
		retval = mount(device_node, mount_point, fs_type, mount_flags, mount_options);
	}
	if (retval == 0) {
		if (mount_flags == (MS_NOATIME | MS_NODIRATIME))
			printf("[Mount VFAT]: %s --> %s\n", device_node, mount_point);
		if (mount_flags == MS_RDONLY)
			printf("[Mount Read-Only VFAT]: %s --> %s\n", device_node, mount_point);
		fflush(stdout);
		for (i = 0; i < MAX_HOT_DISK; i++) {
			if (Device_Type[i] == 0) {
				Device_Type[i] = device_type;
				strcpy(Device_Node[i], device_node);
				strcpy(Mount_Point[i], mount_point);
				break;
			}
		}
		if (i >= MAX_HOT_DISK) {
			printf("reach max hot disks! this will cause umount failed!\n");
			retval = -1;
		}
	} else {
		rmdir(mount_point);
		printf("failed to mount %s --> %s, error code: %s\n", device_node, mount_point, strerror(errno));
	}

	return retval;
}

void do_umount(int device_type)
{
	int	i;

	for (i = 0; i < MAX_HOT_DISK; i++) {
		if (Device_Type[i] == device_type) {
			if (umount2(Mount_Point[i], MNT_DETACH) < 0) {
				printf("umount %s failed!\n", Mount_Point[i]);
			} else { 
				printf("[Umount VFAT]: %s -X-> %s\n", Device_Node[i], Mount_Point[i]);
				fflush(stdout);
				rmdir(Mount_Point[i]);
				Device_Type[i] = 0;
				memset(Device_Node[i], 0, sizeof(Device_Node[i]));
				memset(Mount_Point[i], 0, sizeof(Mount_Point[i]));
			}
		}
	}

	return;
}

int check_existed_partition(void)
{
	char	pbuf[1024];
	char	mbuf[1024];
	char	device_node[16];
	int	device_type;
	int	fd, len, hit, i, j;

	fd = open("/proc/partitions", O_RDONLY);
	if (fd <= 0) {
		perror("open /proc/partitions failed!\n");
		return -1;
	}
	memset(pbuf, 0x0, sizeof(pbuf));
	len = read(fd, pbuf, sizeof(pbuf));
	close(fd);
	// no any partition found
	if (len <= 0) {
		printf("/proc probably not mounted\n");
		return 0;
	}

	fd = open("/proc/mounts", O_RDONLY);
	if (fd <= 0) {
		perror("open /proc/mounts failed!\n");
		return -1;
	}
	memset(mbuf, 0x0, sizeof(mbuf));
	len = read(fd, mbuf, sizeof(mbuf));
	close(fd);
	// no any mount found
	if (len <= 0) {
		printf("/proc probably not mounted\n");
		return 0;
	}

	for (i = 0; i < MAX_SCSI_DISK; i++) {
		hit = 0;
		memset(device_node, 0x0, sizeof(device_node));
		for (j = 1; j <= MAX_PARTITION; j++) {
			if (j == 2)
				continue;
			sprintf(device_node, "%s%d", Scsi_Disk[i], j);
			if (strstr(pbuf, device_node)) {
				hit++;
				if (!strstr(mbuf, device_node)) {
					device_type = get_device_type(device_node);
					sprintf(device_node, "/dev/%s%d", Scsi_Disk[i], j);
					do_mount(device_type, device_node);
				}
			}
			j++;
		}

		// maybe there is no MBR information
		if (!hit) {
			sprintf(device_node,"%s",Scsi_Disk[i]);
			if (strstr(pbuf, device_node) && (!strstr(mbuf, device_node))) {
				device_type = get_device_type(device_node);
				sprintf(device_node,"/dev/%s",Scsi_Disk[i]);
				do_mount(device_type, device_node);
			}
		}
	}
}

int main(int argc, char* argv[])
{
	int	event_list[MAX_HOT_DISK];
	char	device_node[16];
	int	event_num, hit;
	int	i, j;

	hot_sock = init_hot_sock();
	fd_sysmgr = open(SYSMGR_DEV, O_RDWR);
	if (fd_sysmgr < 0) {
		printf("open device %s failed, error: %s\n", SYSMGR_DEV, strerror(errno));
		return -1;
	}

	check_existed_partition();

	while (1) {
		memset(event_list, 0x0, sizeof(event_list));
		event_num = get_hot_event(event_list);

		for (i = 0; i < event_num; i++) {
			if (event_list[i] & EVENT_INSERT) {
				hit = 0;
				memset(device_node, 0x0, sizeof(device_node));
				for (j = 1; j <= MAX_PARTITION; j++) {
					if (j == 2)
						continue;
					if (get_device_node(event_list[i], j, device_node) > 0) {
						hit++;
						do_mount(event_list[i] & 0xF, device_node);
					}
				}

				// maybe there is no MBR information
				if (!hit) {
					if (get_device_node(event_list[i], j, device_node) > 0)
						do_mount(event_list[i] & 0xF, device_node);
				}
			}
			else if (event_list[i] & EVENT_REMOVE) {
				do_umount(event_list[i] & 0xF);
			}
		}
	}

	return 0;
}

