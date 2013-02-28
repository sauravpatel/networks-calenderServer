#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <map>
#include <sstream>
#include "clientMacros.h"

#define BUFSIZE 2560

using namespace std;

/* Integer to string conversion */
string Inttostr(int num)
{
	stringstream ss;
	ss << num;
	return ss.str();
}


void GetallEntry( int sockfd , int entrycount);

/* ERROR CODES
 * 20-operation successfull
 * 21-conflict detected
 * 22-event does not exist
 * 23-invalid number of arguments
 * 24-startime > endtime
 * 25-get event name
 * 26-all event of the day [event lists]
 * 27-getall reply[no. of events]
 * 28-The server is not configured to perform requested function
 * 29-invalid operation type
 * 30-server error
 * 31-nothing to update ( same event already present)
 */
 
void usage()
{
	//cout<<"USAGE :: [program] [HOST] [PORT] [USERNAME] [OPERATION] [DATE] [STARTTIME] [ENDTIME] [EVENTNAME]\n";
	cout<<"A: Adding an event	: ./client hostname port username add date start end event\n";
	cout<<"B: Removing an event	: ./client hostname port username remove date start\n";
	cout<<"C: Updating an event	: ./client hostname port username update date start end event \n";
	cout<<"D: Getting event type: ./client hostname port username get date start \n";
	cout<<"E: All event of day	: ./client hostname port username get date \n";
	cout<<"F: All event of user**	: ./client hostname port username getall \n";
	cout<<"Date Format\t\t:  MMDDYY\n";
	cout<<"Time Format\t\t:  HHMM(24 hour clock)\n";
	cout<<"* Event must finish on the day it started.\n";
	cout<<"** Not available for server running in iterative mode \n\n";
}

/* Mapping of reply code to its correct meaning and take appropriate action.
 * If reply code is for 'getall' function 'getallentry' is called and for
 * all other cases, appropriate response is printed
 */
void processReply( string response , int sock_desc )
{
	int condcodes = atoi(response.substr(0,2).c_str());
	response = response.substr(2);
	map<int,string> operationCodes;
	operationCodes[SUCCESS			]	= "Operation successful.";
	operationCodes[CONFLICT			]	= "Conflict Detected with entry : ";
	operationCodes[NOEVENTEXIST		]	= "Event does not exist.";
	operationCodes[INVALIDARGCOUNT	]	= "Invalid number of arguments.\nType \'help\' to see usage.";
	operationCodes[WRONGDATE		]	= "Invalid date-time.";
	operationCodes[EVENTTYPE		]	= "Requested event name is : ";
	operationCodes[DAYEVENTLIST		]	= "Event(s) of the day are : \n";
	operationCodes[UNAUTHORIZED		]	= "The server is not configured to perform GETALL function. Please try again later!!!";
	operationCodes[INVALIDOP		]	= "Invalid operation.";
	operationCodes[SERVERERROR		]	= "Server failure.";
	operationCodes[REPEATEDEVENT	]	= "Same event already exists.";

		switch(condcodes)
		{
		case GETALLCOUNT:
			{
				int entrycount = atoi(response.c_str());
				cout<<"Number of events are : "<<entrycount<<"\n";
				if(entrycount > 0)
					GetallEntry( sock_desc , entrycount);
				break;
			}
		default:
			cout<<operationCodes[condcodes]<<response<<"\n";
			break;
		}
}

int main( int argc , char **argv )
{	
	if(argc < 5 )
	{
		usage();
		exit(1);
	}

	string hostname, port, message="";
	hostname = argv[1];
	port = argv[2];

	string seperator = "\t";
	for (int i=3;i<argc; i++)
		message += (argv[i] + seperator);
	
	int					sock_desc;
	char				buffer[BUFSIZE];
	struct addrinfo		hints;
	struct addrinfo		*servinfo;	//point to the result
	int					status;
	
	memset(buffer, 0, BUFSIZE);
	memset(&hints, 0, sizeof hints);	//make sure the struct is empty
	hints.ai_family = AF_UNSPEC;		//don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;	//TCP stream sockets
	//hints.ai_flags = AI_PASSIVE;		//fill in my IP for me;
	
	status = getaddrinfo( hostname.c_str(), port.c_str() , &hints , &servinfo);
	if ( status != 0)
	{
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}
	
	/* create a new socket */
	sock_desc = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

	/* connect */
	int new_socket = connect ( sock_desc , servinfo->ai_addr , servinfo->ai_addrlen );
	if( new_socket != 0)
	{
		perror("error connecting\n");
		exit(1);
	}

	/* send request to server */
	char *msg = new char[message.length() + 1];
	strcpy ( msg , message.c_str() );
    int sdatalen = send(sock_desc, msg, strlen(msg), 0);
	if ( sdatalen == -1)
		perror("error sending");
	free(msg);
	
	/* wait for result from server */
	int rdatalen = recv ( sock_desc, buffer, BUFSIZE, 0);
	if(rdatalen <= 0)
	{
		cout<<"Error receiving data\n";
		exit(1);
	}
	//reply from server
	processReply( string(buffer) , sock_desc );	
	close(sock_desc);
	return (0);
}


/* Special codes for getall function
 * Input parameters:
 * 		1.sockfd - socket file descriptor on which connection is established
 * 		2.entrycount - total number of entry required.
 * After each 2 seconds it sends a request to get ith event detail from the server.
 * query format : 'username'+\t+'nextentry'+\t+'i'
 * username is not used in server side, so it can be null ( just for the shake of uniformity during parsing at server side )
 * At server side 'nextentry' function handles the task of getting ith enytry
 */
	
void GetallEntry( int sockfd , int entrycount)
{
	char buffer[BUFSIZE];
	cout<<"The events are : "<<entrycount<<" : \n";
	cout<<"\tDate\t\tStart time\tEnd time\tEvent Name\n";
	for ( int i = 1; i <= entrycount; i++)
	{
		sleep(2);
		string request = "anything\tnextentry\t" + Inttostr(i) + "\t";	//for uniformity in "getoperation()" 'anything\t' is added in the beginning
		char *msg = new char[request.length() + 1];
		strcpy ( msg , request.c_str() );
		if( send(sockfd, msg, strlen(msg), 0) <=0 )
		{
			cout<<"Error sending \"getNext\" request.....aborting.......\n";
			exit(2);
		}
		memset(buffer, 0, BUFSIZE);
		if ( recv ( sockfd, buffer, BUFSIZE, 0) > 0)
		{
			cout<<"\t"<<i<<". "<<buffer<<"\n";
		}
	}
}
