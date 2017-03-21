
#include "iot_event_loop.h"
#include "iot_msg_deal.h"


static void clear_msg_queue(int queue_id, unsigned char dst_mod_mod);


int main(int argc, char *argv[])
{
	int res, i;
	int queue_id=-1;
	IOT_MSG msg_rbuf, msg_sbuf;
	
	while(1)
	{
		if(queue_id==-1){
			queue_id = iot_msg_queue_get(IOT_MODULE_EVENT_LOOP);
			if(queue_id == -1)
			{
				fprintf(stderr, "get event loop's msg queue failed, try again after %d second,(res=%d, err:%s)\n", 
							IOT_MSG_SQUEUE_GET_INTERVAL, queue_id, strerror(errno));
				sleep(IOT_MSG_SQUEUE_GET_INTERVAL);
				continue;
			}

			clear_msg_queue(queue_id, IOT_MODULE_EVENT_LOOP);
		}

		msg_rbuf.dst_mod = IOT_MODULE_EVENT_LOOP;
		res = iot_msg_recv_wait(queue_id, &msg_rbuf);
		if(res<0)
		{
			fprintf(stderr, "recv msg failed, try again after %d ms(res=%d, err:%s)\n", 
						IOT_MSG_RECV_RETRY_INTERVAL, res,strerror(errno));
			usleep(IOT_MSG_RECV_RETRY_INTERVAL*1000);
			continue;
		}
		fprintf(stderr, "recv msg success, res=%d, src_mod=%d, dst_mod=%d, body_len=%d, body=\n\n%s\n\n", 
			res, msg_rbuf.src_mod, msg_rbuf.handler_mod, msg_rbuf.body_len, msg_rbuf.body);

		if(msg_deal(&msg_rbuf, &msg_sbuf)<0)
		{
			fprintf(stderr, "iot msg deal failed, discard it and deal next msg...\n");
			continue;
		}

		i = 0;
		for(i=0;i < IOT_MSG_SEND_RETRY_TIMES; i++)
		{
			res = iot_msg_send(queue_id, &msg_sbuf);
			if(res<0)
			{
				fprintf(stderr,"send msg failed(have retry %d times), res=%d, err:%s\n", i, 
									res,strerror(errno));
				usleep(IOT_MSG_SEND_RETRY_INTERVAL*1000);
			}else
			{
				break;
			}			
		}
		if(i == IOT_MSG_SEND_RETRY_TIMES)
		{
			fprintf(stderr, "iot msg send failed, discard it and deal next msg...\n");
		}
	}

	return 0;
}


static void clear_msg_queue(int queue_id, unsigned char dst_mod)
{
	int res;
	IOT_MSG msg;
	while(1){
		msg.dst_mod = dst_mod;
		res = iot_msg_recv_nowait(queue_id, &msg);
		if(res<0)
			break;
	}
}