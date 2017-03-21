#include <fcntl.h>
#include "wan_type.h"

/* DNS query */
#define	DNS_SERVER_PORT			(53)
#define	DNS_LEN					(12)

#define DNS_URL_LIST_LEN		(3)
#define DNS_TRANS_ID 			(0x0101)

#define	DNS_QUERY_FLAG			(0x0100)	/* query flag */
#define	DNS_QUERY_TYPE			(0x0001)	/* type A */
#define	DNS_QUERY_CLASS			(0x0001)	/* class inet */
#define DNS_QR_RESPOND			(0x8000)

/* query a URL one time, left 256 bytes for it */
#define MAX_DNS_QUERY_LEN 		(256)
#define MIN_INPUT_PARA_NUM 		(3)			/* bin,timeout,pri_dns */
#define MAX_INPUT_PARA_NUM		(4)			/* bin,timeout,pri_dns snd_dns */
#define MAX_DNS_QUERY_TIMEOUT 	(10.0)		/* max timeout wait for respond */

typedef struct input_para
{
	double timeout;
	unsigned int pri_dns;
	unsigned int snd_dns;
} INPUT_PARA;

typedef struct
{
	unsigned short transaction_id;
	unsigned short flag;
	unsigned short questions;
	unsigned short answers_rrs;
	unsigned short authority_rrs;
	unsigned short additional_rrs;
	char data[MAX_DNS_QUERY_LEN];
}DNS_PACKET;

typedef enum
{
	LINK_STATUS_INVALID = 0,
	LINK_STATUS_VALID	
} LINK_STATUS;

typedef struct wan_link_detection
{
	LINK_STATUS type;	/* detected wan link status */
} WAN_LINK_DETECTION;

WAN_LINK_DETECTION g_wan_link_detect;

/* global variable */
int g_trans_Id = 0;
const char *dns_detect_urls[DNS_URL_LIST_LEN] =
{
	"www.baidu.com",
	"www.qq.com",
	"devs.tplinkcloud.com.cn",
};

/* declaration for internal functions */
static int parse_input_para(int argc, char *argv[], INPUT_PARA *input_para);
static void wan_link_detection_process(INPUT_PARA *input_para);
static BOOL dns_query(UINT32 pri_dns, UINT32 snd_dns, double timeout);
static BOOL wan_has_reply(int sock, double timeout);
static BOOL packet_is_offer(char* packet);
static INT16 construct_query_field(char *url, char *query);

/************************************************************
  Function:		main()
  Description:	wan type auto detection
				para1 - seconds to timeout in microsecond unit.
				para2 - primary dns address.
				para3 - secondary dns address.
  Output:		N/A
  Return:		wanType or error code
************************************************************/
int main(int argc, char *argv[])
{
	UINT8 mac[ETH_ALEN];
	INPUT_PARA g_input_para;
	int ret = -1;

	if (argc < MIN_INPUT_PARA_NUM || argc > MAX_INPUT_PARA_NUM)
	{
		DBG_PRINT("para1 - seconds to timeout in microsecond unit .\n");
		DBG_PRINT("para2 - primary dns address.\n");
		DBG_PRINT("para3 - secondary dns address.\n");

		return INPUT_PARA_ERROR;
	}

	memset(&g_input_para, 0 , sizeof(INPUT_PARA));
	ret = parse_input_para(argc, argv, &g_input_para);
	RET_VAR_IF((ret != 0), ret);

	wan_link_detection_process(&g_input_para);
	DBG_PRINT("g_wan_link_detect.type is %d\n", g_wan_link_detect.type);
	return g_wan_link_detect.type;
}

/************************************************************
  Function:		parse_input_para()
  Description:	parse input paras from cmd line
  Input: 		input para from cmd line
  Output:		ifname, mac, trytimes, timeout
  Return:		N/A
************************************************************/
static int parse_input_para(int argc, char *argv[], INPUT_PARA *input_para)
{
	double time = 0;
	char *tmp;
	int base = 10;

	/*get timeout*/
	input_para->timeout = strtod(argv[1], &tmp);
	if ((input_para->timeout <= 0.0) || (input_para->timeout > MAX_DNS_QUERY_TIMEOUT))
	{
		DBG_PRINT("error: invalid time out setting,%s\n",argv[1]);
		return INPUT_PARA_ERROR;
	}
	
	input_para->timeout = (input_para->timeout) < (MAX_DNS_QUERY_TIMEOUT)? (input_para->timeout) : (MAX_DNS_QUERY_TIMEOUT);

	/* get Primary Dns address and Second Dns address */
	if (argc >= MIN_INPUT_PARA_NUM)
	{
		switch (argc)
		{
		case MIN_INPUT_PARA_NUM:
			base = 10;
			if ((!strncmp(argv[2], "0x", 2)) || (!strncmp(argv[2], "0X", 2)))
			{
				base = 16;
			}
			
			input_para->pri_dns = strtoul(argv[2], &tmp, base);
			
			if ((input_para->pri_dns == 0) || (input_para->pri_dns == ULONG_MAX))
			{
				DBG_PRINT("error:dns address error: %d\n", (input_para->pri_dns));
				return INPUT_PARA_ERROR;
			}
			DBG_PRINT("input_para->pri_dns is 0x%x\n", input_para->pri_dns);
			break;
			
		case MAX_INPUT_PARA_NUM:
			base = 10;
			if ((!strncmp(argv[2], "0x", 2)) || (!strncmp(argv[2], "0X", 2)))
			{
				base = 16;
			}			
			
			input_para->pri_dns = strtoul(argv[2], &tmp, base);
			
			base = 10;
			if ((!strncmp(argv[3], "0x", 2)) || (!strncmp(argv[3], "0X", 2)))
			{
				base = 16;
			}		
			
			input_para->snd_dns = strtoul(argv[3], &tmp, base);
			
			if ((input_para->pri_dns == 0) || (input_para->snd_dns == 0) ||
				(input_para->pri_dns == ULONG_MAX) || (input_para->snd_dns == ULONG_MAX))
			{
				DBG_PRINT("error:dns address error: %d, %d\n", (input_para->pri_dns), (input_para->snd_dns));
				return INPUT_PARA_ERROR;
			}
			break;
			
		default:
			DBG_PRINT("error:too many input para: %d\n");
			return INPUT_PARA_ERROR;
		}
	}
		
	return 0;
}

/************************************************************
  Function:		wan_link_detection_process()
  Description:	Detect WAN port available connection type
  Input: 		index - The WAN port index
  Output:		N/A
  Return:		N/A
************************************************************/
static void wan_link_detection_process(INPUT_PARA *input_para)
{
	BOOL ret = FALSE;
	
	/* DNS query */
	ret = dns_query(htonl(input_para->pri_dns), htonl(input_para->snd_dns), input_para->timeout);
	if (ret == TRUE)
	{
		g_wan_link_detect.type = LINK_STATUS_VALID;
		return;
	}

	g_wan_link_detect.type = LINK_STATUS_INVALID;

	return;
}

/************************************************************
  Function:		dns_query()
  Description:	Detect whether DNS server is available.
  Input: 		ifname - The interface name of the WAN port
  				timeout - Timeout of EACH protocal detection loop.
  Output:		N/A
  Return:		TRUE - DNS is available
  				FALSE - DNS is unavailable
  Others:		added by LSY, 4Jan15 ,执行DNS请求流程
************************************************************/
static BOOL dns_query(UINT32 pri_dns, UINT32 snd_dns, double timeout)
{
	static unsigned int url_idx = 0;

	int sock_dns;
	int flag;
	DNS_PACKET dns_packet;
	short query_len;
	short packet_len;
	int sent;
	int ret;
	struct sockaddr_in addr_server;

	if (pri_dns == 0)
	{
		DBG_PRINT("No valid Dns Address\n");
		return FALSE;		
	}
	
	sock_dns = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock_dns < 0)
	{
		DBG_PRINT("Open dns socket failed\n");
		return FALSE;
	}

	flag = fcntl(sock_dns, F_GETFL, 0);
	if (fcntl(sock_dns, F_SETFL, flag | O_NONBLOCK) < 0)
	{
		DBG_PRINT("Set socket nonblock failed\n");
		close(sock_dns);
		return FALSE;
	}

	memset(&dns_packet, 0, sizeof(dns_packet));
	dns_packet.flag = htons(DNS_QUERY_FLAG);
	
	g_trans_Id = DNS_TRANS_ID;
	while (url_idx < DNS_URL_LIST_LEN)
	{
		g_trans_Id += 1;
		dns_packet.transaction_id = htons(g_trans_Id);
		dns_packet.questions = htons(1);

		query_len = construct_query_field((char *)dns_detect_urls[url_idx], dns_packet.data);
		url_idx += 1;
		packet_len = query_len + DNS_LEN;

		addr_server.sin_family		= AF_INET;
		addr_server.sin_port		= htons(DNS_SERVER_PORT);
		addr_server.sin_addr.s_addr = pri_dns;
		sent = sendto(sock_dns, &dns_packet, packet_len, 0, (struct sockaddr*)&addr_server, sizeof(addr_server));
		if (sent < 0)
		{
			DBG_PRINT("Send dns detect packet failed %d\n", sent);
		}
		
		if (snd_dns > 0)
		{
			addr_server.sin_addr.s_addr = snd_dns;
			sent = sendto(sock_dns, &dns_packet, packet_len, 0, (struct sockaddr*)&addr_server, sizeof(addr_server));
			if (sent < 0)
			{
				DBG_PRINT("Send dns detect packet failed    %d\n", sent);
			}
		}
	}
	
	url_idx = 0;
	ret = wan_has_reply(sock_dns, timeout);	
	close(sock_dns);
	
	return ret;
}

/************************************************************
  Function:		wan_has_reply()
  Description:	Test if wan port has offer reply or not.
  Input: 		sock - The socket
  				timeout - Time out of each loop.
  Output:		N/A
  Return:		TRUE - WAN port has relpy.
  				FALSE - WAN port does not has reply.
************************************************************/
static BOOL wan_has_reply(int sock, double timeout)
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

		tv.tv_sec = (long)timeout;		
		tv.tv_usec = (long)(1000000 * (timeout - (double)tv.tv_sec));	

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

				if (packet_is_offer(buf) == TRUE)
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
  Output:		N/A
  Return:		TRUE - The relpy is offer packet.
  				FALSE - The relpy is not offer packet.
************************************************************/
static BOOL packet_is_offer(char* packet)
{
	DNS_PACKET* dns = (DNS_PACKET*) &packet[0];
	BOOL ret = FALSE;

	/* For Respond of DNS, trans_id meet range, with an answer */
	if (ntohs(dns->transaction_id) > DNS_TRANS_ID &&
		ntohs(dns->transaction_id) <= DNS_TRANS_ID + DNS_URL_LIST_LEN &&
		(ntohs(dns->flag) & DNS_QR_RESPOND) &&
		(ntohs(dns->answers_rrs) | ntohs(dns->authority_rrs)) )
	{
		DBG_PRINT("receive DNS Respond.\n");
		ret = TRUE;
	}			

	return ret;
}

/************************************************************
  Function:		construct_query_field()
  Description:	Construct dns query content.
  Input: 		Construct dns query packet as data included 
				in UDP packet
  Output:		N/A
  Return:		The dns query packet size
************************************************************/
static INT16 construct_query_field(char *url, char *query)
{
	char * des = query + 1;		//point destion string for copy data
	char * src = url;			//point source string for copy
	char * pn = query;			//point the number position of destion string
	char n = 0;					//number
	INT16 counter = des-query;	//counter
	USHORT ntype, nclass;

	if ((!url) || (strlen(url) <= 0))
	{
		return 0;
	}
	
	while ((*src != '\0') && (*src != '/'))
	{
		if (*src == '.')
		{
			*pn = n;
			pn = pn + n + 1;
			n = 0;
		}
		else
		{
			*des = *src;
			n++;
		}
		
		des++;
		src++;
		counter++;
	}
	
	if (n != 0)
	{
		*pn = n;
	}
	
	*des++ = '\0';
	counter++;
	ntype = htons(DNS_QUERY_TYPE);		/* type */
	memcpy(des, &ntype,sizeof(ntype));
	des += sizeof(ntype);
	nclass = htons(DNS_QUERY_CLASS);		/* class */
	memcpy(des, &nclass, sizeof(nclass));
	counter = counter + sizeof(ntype) + sizeof(nclass);
	
	return counter;
}
