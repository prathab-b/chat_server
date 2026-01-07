#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/epoll.h>

#define MAX_EVENTS 1024
#define MAX_CLIENTS 1024
#define MAX_NAME_LENGTH 1024
#define MAX_BUFFER 1024

typedef struct client
{
    int sfd;
    char name[MAX_NAME_LENGTH];
    int registered;
} client;

client *clients[MAX_CLIENTS];
int client_count = 0;

void ignore_sigpipe(void)
{
  struct sigaction myaction;

  myaction.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &myaction, NULL);
}

int non_blocking_mode(int sfd)
{
    int flags;

    if ((flags = fcntl(sfd, F_GETFL)) == -1)
    {
        perror("fcntl");
        return -1;
    }

    return fcntl(sfd, F_SETFL, flags | O_NONBLOCK);
}

void new_client(int cfd)
{
    if (client_count < MAX_CLIENTS)
    {
        clients[client_count] = (client *)calloc(1, sizeof(client));
        clients[client_count]->sfd = cfd;
        client_count++;
    }

    else
    {
        perror("new_client, too many clients");
    }
}

int find_client(int cfd)
{
    for (int i = 0; i < client_count; i++)
    {
        if (clients[i]->sfd == cfd)
        {
            return i;
        }
    }

    return -1;
}

void disconnect_client(int cfd, int e);

void broadcast(char message[MAX_BUFFER], int e)
{
    for (int i = 0; i < client_count; i++)
    {
        if (clients[i]->registered == 1)
        {
            if (write(clients[i]->sfd, message, strlen(message)) == -1)
            {
                disconnect_client(clients[i]->sfd, e);
            }
        }
    }
}

void disconnect_client(int cfd, int e)
{
    int client_num = find_client(cfd);

    if (client_num == -1)
    {
        return;
    }

    if (clients[client_num]->registered == 1)
    {
        char message[MAX_BUFFER + 6];
        snprintf(message, sizeof(message), "%s LEFT\n", clients[client_num]->name);
        broadcast(message, e);
    }

    clients[client_num]->registered = 0;
    free(clients[client_num]);

    if (client_num != client_count - 1)
    {
        clients[client_num] = clients[client_count - 1];
    }

	clients[client_count - 1] = NULL;

    client_count--;

    epoll_ctl(e, EPOLL_CTL_DEL, cfd, NULL);

    close(cfd);
}

void interpret_message(char buffer[MAX_BUFFER], int cfd, int ep)
{
    int client_num = find_client(cfd);
    char message[MAX_BUFFER * 2 + 2];

    if (client_num == -1)
    {
        return;
    }

    if (strncmp(buffer, "JOIN", 4) == 0)
    {
        char name[MAX_NAME_LENGTH];

        if (sscanf(buffer, "JOIN %1024s\n", name) == 1)
        {
            for (int i = 0; i < client_count; i++)
            {
                if (strcmp(name, clients[i]->name) == 0)
                {
                    if (write(clients[client_num]->sfd, "USERNAME TAKEN\n", 15) == -1)
                    {
                        disconnect_client(clients[client_num]->sfd, ep);
                    }

                    return;
                }
            }
        }

        else
        {
            if (write(clients[client_num]->sfd, "INVALID JOIN\n", 13) == -1)
            {
                disconnect_client(clients[client_num]->sfd, ep);
            }

            return;
        }

		strncpy(clients[client_num]->name, name, MAX_NAME_LENGTH - 1);
		clients[client_num]->name[MAX_NAME_LENGTH - 1] = '\0';

        clients[client_num]->registered = 1;

        snprintf(message, sizeof(message), "WELCOME %s\n", name);
        broadcast(message, ep);
    }

    else if (strncmp(buffer, "MESSAGE", 7) == 0)
    {
        if (clients[client_num]->registered == 0)
		{
			if (write(clients[client_num]->sfd, "REGISTER FIRST\n", 15) == -1)
			{
				disconnect_client(clients[client_num]->sfd, ep);
			}

			return;
		}
		
		char recipient[MAX_NAME_LENGTH];
		char message_to_send[MAX_BUFFER];

        if (sscanf(buffer, "MESSAGE %1024s %[^\n]", recipient, message_to_send) == 2)
        {
            int found = 0;

			for (int i = 0; i < client_count; i++)
			{
				if (strcmp(clients[i]->name, recipient) == 0)
				{
					snprintf(message, sizeof(message), "%s: %s\n", clients[client_num]->name, message_to_send);
					
					if (write(clients[i]->sfd, message, strlen(message)) == -1)
					{
						disconnect_client(clients[i]->sfd, ep);
					}

					found = 1;
					break;
				}
			}

			if (found == 0)
			{
				if (write(clients[client_num]->sfd, "USER NOT FOUND\n", 15) == -1)
                {
                    disconnect_client(clients[client_num]->sfd, ep);
                }
			}
        }

		else
		{
			if (write(clients[client_num]->sfd, "INVALID MESSAGE\n", 16) == -1)
            {
                disconnect_client(clients[client_num]->sfd, ep);
            }
		}
    }

	else if (strncmp(buffer, "QUIT", 4) == 0)
	{
    	disconnect_client(cfd, ep);
	}
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		perror("invalid argument count");
		return 1;
	}
    int sfd;
    struct sockaddr_in a;

    ignore_sigpipe();

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&a, 0, sizeof(struct sockaddr_in));

	a.sin_family = AF_INET;
    a.sin_port = htons(atoi(argv[1]));
    a.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sfd, (struct sockaddr *)&a, sizeof(struct sockaddr_in)) == -1)
    {
        perror("bind");
        return -1;
    }

    if (listen(sfd, 2) == -1)
    {
        perror("listen");
        return -1;
    }

    if (non_blocking_mode(sfd) == -1)
    {
        perror("non_blocking_mode");
        return -1;
    }

    int ep;
    struct epoll_event e;

    if ((ep = epoll_create1(0)) == -1)
    {
        perror("epoll_create1");
        return -1;
    }

    e.events = EPOLLIN;
    e.data.fd = sfd;

    if (epoll_ctl(ep, EPOLL_CTL_ADD, sfd, &e) == -1)
    {
        perror("epoll_ctl");
        return -1;
    }

    for (;;)
    {
        int n;
        struct epoll_event es[MAX_EVENTS];

        n = epoll_wait(ep, es, MAX_EVENTS, -1);

        for (int i = 0; i < n; i++)
        {
            if (es[i].events & (EPOLLHUP | EPOLLRDHUP))
            {
                disconnect_client(es[i].data.fd, ep);
            }

            else
            {
                if (es[i].data.fd == sfd)
                {
                    int cfd;
                    struct sockaddr_in ca;
                    socklen_t sinlen;

                    sinlen = sizeof(struct sockaddr_in);
                    if ((cfd = accept(sfd, (struct sockaddr *)&ca, &sinlen)) != -1)
                    {
                        if (non_blocking_mode(cfd) == -1)
                        {
                            perror("non_blocking_mode, client");
                            return -1;
                        }

                        e.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                        e.data.fd = cfd;
                        epoll_ctl(ep, EPOLL_CTL_ADD, cfd, &e);

                        new_client(cfd);
                    }

                    else
                    {
                        perror("accept");
                    }
                }

                else
                {
                    char buffer[MAX_BUFFER];
                    int num_read = read(es[i].data.fd, buffer, MAX_BUFFER);
                    int num_chars = 0;
                    int client_found = 0;

					if (num_read > 0)
					{
						buffer[num_read] = '\0';
					}

                    for (int j = 0; j < client_count; j++)
                    {
                        if (clients[j]->sfd == es[i].data.fd)
                        {
                            client_found = 1;
                        }
                    }

                    if (client_found == 1)
                    {
                        if (num_read > 0)
                        {
                            while (buffer[num_chars] != '\n' && num_chars < 100)
                            {
                                num_chars++;
                            }

                            if (num_chars < 100 || buffer[99] == '\n')
                            {
                                buffer[num_chars] = '\0';

                                interpret_message(buffer, es[i].data.fd, ep);
                            }

                            else
                            {
                                disconnect_client(es[i].data.fd, ep);
                            }
                        }

                        else
                        {
                            disconnect_client(es[i].data.fd, ep);
                        }
                    }
                }
            }
        }
    }
}