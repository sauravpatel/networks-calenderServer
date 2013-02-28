
#define USERDATA "../users/"

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
#define REPEATEDEVENT			31

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
