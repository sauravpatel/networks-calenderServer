#include "calender.h"
#include "datetime.h"
#include <vector>
#include <map>
#include <dirent.h>

using namespace std;

/* 'clientfd' and 'authenticate' is used only for the 'nextentry' function call
 * 'authenticate[clientfd]' contains username for connection established on socket descriptor 'clientfd'
 */
string handleClientData( int clientfd , char buffer[BUFSIZE] , int smode, string (&authenticate)[1024])
{
	/* MESSAGE PARSING
	 * Client sends a request in the form of 
	 * line containing words seperated by tabs in the format:
	 * username + operation + date + startime + (endtime) + (eventname)
	 */
	
	map<string,int> operationCodes;
	operationCodes["add"		]	= ADD;
	operationCodes["remove"		]	= REMOVE;
	operationCodes["update"		]	= UPDATE;
	operationCodes["get"		]	= GET;
	operationCodes["getall"		]	= GETALL;
	operationCodes["nextentry"	]	= NEXTENTRY;
	
	vector<string> parameters;
	stringstream ss;
	ss << buffer;
	string query = ss.str();
	//map<string,int> operationCodes;

	unsigned tabPos;
	while ( true )
	{
		tabPos = query.find("\t");
		parameters.push_back(query.substr(0,tabPos));
		//cout<<query.substr(0,tabPos)<<"\n";
		query = query.substr(tabPos);
		if(query.compare("\t") != 0)
			query = query.substr(1);
		else
			break;
	}
	string operation = string(parameters.at(OPERATION));
	if(smode && authenticate[clientfd].empty() )
		authenticate[clientfd] = string(parameters.at(USERNAME)) ;	//Authorize user
	string opstatus = Inttostr ( INVALIDARGCOUNT );	//status after operation is executed
	switch(operationCodes[operation])
	{
		case ADD:
			if ( parameters.size() == 6 )
				opstatus = add( string(parameters[USERNAME]), string(parameters[DATE]), string(parameters[STARTIME]), string(parameters[ENDTIME]), string(parameters[EVENTNAME]) );
				break;
		case REMOVE:
			if ( parameters.size() == 4 )
				opstatus = Remove( string(parameters[USERNAME]), string(parameters[DATE]), string(parameters[STARTIME]));
				break;
		case UPDATE:
			if ( parameters.size() == 6 )
				opstatus = update( string(parameters[USERNAME]), string(parameters[DATE]), string(parameters[STARTIME]), string(parameters[ENDTIME]), string(parameters[EVENTNAME]));
				break;
		case GET:
			if ( parameters.size() == 3 )
				parameters.push_back( "" );
			if ( parameters.size() == 4 )
				opstatus = get( string(parameters[USERNAME]), string(parameters[DATE]), string(parameters[STARTIME]) );
			break;
		case GETALL:
			if ( parameters.size() == 2 )
				opstatus = getall( string(parameters[USERNAME]), smode);
			break;
		case NEXTENTRY:
			if ( parameters.size() == 3 )
				opstatus = NextEntry( authenticate[clientfd], atoi(string(parameters[SEQNO]).c_str()));
			break;
		default:
			opstatus = Inttostr( INVALIDOP ) ;
	}
	return opstatus;
}
/* -------------------------------------------------------------------------------- */

/* detect conflicting entry in the file (filename) having 
 * ('date', 'startime' and 'endtime') same as parameters passed to the function
 * returns 0 if no conflict else the conflicting entry number ( sequence number of that entry in file )
 */
int ConflictEntry(string filename, string date , string startime, string endtime)
{
	int entryno = 0;
	int retvalue = 0;
	string line="";
	fstream myfile (filename.c_str());
	if (myfile.is_open())
	{
		while ( !myfile.eof() )
		{
			getline ( myfile , line );
			if(line.compare("") == 0 || line.find(" ") == 0)
				continue;
			else
				entryno++;
			string currdate = line.substr(0,line.find("\t"));
			if (date == currdate)
			{
				line = line.substr(line.find("\t")+1);
				string currStartTime = line.substr(0,line.find("\t"));
				if (endtime > currStartTime )
				{
					line = line.substr(line.find("\t")+1);
					string currEndTime = line.substr(0,line.find("\t"));
					if(startime < currEndTime )
					{
						retvalue = entryno;
						break;
					}
				}
			}
		}
		myfile.close();
	}
	return retvalue;
}
/* --------------------------------------------------- */

/*isempty file */
bool IsFileEmpty(string filename)
{
	bool result = true;
	string line;
	fstream myfile (filename.c_str());
	if (myfile.is_open())
	{
		while ( !myfile.eof() )
		{
			getline ( myfile , line );
			if(line.find(" ") != 0 )
			{
				result = false;
				break;
			}
		}
	}
	return result;
}
/* --------------------------------------------------- */

/* ADD operation */
string add( string username, string date, string startime, string endtime, string eventname  )
{
	string status = "";
	if(!CheckDateTime(date, startime, endtime))
	{
		status = Inttostr(WRONGDATE);	//invalid date or time
	}
	else
	{
		/* File Handling */
		string line;
		string filename = USERDATA + username;
		int entryno = ConflictEntry(filename, date, startime, endtime);
		if ( entryno )
		{
			string conflictentry = NextEntry(username,entryno);
			status = Inttostr(CONFLICT) + " " + conflictentry;	//conflict detected with entry : " + line
		}
		else
		{
			// No conflict detected. Good to go...
			fstream myfile;
			myfile.open (filename.c_str() , ios::out | ios::app );
			if ( myfile.is_open() )
			{
				string str = date + "\t" + startime + "\t" + endtime + "\t" + eventname + "\n";
				myfile << str.c_str();
				myfile.close();
			}
			else
			{
				status = Inttostr(SERVERERROR);	//internal server problem encountered.....Please try again later\n";
			}
			status = Inttostr(SUCCESS);
		}
	}
	SyncCalender();
	return status;
}

/* REMOVE operation 
 * replace the line to be removed by sequence of spaces.
 */ 
string Remove( string username, string date, string startime )
{
	string status = "";
	if(!CheckDateTime(date, startime))
	{
		status = Inttostr(WRONGDATE);	//invalid date or time
		return status;
	}
	else
	{
		/* File Handling */
		string line;
		string filename = USERDATA + username;
		fstream myfile (filename.c_str());
		if (myfile.is_open())
		{
			while ( !myfile.eof() )
			{
				int currfptr = myfile.tellg();
				getline ( myfile , line );
				if(line.compare("") == 0 || line.find(" ") == 0)
				{
					continue;
				}
				string currdate = line.substr(0,line.find("\t"));
				if (date == currdate)
				{		
					line = line.substr(line.find("\t")+1);
					string currstartime = line.substr(0,line.find("\t"));
					if (startime == currstartime )
					{
						myfile.seekp(currfptr);
						line.replace(line.begin(),line.end(),line.length(),' ');
						myfile<<line.c_str();
						status = Inttostr(SUCCESS);	//Event deleted successfully.
						break;
					}
				}
			}// out of while
			myfile.close();
		}
	}
	if( status.compare("") == 0)
		status = Inttostr(NOEVENTEXIST);	//Your profile doesn't exist.
	
	SyncCalender();	//remove eventless file
	return status;
}
/* ------------------------------------------------------------------------ */

/* UPDATE operation */
string update( string username, string date, string startime, string endtime, string eventname )
{
	string status = "";
	if(!CheckDateTime(date, startime, endtime))
	{
		status = Inttostr(WRONGDATE);	//invalid date or time
	}
	else
	{
		/* File Handling */
		string line, oldentry;
		oldentry = "";
		int flag = 0;
		int match;
		string filename = USERDATA + username;
		fstream myfile (filename.c_str() );
		if (myfile.is_open())
		{
			while ( !myfile.eof() && !flag )	//finding the event to be updated
			{
				int currfptr = myfile.tellg();
				getline ( myfile , line );
				oldentry = line;
				if(line.compare("") == 0 || line.find(" ") == 0)
					continue;
				string currdate = line.substr(0,line.find("\t"));
				if (date == currdate)
				{
					string str;
					str.insert( 0 ,line.length(),' ');
					line = line.substr(line.find("\t")+1);
					string currstartime = line.substr(0,line.find("\t"));
					if (startime == currstartime )	//found the entry to be changed
					{
						flag = 1;
						string updateentry = date + "\t" + startime + "\t" + endtime + "\t" + eventname;
						if(updateentry.compare(oldentry) == 0 )
						{
							flag = 0;
							status = Inttostr(REPEATEDEVENT);	// same event already exists
							break;
						}
						match = currfptr;
						myfile.seekp(match);
						myfile<<str.c_str();	//remove old entry
					}
				}
			}
			myfile.close();

			if(flag == 1)
			{					
				//check for conflict occuring due to update
				int entryno = ConflictEntry(filename, date, startime, endtime);
				if( entryno )	//conflict occurs, revert back to old entry
				{
					status = NextEntry(username, entryno);	//get the conflicting entry
					status = Inttostr(CONFLICT) + " " +status;	//conflict detected with entry : " + line
					myfile.open (filename.c_str());
					if ( myfile.is_open() )
					{
						myfile.seekp(match);
						string str = oldentry + "\n";
						myfile << str.c_str();	//replace with oldentry
						myfile.close();
					}
				}
				else
				{
					//no conflict, add new event in the calender
					fstream myfile;
					myfile.open (filename.c_str() , ios::out | ios::app );
					if ( myfile.is_open() )
					{
						string str = date + "\t" + startime + "\t" + endtime + "\t" + eventname + "\n";
						myfile << str.c_str();
						myfile.close();
						status = Inttostr(SUCCESS);
					}
				}
			}
		}
	}
	if( status.compare("") == 0)
		status = Inttostr(NOEVENTEXIST);	//Your profile doesn't exist.

	SyncCalender();
	return status;
}

/* ------------------------------------------------------------------------ */
/* GET operation */
string get( string username, string date, string startime )
{
	/* return format: "status_code" + "space" + "event_name/list of entries" */
	string status;
	if(!CheckDateTime(date, startime))
	{
		status = Inttostr(WRONGDATE);	//invalid date or time
	}
	/* File Handling */
	status = "";
	string line;
	string filename = USERDATA + username;
	fstream myfile (filename.c_str() , ios::in );
	if (myfile.is_open())
	{
		while ( !myfile.eof() )
		{
			getline ( myfile , line );
			if(line.compare("") == 0 || line.find(" ") == 0 )
				continue;
			
			unsigned found = line.find("\t");
			string currdate = line.substr (0, found);
			if (date == currdate)
			{
				if( status.compare("") == 0)
					status = Inttostr(DAYEVENTLIST) + " ";
				string remainline = line.substr(found+1);
				status += remainline + "\n";
				if (startime.compare("") != 0)
				{
					status = Inttostr(NOEVENTEXIST);	//The requested event does not exist.
					found = remainline.find("\t");	//all event of the day
					string curtime = remainline.substr(0,found);
					if(startime == curtime )
					{
						unsigned namepos = remainline.find_first_of("\t",found+1);
						status = Inttostr(EVENTTYPE) + " " + remainline.substr(namepos+1) ;	//event name only
						break;
					}
				}
			}
		}
		myfile.close();
	}
	if( status.compare("") == 0)
		status = Inttostr(NOEVENTEXIST);	//Your profile doesn't exist.
	
	SyncCalender();
	return status;
}
/* ------------------------------------------------------------------------ */

/* GETALL operation */
string getall( string username, int smode )
{
	/* return format: "status_code" + "space" + "entrycount" + "tab" + "list of entries" */
	
	string status = "";
	
	if(smode != 1)
	{
		status = Inttostr(UNAUTHORIZED);	//The server is not configured to perform GETALL function. Please try again later!!!\n
	}
	else
	{
		string line;
		int entrycount = 0;
		string filename = USERDATA + username;
		fstream myfile (filename.c_str() , ios::in );
		if (myfile.is_open())
		{
			while ( !myfile.eof() )
			{
				getline ( myfile , line );
				if(line.compare("") == 0 || line.find(" ") == 0)
					continue;
				else
				{
					entrycount++;
				}
			}
			myfile.close();
		}
		status = Inttostr(GETALLCOUNT) + " " + Inttostr(entrycount);
	}
	SyncCalender();
	return status;

}
/* ------------------------------------------------------------------------ */
/* NEXTENTRY
 * Here we have a authentication type system. We have a array of string "authenticate" which stores username corresponding
 * to a socket descriptor and the entry is removed when connection is closed. "nextentry()" is used to fetch jth entry 
 * in the user database. "nextentry()" takes two parameter a)username supplied from authenticate array and b) entry no.
 * message format of client : "nextentry" + "space" + "entry no."
 * So using socket descriptor (i) we can only access the database of user who opened it.
 * It returns the event details correspoonding to the seq/entry number in the file 
 */ 
string NextEntry ( string username, int seqno )
{
	
	string status = "Invalid line number.\n";
	string line;
	int currindex = 1;
	string filename = USERDATA + username;
	fstream myfile (filename.c_str() , ios::in );
	if (myfile.is_open())
	{
		while ( !myfile.eof() )
		{
			getline ( myfile , line );
			//  !last line				!empty line
			if(line.compare("") == 0 || line.find(" ") == 0)
				continue;
			if ( seqno == currindex )
			{
				status  = line;
				break;
			}
			currindex++;
		}
		myfile.close();
	}
	return status;
}
/* ------------------------------------------------- */

/* 1 valid, 0 invalid 
 * Checks if event has passed its endtime
 * string 'line' = 'date'\t'startime'\t'endtime'
 */
bool IsEntryValid(string line)
{
	bool valid = false;
	time_t t = time(0);   // get time now
    struct tm * now = localtime( & t );
    string date = line.substr(0,line.find("\t"));
    string endtime = line.substr(line.find_last_of("\t")+1);
	if( (now->tm_year - 100) < atoi(date.substr(4,2).c_str()) )
		valid=true;
	else if( (now->tm_mon + 1) < atoi(date.substr(0,2).c_str()) )
		valid=true;
	else if( (now->tm_mday) < atoi(date.substr(2,2).c_str()) )
		valid=true;
	else if( (now->tm_hour) < atoi(endtime.substr(0,2).c_str()) )
		valid=true;
	else if( (now->tm_min) < atoi(endtime.substr(2,2).c_str()) )
		valid=true;
	return valid;
}
/* ------------------------------------------------------------------------------ */

/* remove all invalid/completed events */
void SyncCalender()
{
	DIR *pDIR;
	struct dirent *entry;
	if( (pDIR=opendir(USERDATA)) )
	{
		while( (entry = readdir(pDIR)) )
		{
			if( strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 )
				continue;
			stringstream ss;
			ss << entry->d_name;
			string filename = USERDATA + ss.str();
			string line;
			fstream myfile (filename.c_str() );
			if (myfile.is_open())
			{
				while ( !myfile.eof() )
				{
					int currfptr = myfile.tellg();
					getline ( myfile , line );
					cout<<line<<"\n";
					if(line.compare("") == 0 || line.find(" ") == 0)
						continue;
					if( !IsEntryValid(line.substr(0,line.find_last_of("\t"))) )	//remove invalid entry
					{
						cout<<"Found invalid:"<<line<<":\n";
						myfile.seekp(currfptr);
						line.replace(line.begin(),line.end(),line.length(),' ');
						myfile<<line.c_str();
					}
				}
				myfile.close();
			}
			if(IsFileEmpty(filename))
			{
				cout<<"removing "<<filename;
				remove(filename.c_str());	//remove eventless file
			}
		}
		closedir(pDIR);
	}
}
