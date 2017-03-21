#include "wan_type.h"

#ifndef INVALID_SOCKET
#define INVALID_SOCKET	(-1)
#endif

#define DHCP_DISCOVER	0x01
#define DHCP_OFFER		0x02

#define DHCP_SERVER_PORT   67
#define DHCP_CLIENT_PORT   68

#define DHCP_OPTION_MESSAGE_TYPE						0x35
#define DHCP_OPTION_CLIENT_IDENTIFIER					0x3d
#define DHCP_OPTION_VENDOR_CLASS_IDENTIFIER				0x3c
#define DHCP_OPTION_PARAMETER_REQUEST_LIST				0x37
#define DHCP_OPTION_SUBNET_MASK							0x01
#define DHCP_OPTION_ROUTER								0x03
#define DHCP_OPTION_DOMAIN_NAME_SERVER					0x06
#define DHCP_OPTION_HOST_NAME							0x0c
#define DHCP_OPTION_DOMAIN_NAME							0x0f
#define DHCP_OPTION_BROADCAST_ADDRESS					0x1c
#define DHCP_OPTION_NETBIOS_OVER_TCP_IP_NAME_SERVER		0x2c
#define DHCP_OPTION_END									0xff
#define DHCP_OPTION_PADDING								0x00

#define	DHCP_XID_POS	4
#define IP_CHECKSUM_POS	24

#define IP_HLEN		sizeof(struct iphdr)	/* 20 (no options)*/
#define UDP_HLEN	sizeof(struct udphdr)	/* 8 */

#define MAX_DHCP_CHADDR_LENGTH           16
#define MAX_DHCP_SNAME_LENGTH            64
#define MAX_DHCP_FILE_LENGTH             128
#define MIN_DHCP_OPTIONS_LENGTH          312
#define EVENT_START          1
#define EVENT_END            0

typedef struct dhcp_packet
{
        UINT8	op;		/* packet type */
        UINT8	htype;		/* type of hardware address for this machine (Ethernet, etc) */
        UINT8	hlen;		/* length of hardware address (of this machine) */
        UINT8	hops;		/* hops */
        UINT32	xid;		/* random transaction id number - chosen by this machine */
        UINT16	secs;		/* seconds used in timing */
        UINT16	flags;		/* flags */
        UINT32	ciaddr;		/* IP address of this machine (if we already have one) */
        UINT32	yiaddr;		/* IP address of this machine (offered by the DHCP server) */
        UINT32	siaddr;		/* IP address of DHCP server */
        UINT32	giaddr;		/* IP address of DHCP relay */
        UINT8	chaddr[MAX_DHCP_CHADDR_LENGTH];	 	/* hardware address of this machine */
        UINT8	sname[MAX_DHCP_SNAME_LENGTH];		/* name of DHCP server */
        UINT8	file[MAX_DHCP_FILE_LENGTH];		/* boot file name (used for disk less booting) */
        UINT8	options[MIN_DHCP_OPTIONS_LENGTH];
} DHCP_PACKET;


typedef struct wan_type_detection
{
	WAN_TYPE type;	/* detected wan type */
	int timeout;
	int retries;
	BOOL required;

	BOOL have_rsp_PPPoE;
	BOOL have_rsp_DHCP;
} WAN_TYPE_DETECTION;

WAN_TYPE_DETECTION g_wan_type_detect;

int g_trans_Id = 0;

/*内部函数声明*/
static int get_if_mac_addr(
				char ifname[IFNAMSIZ + 1],
				UINT8 mac[ETH_ALEN]);
static int parse_input_para(
				char *argv[],
				char ifname[IFNAMSIZ + 1],
				int *p_try_times,
				int *p_timeout,
				int *p_bitmap);
static void wan_type_detection_process(
						char* ifname,
						UINT8 mac[ETH_ALEN],
						int try_times,
						int timeout,
						int detect_bitmap);
static BOOL wan_type_is_PPPoE(char* ifname, UINT8 mac[ETH_ALEN], int timeout);
static BOOL wan_type_is_DHCP(char* ifname, UINT8 mac[ETH_ALEN], int timeout);
static BOOL wan_has_reply(int sock, UINT8* mac, WAN_TYPE type, int timeout);
static BOOL packet_is_offer(char* packet, UINT8* mac, WAN_TYPE type);
static UINT16 checksum(void* addr, int count);


/************************************************************
  Function:		main()
  Description:	wan type auto detection
  Input: 		para1 - interface name, "eth0" etc.
				para2 - mac of interface, "11:22:33:44:55:66" etc.
				para3 - try times.
				para4 - seconds to timeout.
  Output:		N/A
  Return:		wanType or error code
************************************************************/

int main(int argc, char *argv[])
{
	char ifname[IFNAMSIZ + 1];
	UINT8 mac[ETH_ALEN];
	int try_times = 0;
	int timeout = 0;
	int detect_bitmap = 0;
	int ret = -1;

	if (argc != INPUT_PARA_NUM)
	{
		DBG_PRINT("para1 - interface name, \"eth0\" etc.\n");
		DBG_PRINT("para2 - try times.\n");
		DBG_PRINT("para3 - seconds to timeout.\n");
		DBG_PRINT("para4 - bitmap for detect type: 0x1-dhcp; 0x2-PPPoE; 0x3-both.\n");

		return INPUT_PARA_ERROR;
	}

	ret = parse_input_para(argv, ifname, &try_times, &timeout, &detect_bitmap);
	RET_VAR_IF((ret != 0), ret);

	ret = get_if_mac_addr(ifname, mac);

	wan_type_detection_process(ifname, mac, try_times, timeout, detect_bitmap);

	DBG_PRINT("wan type is: %d\n", g_wan_type_detect.type);

	return g_wan_type_detect.type;
}

/************************************************************
  Function:		get_if_mac_addr()
  Description:	get mac addr of interface
  Input: 		ifname
  Output:		mac addr of if
  Return:		N/A
************************************************************/
static int get_if_mac_addr(
				char ifname[IFNAMSIZ + 1],
				UINT8 mac[ETH_ALEN])
{
	int sock = -1;
	struct ifreq ifr;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		DBG_PRINT("Could not open DGRAM socket\n");
		return INTERNAL_ERROR;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0)
	{
		DBG_PRINT("Could not get mac addr of wan\n");
		close(sock);
		return INTERNAL_ERROR;
	}

	memcpy(mac, &ifr.ifr_hwaddr.sa_data, ETH_ALEN);
	DBG_PRINT("Wan mac Addr: %2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X\n",
			  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	close(sock);

	return 0;
}

/************************************************************
  Function:		parse_input_para()
  Description:	parse input paras from cmd line
  Input: 		input para from cmd line
  Output:		ifname, mac, try_times, timeout
  Return:		N/A
************************************************************/
static int parse_input_para(
				char *argv[],
				char ifname[IFNAMSIZ + 1],
				int *p_try_times,
				int *p_timeout,
				int *p_bitmap)
{
	char *tmp;
	int base = 10;

	/*get ifname*/
	strncpy(ifname, argv[1], IFNAMSIZ);
	ifname[IFNAMSIZ] = 0;

	/*get try times*/
	*p_try_times = atoi(argv[2]);

	/*get timeout*/
	*p_timeout = atoi(argv[3]);

	/*get bitmap of detect type*/
	if ((!strncmp(argv[4], "0x", 2)) || (!strncmp(argv[4], "0X", 2)))
	{
		base = 16;
	}
	*p_bitmap = strtol(argv[4], &tmp, base);

	/*check*/
	if (strlen(ifname) <= 0)
	{
		DBG_PRINT("error:para1 should be ifname\n");
		return INPUT_PARA_ERROR;
	}

	if ((*p_try_times <= 0) || (*p_try_times > MAX_RETRY_TIMES))
	{
		return INPUT_PARA_ERROR;
	}

	if ((*p_timeout <= 0) || (*p_timeout > MAX_WAIT_SECONDS))
	{
		return INPUT_PARA_ERROR;
	}

	if (((*p_bitmap) & 0xFFFFFFFC) || ((*p_bitmap) == 0))
	{
		DBG_PRINT("error:detect type error: %d\n", (*p_bitmap));
		return INPUT_PARA_ERROR;
	}

	return 0;
}

/************************************************************
  Function:		wan_type_detection_process()
  Description:	Detect WAN port available connection type
  Input: 		index - The WAN port index
  Output:		N/A
  Return:		N/A
************************************************************/
static void wan_type_detection_process(
						char* ifname,
						UINT8 mac[ETH_ALEN],
						int try_times,
						int timeout,
						int detect_bitmap)
{
	int ret = 0;
	int i = 0;
	int wanType = 0;

	if (detect_bitmap & DETECT_TYPE_BIT_PPPOE)
	{
		/* PPPoE discovery */
		for	(i = 0; i < try_times; i++)
		{
			ret = wan_type_is_PPPoE(ifname, mac, timeout);
			if (ret == TRUE)
			{
				g_wan_type_detect.have_rsp_PPPoE = TRUE;
				g_wan_type_detect.type = WAN_TYPE_PPPOE;

				return;
			}
			else
			{
				g_wan_type_detect.have_rsp_PPPoE = FALSE;
			}
		}
	}

	if (detect_bitmap & DETECT_TYPE_BIT_DHCP)
	{
		/* DHCP discovery */
		for	(i = 0; i < try_times; i++)
		{
			ret = wan_type_is_DHCP(ifname, mac, timeout);
			if (ret == TRUE)
			{
				g_wan_type_detect.have_rsp_DHCP = TRUE;
				g_wan_type_detect.type = WAN_TYPE_DHCP;
				return;
			}
			else
			{
				g_wan_type_detect.have_rsp_DHCP = FALSE;
			}

		}
	}

	/* Default */
	g_wan_type_detect.type = WAN_TYPE_STATIC_IP;

	return;
}

/************************************************************
  Function:		wan_type_is_PPPoE()
  Description:	Detect whether PPPoE server is available.
  Input: 		ifname - The interface name of the WAN port
  				timeout - Timeout of EACH protocal detection loop.
  Output:		N/A
  Return:		TRUE - PPPoE is available
  				FALSE - PPPoE is unavailable
************************************************************/
static BOOL wan_type_is_PPPoE(char* ifname, UINT8 mac[ETH_ALEN], int timeout)
{
	int sock = 0;
	BOOL ret = FALSE;
	ssize_t sent = 0;
	struct sockaddr sa;		/* for interface name */

	UINT8 padi[] = {
		/* Ethernet header */
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	/* dest mac */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* src mac (8) */
		0x88, 0x63,	/* ether type = PPPoE Discovery */

		/* PADI packet */
		0x11, /* ver & type */
		0x09, /* code (CODE_PADI) */
		0x00, 0x00, /* session ID */
		0x00, 0x0c, /* Payload Length */

		/* PPPoE Tags */
		0x01, 0x03, /* TAG_TYPE =  Host-Uniq */
		0x00, 0x04, /* TAG_LENGTH = 0x0004 */
		0x12, 0x34, 0x56, 0x78,	/* TAG_VALUE */
		0x01, 0x01, /* TAG_TYPE = Service-Name */
		/* When the TAG_LENGTH is zero, this TAG is used to indicate that any service is acceptable. */
		0x00, 0x00, /* TAG_LENGTH = 0x0000 */
		};

	memset(&sa, 0, sizeof(sa));
	strcpy(sa.sa_data, ifname);

	memcpy(&padi[0 + ETH_ALEN], mac, ETH_ALEN);

	if ((sock = socket (PF_PACKET, SOCK_PACKET, htons(ETH_P_PPP_DISC))) < 0)
	{
		DBG_PRINT("Could not open raw socket\n");
		return FALSE;
    }

	DBG_PRINT("Send PADI\n");
	sent = sendto(sock, &padi, sizeof(padi), 0, &sa, sizeof(sa));
	ret = wan_has_reply(sock, mac, WAN_TYPE_PPPOE, timeout);
	close(sock);

	return ret;
}

#define DHCP_DETECT_UDP (0)
/************************************************************
  Function:		wan_type_is_DHCP()
  Description:	Detect whether DHCP server is available.
  Input: 		ifname - The interface name of the WAN port
  				timeout - Timeout of EACH protocal detection loop.
  Output:		N/A
  Return:		TRUE - DHCP is available
  				FALSE - DHCP is unavailable
************************************************************/
static BOOL wan_type_is_DHCP(char* ifname, UINT8 mac[ETH_ALEN], int timeout)
{
#if DHCP_DETECT_UDP
	int sock = 0;
#endif
	int rawsock = 0;

	ssize_t sent = 0;
	BOOL ret = FALSE;
	int flag = 1;

	struct sockaddr_in clisa;		/* for interface name */
	struct sockaddr_in bcsa;
	struct sockaddr sa;				/* for interface name */

	const int DHCP_PACKET_LEN = 590;// 548;
	UINT8 dhcp[DHCP_PACKET_LEN];


	unsigned char header[] = { /* size == 42 */
		/* ether header (14)*/
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x08, 0x00,

		/* IP header (20)*/
		0x45,	/* version & ihl */
		0x00,	/* tos */
		0x02, 0x40, /* tos_len */
		0x00, 0x00, /* id */
		0x00, 0x00,
		0x40,	/* TTL = 64 */
		0x11,	/* protocol */
		0x00, 0x00, /* IP header check sum (*) */
		0x00, 0x00, 0x00, 0x00, /* src IP */
		0xff, 0xff, 0xff, 0xff, /* dest IP */

		/* UDP header (8)*/
		0x00, 0x44, /* src port */
		0x00, 0x43, /* dest port */
		0x02, 0x2c, /* length */
		0x00, 0x00, /* check sum (*) */
        };

	unsigned char data[] = { /* size == 34 */
		/* dhcp packet */
		0x01,	/* Message type -- BOOTREQUEST */
		0x01,	/* Hardware type -- ethernet */
		0x06,	/* Hardware address length */
		0x00,	/* Hops */
		0x00, 0x00,	0x00, 0x00, /* Transaction ID (*) */
		0x00, 0x00,	/* Time elapsed */
		0x80, 0x00, /* Flags = Broadcast */
		0x00, 0x00, 0x00, 0x00,	/* Client IP address */
		0x00, 0x00, 0x00, 0x00, /* Your IP address */
		0x00, 0x00, 0x00, 0x00, /* Next server IP address */
		0x00, 0x00, 0x00, 0x00, /* Relay agent IP address */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Client hardware address (*) */
		/* 0x00, 0x12, 0x17, 0x37, 0x66, 0x29, Client hardware address */
		/* 0x00, 0x00, 0x00, 0x00 Server host name */
		};

	unsigned char option[] = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			/* first four bytes of options field is magic cookie (as per RFC 2132) */
			0x63, 0x82, 0x53, 0x63,

			DHCP_OPTION_MESSAGE_TYPE,
			0x01, 	/* Length */
			DHCP_DISCOVER, 	/* Value */

			DHCP_OPTION_CLIENT_IDENTIFIER,
			0x07, 	/* Length */
			0x01, 	/* Hardware type: Ethernet */
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	/* Client MAC address */

			DHCP_OPTION_VENDOR_CLASS_IDENTIFIER,
			0x0b, 	/* Length = 11 */
			84, 80, 45, 76, 73, 78, 75, 32, 86, 83, 89, /* tp-link  vsy */

			DHCP_OPTION_PARAMETER_REQUEST_LIST,
			0x07, 	/* Length */
			DHCP_OPTION_SUBNET_MASK,
			DHCP_OPTION_ROUTER,
			DHCP_OPTION_DOMAIN_NAME_SERVER,
			DHCP_OPTION_HOST_NAME,
			DHCP_OPTION_DOMAIN_NAME,
			DHCP_OPTION_BROADCAST_ADDRESS,
			DHCP_OPTION_NETBIOS_OVER_TCP_IP_NAME_SERVER,
			DHCP_OPTION_END,
			DHCP_OPTION_PADDING
			};

	UINT32 sum = 0;

	memset(&clisa, 0, sizeof(clisa));
	clisa.sin_family = AF_INET;
	clisa.sin_port = htons(DHCP_CLIENT_PORT);
	clisa.sin_addr.s_addr = INADDR_ANY;
	bzero(&clisa.sin_zero,sizeof(clisa.sin_zero));

	/* send the DHCPDISCOVER packet to broadcast address */
	bcsa.sin_family = AF_INET;
	bcsa.sin_port = htons(DHCP_SERVER_PORT);
	bcsa.sin_addr.s_addr = INADDR_BROADCAST;
	bzero(&bcsa.sin_zero, sizeof(bcsa.sin_zero));

	memset(&dhcp, 0, sizeof(dhcp));
	memcpy(&dhcp[0], data, sizeof(data));
	memcpy(&dhcp[230], option, sizeof(option));

	memcpy(&dhcp[28], mac, ETH_ALEN);	// Client hardware address
	memcpy(&dhcp[246], mac, ETH_ALEN);	// Client hardware address

	/* Create xid */
	srand(time(NULL));
	g_trans_Id = random();

	dhcp[DHCP_XID_POS]     = g_trans_Id >> 24;
	dhcp[DHCP_XID_POS + 1] = g_trans_Id >> 16;
	dhcp[DHCP_XID_POS + 2] = g_trans_Id >> 8;
	dhcp[DHCP_XID_POS + 3] = g_trans_Id;

#if DHCP_DETECT_UDP
	if ((sock = socket(PF_INET , SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		DBG_PRINT("Socket creation faild\n");
		return FALSE;
    }
#endif

	if ((rawsock = socket(PF_PACKET, SOCK_PACKET, htons(ETH_P_IP))) < 0)
	{
		DBG_PRINT("raw socket call failed\n");
#if DHCP_DETECT_UDP
		close(sock);
#endif
		return FALSE;
	}

#if DHCP_DETECT_UDP
	/* set the reuse address flag so we don't get errors when restarting */
	flag = 1;

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag)) < 0)
	{
		DBG_PRINT("Error: Could not set reuse address option on DHCP socket!\n");
		goto ERROR_OUT;
	}

	/* set the broadcast option - we need this to listen to DHCP broadcast messages */

	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&flag, sizeof(flag)) < 0)
	{
		DBG_PRINT("Error: Could not set broadcast option on DHCP socket!\n");
		goto ERROR_OUT;
    }

	 /* bind the socket */
	if (bind(sock,(struct sockaddr *)&clisa, sizeof(struct sockaddr)) < 0)
	{
		DBG_PRINT("Error: Could not bind to DHCP socket.\n");
		goto ERROR_OUT;
	}

	/* Send the UDP Socket Packet */
	sent = sendto(sock, &dhcp, 548, 0, (struct sockaddr*)&bcsa, sizeof(struct sockaddr));
	DBG_PRINT("Send DHCP Discover (udp) -- %d bytes\n",  sent);
#endif

	sum = 0;
	memmove((char*)dhcp + sizeof(header), (char*)dhcp, 548);
	memcpy((char*)dhcp, (char*)header, sizeof(header));
	memcpy(&dhcp[6], mac, ETH_ALEN);	// Source

	/* Calculate IP checksum */
	sum = checksum(&dhcp[ETH_HLEN], IP_HLEN);
	sum = htons(sum);
	dhcp[IP_CHECKSUM_POS + 1] = sum;
	dhcp[IP_CHECKSUM_POS] = sum >> 8;

	memset(&sa, 0, sizeof(sa));
	strcpy(sa.sa_data, ifname);

	/* Send the RAW Socket Packet */
	sent = sendto(rawsock, &dhcp, sizeof(dhcp), 0, &sa, sizeof(sa));
	DBG_PRINT("Send DHCP Discover (raw) -- %d bytes\n",  sent);

	/* Receive the packets */
	ret = wan_has_reply(rawsock, mac, WAN_TYPE_DHCP, timeout);

#if DHCP_DETECT_UDP
	close(sock);
#endif
	close(rawsock);

	return ret;

ERROR_OUT:
#if DHCP_DETECT_UDP
		close(sock);
#endif
	close(rawsock);
	return FALSE;
}

/************************************************************
  Function:		wan_has_reply()
  Description:	Test if wan port has offer reply or not.
  Input: 		sock - The socket
  				mac - The MAC of the WAN port
  				type - The WAN type
  				timeout - Time out of each loop.
  Output:		N/A
  Return:		TRUE - WAN port has relpy.
  				FALSE - WAN port does not has reply.
************************************************************/
static BOOL wan_has_reply(int sock, UINT8* mac, WAN_TYPE type, int timeout)
{
	int received = -1;
	struct timeval tv;
	time_t prev;
	fd_set fdset;
	static char buf[ETH_FRAME_LEN];

	tv.tv_usec = 0;
	time(&prev);
	while (timeout > 0)
	{
		FD_ZERO(&fdset);
		FD_SET(sock, &fdset);

		tv.tv_sec = timeout;

		if (select(sock + 1, &fdset, (fd_set*)NULL, (fd_set*)NULL, &tv) < 0)
		{
			DBG_PRINT("Select error.\n");
		}
		else
		{
			if (FD_ISSET(sock, &fdset))
			{
				memset(&buf, 0, sizeof(buf));
				received = recv(sock, buf, sizeof(buf), 0);

				if (packet_is_offer(buf, mac, type) == TRUE)
				{
					return TRUE;
				}
			}
		}

		timeout -= time(NULL) - prev;
		time(&prev);
	}

	return FALSE;
}

/************************************************************
  Function:		packet_is_offer()
  Description:	Test if recieved packet is OFFER or not.
  Input: 		packet - The recieved packet
  				mac - The MAC of the WAN port
  				type - The WAN type
  Output:		N/A
  Return:		TRUE - The relpy is offer packet.
  				FALSE - The relpy is not offer packet.
************************************************************/
static BOOL packet_is_offer(char* packet, UINT8* mac, WAN_TYPE type)
{
	struct ether_header* eh = (struct ether_header *) &packet[0];
	struct iphdr* ih = (struct iphdr*) &packet[ETH_HLEN];
	DHCP_PACKET* dhcp = (DHCP_PACKET*) &packet[ETH_HLEN + IP_HLEN + UDP_HLEN];
	BOOL ret = FALSE;

	switch(type)
	{
	case WAN_TYPE_PPPOE:
		/* For PADO of PPPoE, the destination MAC is Client'MAC. */
		if (!strncmp((char*)mac, (char*)eh->ether_dhost, ETH_ALEN) &&
		   eh->ether_type == htons(ETH_P_PPP_DISC))	/*8863*/
		{
			DBG_PRINT("Receive PADO.\n");
			ret = TRUE;
		}
		break;

	case WAN_TYPE_DHCP:
		/* For Offer of DHCP, the destination MAC is FF:FF:FF:FF:FF:FF, the IP is 255.255.255.255. */
		if (eh->ether_type == htons(ETH_P_IP) &&
			ih->protocol == IPPROTO_UDP &&
			ntohl(dhcp->xid) == g_trans_Id &&
			dhcp->op == DHCP_OFFER)	// It is better to add chaddr compare
		{
			DBG_PRINT("receive DHCP Offer.\n");
			ret = TRUE;
		}
		break;

	default:
		break;
	}

	return ret;
}

/************************************************************
  Function:		checksum()
  Description:	Calculate the checksum.
  Input: 		Compute Internet Checksum for "count" bytes
  				beginning at location "addr".
  Output:		N/A
  Return:		The checksum
************************************************************/
static UINT16 checksum(void* addr, int count)
{
	/*
	 * Compute Internet Checksum for "count" bytes
	 * beginning at location "addr".
	 */
	register INT32 sum = 0;
	UINT16 *source = (UINT16 *) addr;

	while (count > 1)
	{
		/*  This is the inner loop */
		sum += *source++;
		count -= 2;
	}

	/*  Add left-over byte, if any */
	if (count > 0)
	{
		/*
		 * Make sure that the left-over byte is added correctly both
		 * with little and big endian hosts
		*/
		UINT16 tmp = 0;
		*(UINT8 *) (&tmp) = * (UINT8 *) source;
		sum += tmp;
	}
	/*  Fold 32-bit sum to 16 bits */
	while (sum >> 16)
	{
		sum = (sum & 0xffff) + (sum >> 16);
	}

	return ~sum;
}

