#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include "calender.h"
#include <sstream>

#define USERDATA "../users/"
#define MYPORT		"7000"		// default port
#define BACKLOG		10
#define MYIP		"127.0.0.1"

void *networkingThread ( void *arg);

int main( int argc , char *argv[] )
{
	pthread_t threadId;
	string port;
	string authenticate[1024];	//used for getall only
	for(int i=0;i<1024;i++)
		authenticate[i] = "";
	string servermode;
	if ( argc == 3 )
	{
		servermode = argv[1];
		port = argv[2];
		cout<<"Server will start with port:"<<port<<"\n";
	}
	else if (argc == 2)
	{
		servermode = argv[1];
		port = MYPORT;
		cout<<"\nNo port specified, Continuing with default port:"<< MYPORT <<"\n\n";
	}
	else
	{
		servermode = "0";
	}

    int							sock_desc;		// listening socket descriptor
	int							new_socket;		// newly accept()ed socket descriptor
	struct sockaddr_storage		remoteaddr;		// client address
    socklen_t					addrlen = sizeof remoteaddr;

	int							bufsize = BUFSIZE;
	char						buffer[bufsize];//buffer for client data
	struct addrinfo				hints;			//hints for getaddrinfo
	struct addrinfo				*result;		//point to the result
	int							status;
	
	memset(&hints, 0, sizeof hints);			//make sure the struct is empty
	hints.ai_family = AF_UNSPEC;				//don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;			//TCP stream sockets
	hints.ai_flags = AI_PASSIVE;    		 	// fill in my IP for me
	
	// status = getaddrinfo( MYIP , port , &hints , &result);	//use designated ip
	status = getaddrinfo( NULL , port.c_str() , &hints , &result);	//use system ip
	if ( status != 0)
	{
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
	}
	
	sock_desc = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if ( sock_desc == -1)
	{
		perror("create socket:- ");
		exit(2);
	}
	/* The SO_REUSEADDR is for when the socket bound to an address has already been closed,
	 * the same address (ip-address/port pair) can be used again directly.
	 */
    int	yes=1;
    if ( setsockopt(sock_desc, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 )
    {
		cout<<"Could not free port\n";
        exit(1);
	}
	
	/* bind the socket to the port specified above */
	int success = bind ( sock_desc , result->ai_addr , result->ai_addrlen );
	if(success != 0)
	{
		close(sock_desc);
		perror("bind:- ");
		exit(2);
	}

	freeaddrinfo(result);	//no use afterwards

	/* listening socket */
	if (listen(sock_desc, BACKLOG) == -1)
	{
		close (sock_desc);
	}
	/*---------------------Server Initialization complete--------------------------*/
	
	
	int mode = atoi( servermode.c_str() );
	switch ( mode )
	{
		/* ITERATIVE APPROACH */
		case 1:
		{
			int loopno = 0;
			while(1)
			{
				cout<<"loop :"<<loopno++<<"\n";
				/* accept a connection */
				new_socket = accept(sock_desc, (struct sockaddr *)&remoteaddr, &addrlen);
				if (new_socket < 0)
				{
					perror("Accept connection:- ");
					exit(1);
				}
				
				memset(buffer, 0, bufsize);
				
				/* receive querry */
				int rdatalen = recv ( new_socket, buffer, bufsize, 0);
				if ( rdatalen <= 0 )
				{
					perror("Receive:- ");
				}
				else
				/* querry received correctly from client */
				{
					char message[BUFSIZE];
					/* process the user querry using 'handleClientData' function 
					 * and store the result in string 'response'
					 */
					string response = handleClientData(new_socket, buffer, 0, authenticate);
					strcpy(message,response.c_str());	//string to char
					/* send response to client */
					int sdatalen = send(new_socket, message, strlen(message), 0);
					if(sdatalen == -1)
						cout<<"error sending\n";
					close(new_socket);	//closse the connection
				}
			}
			break;
		}
		/*-----------------------------------------------------------------------------*/
		/* SELECT MODE */
		case 2:
		{
			fd_set 				master;    // master file descriptor list
			fd_set 				read_fds;  // temp file descriptor list for select()
			int 				fdmax;     // maximum file descriptor number
			
			FD_ZERO(&master);				// clear the entries from the master set
			FD_ZERO(&read_fds);				// Clear all entries from the temporary set read_fds
			/* add the listener to the master set */
			FD_SET ( sock_desc , &master );
			
			/* keep track of the biggest file descriptor */
			fdmax = sock_desc;
			int loopNo = 1;
			while(1)
			{
				cout<<"loop :"<<loopNo++<<" + fdmax :"<<fdmax<<"\n";
				read_fds = master;
				if ( select ( fdmax+1 , &read_fds , NULL , NULL , NULL) == -1)
				{
					perror("select:- ");
					exit(2);
				}
				
				/* run through the existing connection looking for data to read */
				for ( int i = 0; i <= fdmax; i++ )
				{
					if ( FD_ISSET ( i , &read_fds) )
					{
						if ( i == sock_desc )
						{
							/* handle new connections */
							cout << "Ready to accept a new connection\n";
							addrlen = sizeof remoteaddr;
							/* accept a connection */
							new_socket = accept(sock_desc, (struct sockaddr *)&remoteaddr, &addrlen);
							if (new_socket < 0)
							{
								perror("Accept connection:- ");
								exit(1);
							}
							else
							{
								FD_SET (new_socket , &master ); // add to master set
								if ( new_socket > fdmax )
								{
									fdmax = new_socket;
								}
							}
						}
						else
						{
							/* handle data from client */
							memset(buffer, 0, bufsize);
							
							/* receive data */
							int rdatalen = recv ( i, buffer, bufsize, 0);
							if ( rdatalen <= 0 )
							{
								/* got error or connection closed by client */
								if (rdatalen == 0) {
									/* connection closed */
									printf("selectserver: socket %d hung up\n", i);
								} else {
									perror("recv:- ");
								}
								//Deauthorize user
								authenticate[i] = "";
								cout<<"User deauthorized\n";
								close(i); // bye!
								FD_CLR(i, &master); // remove from master set
							}
							else
							{
								/* some data received from client */
								char message[BUFSIZE];
								/* process the user querry using 'handleClientData' function 
								 * and store the result in string 'response'
								 */
								string response = handleClientData(i, buffer, 1, authenticate);
								/* send appropriate response to client */
								strcpy(message,response.c_str());	//convert string to char
								int sdatalen = send(i, message, strlen(message), 0);
								if(sdatalen == -1)
									cout<<"error sending\n";
							}
						} /* END handling data from client */
					} /* END got new incoming connection */
				} /* END looping through file descriptors */
			} /* END main while loop */
			break;
		}

		/*-----------------------------------------------------------------------------*/
		/* MULTITHREADED */
		case 3:
		{
			while(1)
			{
				intptr_t sockfd = accept(sock_desc, (struct sockaddr *)&remoteaddr, &addrlen);
				if (sockfd < 0)
				{
					perror("Accept connection:- ");
					exit(1);
				}
				pthread_create ( &threadId, NULL, networkingThread, (void*)sockfd);
				pthread_detach ( threadId );
			}
			break;
			default:
			{
				cout<<"Invalid mode specified.\n";
				cout<<"Usage: ./server MODE PORT[optional]\n";
			}
		}
	}
	
	return (0);
}

/*-----------------------------------------------------------------------------*/

/* operation for each thread */
void *networkingThread( void *arg)
{
	intptr_t sockfd = intptr_t(arg);
	if (sockfd < 0)
	{
		perror("Accept connection:- ");
		exit(1);
	}
	char buffer[BUFSIZE];
	memset(buffer, 0, BUFSIZE);
	string authenticate[1024];

	/* receive data */
	while (recv ( sockfd, buffer, BUFSIZE, 0) > 0 )
	{
		cout <<"current socket fd : "<< sockfd <<"\n";
		/* some data received from client */
		char message[BUFSIZE];
		string response = handleClientData(sockfd, buffer, 1 , authenticate);
		strcpy(message,response.c_str());
		/* send appropriate response to client */
		int sdatalen = send(sockfd, message, strlen(message), 0);
		if(sdatalen == -1)
			cout<<"error sending\n";
		bzero(buffer, BUFSIZE);
	}
	/* deauthorize user */
	authenticate[sockfd] = "";
	cout<<"Socket "<<sockfd<<" hung up\n";
	close(sockfd); // bye!
	return 0;
}
