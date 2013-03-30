#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <fstream>
#include <string.h> //include if datetime.h not included

#ifndef USERDATA
#define USERDATA "../users/"
#endif

//error codes

#define SUCCESS					20
#define CONFLICT				21
#define NOEVENTEXIST			22
#define INVALIDARGCOUNT			23
#define WRONGDATE				24
#define EVENTTYPE				25
#define DAYEVENTLIST			26
#define GETALLCOUNT				27
#define UNAUTHORIZED			28
#define INVALIDOP				29
#define SERVERERROR				30

#define USERNAME	0
#define OPERATION	1
#define DATE		2
#define SEQNO		2
#define STARTIME	3
#define ENDTIME		4
#define EVENTNAME	5

#define BUFSIZE		2560		//size of sent/received message buffers

/* operations */
#define ADD			11
#define REMOVE		12
#define UPDATE		13
#define GET			14
#define GETALL		15
#define NEXTENTRY	16
#define INVALID		10

using namespace std;

string ctos(char c);
int getoperation(string operation);
string handleClientData( int cfd, char buffer[BUFSIZE], int smode, string (&authenticate)[1024]);
string add( string username, string date, string startime, string endtime, string eventname );
string Remove( string username, string date, string startime );
string update( string username, string date, string startime, string endtime, string eventname );
string get( string username, string date, string startime );
string getall( string username, int smode );
string NextEntry( string username, int seqno );
void SyncCalender();
bool IsFileEmpty(string filename);
bool IsEntryValid(string line);	//1 valid, 0 invalid
