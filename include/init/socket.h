#include <netex/ifctl.h>

#define getPort(p1x, p2x) ((p1x * 256) + p2x)

static int ssend(int socket, const char *str)
{
	return send(socket, str, strlen(str), 0);
}

static int connect_to_server_ex(const char *server_ip, u16 port, u8 rcv_timeout)
{
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if(s < 0)
	{
		return FAILED;
	}

	struct sockaddr_in sin;
	socklen_t sin_len = sizeof(sin);
	_memset(&sin, sin_len);

	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	unsigned int ip_address;

	if((ip_address = inet_addr(server_ip)) != (unsigned int)-1)
	{
		sin.sin_addr.s_addr = ip_address;
	}
	else
	{
		struct hostent *hp;

		if((hp = gethostbyname(server_ip)) == NULL)
		{
			return FAILED;
		}

		sin.sin_family = hp->h_addrtype;
		memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);
	}

	struct timeval tv;
	tv.tv_usec = 0;
	tv.tv_sec = rcv_timeout ? rcv_timeout : 30;
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

	if(rcv_timeout)
		setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	if(connect(s, (struct sockaddr *)&sin, sin_len) < 0)
	{
		return FAILED;
	}

	tv.tv_sec = 60;
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
	//setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	return s;
}

static int connect_to_server(const char *server_ip, u16 port)
{
	return connect_to_server_ex(server_ip, port, false);
}

static void sclose(int *socket_e)
{
	if(*socket_e != NONE)
	{
		shutdown(*socket_e, SHUT_RDWR);
		socketclose(*socket_e);
		*socket_e = NONE;
	}
}

static int slisten(int port, int backlog)
{
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if(s < 0)
	{
		return FAILED;
	}

	//int optval = 1;
	//setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	//setsockopt(s, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval));
	//setsockopt(s, SOL_SOCKET, SO_SNDBUF, &optval, sizeof(optval));

	struct sockaddr_in sa;
	socklen_t sin_len = sizeof(sa);
	_memset(&sa, sin_len);

	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(s, (struct sockaddr *)&sa, sin_len) < 0)
	{
		sclose(&s);
		return FAILED;
	}

	listen(s, backlog);

	return s;
}

#define MIN_KERNEL_FREE_SPACE 256*1024 // 256KiB
#define MIN_KERNEL_MAX_TRIES 10 // Try 10 times
#define MIN_KERNEL_RETRY_PAUSE 2 // 2 seconds

static bool wait_for_new_socket(void)
{
	int failures = 0;
	while(true)
	{
		unsigned int kernel_free_current = 0;
		sys_net_if_ctl(0, SYS_NET_CC_GET_MEMORY_FREE_CURRENT, &kernel_free_current, sizeof(kernel_free_current));

		if(kernel_free_current >= MIN_KERNEL_FREE_SPACE)
		{
			break; // There's enough space for new socket
		}
		else
		{
			failures++;

			if(failures >= MIN_KERNEL_MAX_TRIES)
			{
				return true; // Kernel is full: Not enough space found
			}

			sys_ppu_thread_sleep(MIN_KERNEL_RETRY_PAUSE); // Give kernel more time to free up space
		}
	}
	return false;
}

#ifdef FIX_CLOCK
static u32 get_server_data(char *url, char *data, u32 size)
{
	if(!islike(url, "http://")) return 0;

	strtok(url, "/"); // http://
	const char *hostname = strtok(NULL, "/");
	const char *path = strtok(NULL, "#");
	int port = 80; char *sep = strchr(hostname, ':'); if(sep) port = val(sep + 1);

	int socket_e = connect_to_server(hostname, port);
	if(socket_e >= 0)
	{
		int req_len = sprintf(data,
					 "GET /%s HTTP/1.1\r\n"
					 "Host: %s\r\n"
					 "Connection: close\r\n"
					 "User-Agent: " WM_APPNAME "/1.0\r\n"
					 "\r\n",
					 path, hostname);

		if(send(socket_e, data, req_len, 0) == req_len)
		{
			recv(socket_e, data, size, MSG_WAITALL);
			sclose(&socket_e);

			char *pos = strstr(data, "\r\n\r\n");
			if(pos)
			{
				pos += 4; u32 len = MIN((u32)convertH(pos), (u32)size);
				char *pos2 = strstr(pos, "\r\n"); if(pos2) pos = pos2 + 2;
				strncpy(data, pos, len); data[len] = 0;
				return (len - 1);
			}
		}
		sclose(&socket_e);
	}
	return 0;
}
#endif
