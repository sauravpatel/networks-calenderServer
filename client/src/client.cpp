#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include "macros.h"
#include <string.h>
#include <time.h>
#include <sstream>

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
 * 0-operation successfull
 * 1-conflict detected
 * 2-event does not exist
 * 3-invalid number of arguments
 * 4-startime > endtime
 * 5-get event name
 * 6-all event of the day [event lists]
 * 7-getall reply[no. of events]
 * 8-permission denied
 * 9-unknown error, try later
 */
/* syntax
 * socket()	: int socket(int domain, int type, int protocol);
 * connect(): int connect(int sockfd, const struct sockaddr *addr, socklen_t  addrlen);
 * read()	: ssize_t read(int fd, void *buf, size_t count);
 * write()	: ssize_t write(int fd, void *buf, size_t count);
 * send()	: int send(int sockfd, const void *msg, int len, int flags );
 * recv()	: int receive(int sockfd, const void *msg, int len, int flags );
 */
 
int main( int argc , char **argv )
{	
	if(argc < 5 )
	{
		string s=argv[1];
		cout<<"USAGE :: [program] [HOST] [PORT] [USERNAME] [OPERATION] [DATE] [STARTTIME] [ENDTIME] [EVENTNAME]\n";
		cout<<"A: Adding an event	: ./mycal hostname port myname add date start end Exam\n";
		cout<<"B: Removing an event	: ./mycal hostname port myname remove date start\n";
		cout<<"C: Updating an event	: ./mycal hostname port myname update date start end OralExam \n";
		cout<<"D: Getting event type	: ./mycal hostname port myname get date start \n";
		cout<<"E: All event of day	: ./mycal hostname port myname get date \n";
		cout<<"F: All event of user**	: ./mycal hostname port myname getall \n";
		cout<<"** Not available for server running in iterative mode \n\n";
		cout<<"Date Format\t\t:  MMDDYY\n";
		cout<<"Time Format\t\t:  HHMM(24 hour clock)\n";
		exit(1);
	}
	
	if ( argc < 5 || argc > 9)
	{
		cout << "Invalid number of arguments.\n";
		cout << "Type \'help\' to see usage.\n";
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
	string response = string(buffer);
	int pos = response.find(" ");
	int condcodes = atoi(response.substr(0,pos).c_str());
	response = response.substr(pos+1);
	switch(condcodes)
	{
		case SUCCESS:
			cout<<"Operation successful.\n";
			break;
		case CONFLICT:
			cout<<"Conflict Detected with entry : "<<response<<".\n";
			break;
		case NOEVENTEXIST:
			cout<<"Event does not exist.\n";
			break;
		case INVALIDARGCOUNT:
			cout<<"Invalid number of arguments.\n";
			break;
		case WRONGDATE:
			cout<<"Invalid date-time.\n";
			break;
		case EVENTTYPE:
			cout<<"Requested event name is : "<<response<<"\n";
			break;
		case DAYEVENTLIST:
			cout<<"Event(s) of the day are : \n"<<response<<"\n";
			break;
		case GETALLCOUNT:
			{
				int entrycount = atoi(response.c_str());
				cout<<"Number of events are : "<<entrycount<<"\n";
				if(entrycount > 0)
					GetallEntry( sock_desc , entrycount);
				break;
			}
		case UNAUTHORIZED:
			cout<<"The server is not configured to perform GETALL function. Please try again later!!!\n";
			break;
		case INVALIDOP:
			cout<<"Invalid operation.\n";
			break;
		case SERVERERROR:
			cout<<"Server failure\n";
			break;
		default:
			cout<<"Default response : "<<response;
			break;
			
	}
	close(sock_desc);
	return (0);
}


//Special codes for getall function
/* ******------ */
	
void GetallEntry( int sockfd , int entrycount)
{
	char buffer[BUFSIZE];
	cout<<"The events are : "<<entrycount<<" : \n";
	cout<<"\tDate\t\tStart time\tEnd time\tEvent Name\n";
	for ( int i = 1; i <= entrycount; i++)
	{
		sleep(2);
		string request = "anything\tnextentry\t" + Inttostr(i) + "\t";	//for uniformity in "getoperation()" '\t' is added in the beginning
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

/* ------****** */


