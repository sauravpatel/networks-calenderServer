#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include "calender.h"

#define USERDATA "../users/"
#define MYPORT		"7000"		// default port
#define BACKLOG		10
#define MYIP		"127.0.0.1"
#define BUFSIZE		1024		//size of sent/received message buffers

/* operations */
#define ADD			11
#define REMOVE		12
#define UPDATE		13
#define GET			14
#define GETALL		15
#define NEXTENTRY	16
#define INVALID		10

#define USERNAME	0
#define OPERATION	1
#define DATE		2
#define SEQNO		2
#define STARTIME	3
#define ENDTIME		4
#define EVENTNAME	5

/*
 * create socket
 * bind
 * listen
 * accept
 */
 
/* syntax
 * socket()	: int socket(int domain, int type, int protocol);
 * bind() 	: int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
 * listen	: int listen(int sockfd, int backlog);
 * accept()	: int accept(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
 * send()	: int send(int sockfd, const void *msg, int len, int flags );
 * recv()	: int receive(int sockfd, const void *msg, int len, int flags );
 */
 
void *networkingThread ( void *arg);
int getoperation(string operation);
string handleClientData( int cfd, char buffer[BUFSIZE], int smode, string (&authenticate)[1024]);

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
					char message[1024];
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
								char message[1024];
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

/* ----------------------------------------------------------------------------- */

int getoperation(string operation)
{
	if(operation.compare("add") == 0)
		return ADD;
	if(operation.compare("remove") == 0)
		return REMOVE;
	if(operation.compare("update") == 0)
		return UPDATE;
	if(operation.compare("get") == 0)
		return GET;
	if(operation.compare("getall") == 0)
		return GETALL;
	if(operation.compare("nextentry") == 0)
		return NEXTENTRY;
	return INVALID;
}

/*-----------------------------------------------------------------------------*/

string handleClientData( int cfd , char buffer[BUFSIZE] , int smode, string (&authenticate)[1024])
{
	/* MESSAGE PARSING
	 * Client sends a request in the form of 
	 * line containing words seperated by spaces in the format:
	 * username + operation + date + startime + (endtime) + (eventname)
	 */
	 
	/* parsing the message 
	 * 0:username
	 * 1:operation
	 * 2:date
	 * 3:start
	 * 4:end
	 * 5:event
	 */
	char *pch;
	int index=0;
	string cmd[6]={"","","","","",""};
	pch = strtok(buffer, " ");
	while ( pch != NULL )
	{
		cmd[index]=pch;
		index++;
		pch = strtok(NULL," ");
	}
	
	/* ------------------------------- */
	string opstatus="";	//status after operation is executed
	
	
	int operation = getoperation(cmd[OPERATION]);

	if(smode && authenticate[cfd].empty() )
	{
		/* Authorize user */
		authenticate[cfd] = string(cmd[USERNAME]);
		cout<<"Authenticxation successful\n";
	}
		
	switch(operation)
	{
		case ADD:
			opstatus = add( cmd[USERNAME], cmd[DATE], cmd[STARTIME], cmd[ENDTIME], cmd[EVENTNAME]);
			break;
			
		case REMOVE:
			opstatus = Remove( cmd[USERNAME], cmd[DATE], cmd[STARTIME]);
			break;
			
		case UPDATE:
			opstatus = update( cmd[USERNAME], cmd[DATE], cmd[STARTIME], cmd[ENDTIME], cmd[EVENTNAME]);
			break;
			
		case GET:
			opstatus = get( cmd[USERNAME], cmd[DATE], cmd[STARTIME]);
			break;
			
		case GETALL:
			opstatus = getall( cmd[USERNAME], smode);
			break;
		case NEXTENTRY:
		/* 
		 * Here we have a authentication type system. We have a array of string "authenticate" which stores username corresponding
		 * to a socket descriptor and the entry is removed when connection is closed. "nextentry()" is used to fetch jth entry 
		 * in the user database. "nextentry()" takes two parameter a)username supplied from authenticate array and b) entry no.
		 * message format of client : "nextentry" + "space" + "entry no."
		 * So using socket descriptor (i) we can only access the database of user who opened it.
		 */
			opstatus = NextEntry( authenticate[cfd], atoi(cmd[SEQNO].c_str()));
			break;
			
		default:
			opstatus = INVALIDOP ;
	}
	return opstatus;
}

/* --------------------------------------------------- */

/* operation for each thread */
void *networkingThread( void *arg)
{
	intptr_t sockfd = intptr_t(arg);
	if (sockfd < 0)
	{
		perror("Accept connection:- ");
		exit(1);
	}
	int bufsize = BUFSIZE;
	char buffer[bufsize];
	memset(buffer, 0, bufsize);
	string authenticate[1024];

	/* receive data */
	while (recv ( sockfd, buffer, bufsize, 0) > 0 )
	{
		cout <<"current socket fd : "<< sockfd <<"\n";
		/* some data received from client */
		char message[1024];
		string response = handleClientData(sockfd, buffer, 1 , authenticate);
		strcpy(message,response.c_str());
		/* send appropriate response to client */
		int sdatalen = send(sockfd, message, strlen(message), 0);
		if(sdatalen == -1)
			cout<<"error sending\n";
	}
	/* deauthorize user */
	authenticate[sockfd] = "";
	cout<<"selectserver: socket "<<sockfd<<" hung up\n";
	close(sockfd); // bye!
	return 0;
}
