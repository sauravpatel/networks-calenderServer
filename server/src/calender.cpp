#include "calender.h"
#include "datetime.h"
#include <dirent.h>

using namespace std;

/* detect conflicting entry in the file (filename) having 
 * ( 'date', 'startime' and 'endtime' ) same as parameters passed to the function
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
				string currendtime = line.substr(0,line.find("\t"));
				if (endtime > currendtime)
				{
					line = line.substr(line.find("\t")+1);
					string currstartime = line.substr(0,line.find("\t"));
					if(startime < currstartime)
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
			if(line.find(" ") == 0 )
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
	cout<<"Adding\n";
	if(!CheckDateTime(date, startime, endtime))
	{
		status = Inttostr(WRONGDATE);	//invalid date or time
	}
	else if ( eventname.compare("") == 0 )
	{
		status = Inttostr(INVALIDARGCOUNT);	//some paramter(s) missing for ADD operation.
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
	if ( startime.compare("") == 0 )
	{
		status = Inttostr(INVALIDARGCOUNT);	//some paramter(s) missing for REMOVE operation."
		return status;
	}
	else if(!CheckDateTime(date, startime))
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
		status = Inttostr(NOEVENTEXIST);	//No such events exist
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
	SyncCalender();	//remove eventless file
	return status;
}

/* ------------------------------------------------------------------------ */
/* UPDATE operation 
 * conflict check on update not enabled
 */
string update( string username, string date, string startime, string endtime, string eventname )
{
	string status = "";
	if(!CheckDateTime(date, startime, endtime))
	{
		status = Inttostr(WRONGDATE);	//invalid date or time
		return status;
	}
	else if ( eventname.compare("") == 0 )
	{
		status = Inttostr(INVALIDARGCOUNT);	//some paramter(s) missing for UPDATE operation.
		return status;
	}
	else
	{
		/* File Handling */
		status = Inttostr(NOEVENTEXIST);	//Your profile doesn't exist
		string line, oldentry;
		oldentry = "";
		int flag = 0;
		int match;
		string filename = USERDATA + username;
		fstream myfile (filename.c_str() );
		if (myfile.is_open())
		{
			while ( !myfile.eof() || !flag )	//finding the event to be updated
			{
				int currfptr = myfile.tellg();
				getline ( myfile , line );
				oldentry = line;
				if(line.compare("") == 0)
					break;
				string currdate = line.substr(0,line.find("\t"));
				if (date == currdate)
				{
					line = line.substr(line.find("\t")+1);
					string currstartime = line.substr(0,line.find("\t"));
					if (startime == currstartime )	//found the entry to be changed
					{
						flag = 1;
						string updateentry = date + "\t" + startime + "\t" + endtime + "\t" + eventname;
						if(updateentry.compare(oldentry) == 0 )
						{
							flag = 0;
							status = Inttostr(SUCCESS);	//nothing to change ( new is old and old is new )
							break;
						}
						match = currfptr;
						myfile.seekp(match);
						line.replace(line.begin(),line.end(),line.length(),' ');
						myfile<<line.c_str();	//remove old entry
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
					line = date + "\t" + startime + "\t" + endtime + "\t" + eventname + "\n";
					myfile.open (filename.c_str() );
					if ( myfile.is_open() )
					{
						myfile.seekp(match);
						myfile<<line.c_str();
						myfile.close();
						status = Inttostr(SUCCESS);	//Event updated successfully.
					}
				}
			}
		}
	}
	SyncCalender();
	return status;
}

/* ------------------------------------------------------------------------ */
/* GET operation */
string get( string username, string date, string startime )
{
	/* return format: "status_code" + "space" + "event_name/list of entries" */

	string status;
	
	if(startime.compare("") != 0)
	{
		if(!CheckDateTime(date, startime))
		{
			status = Inttostr(WRONGDATE);	//invalid date or time
			return status;
		}
	}
	else if(!CheckDateTime(date ))
	{
		status = Inttostr(WRONGDATE);	//invalid date or time
		return status;
	}
	/* File Handling */
	status = Inttostr(DAYEVENTLIST) + " ";
	string line;
	string filename = USERDATA + username;
	fstream myfile (filename.c_str() , ios::in );
	if (myfile.is_open())
	{
		while ( !myfile.eof() )
		{
			getline ( myfile , line );
			if(line.compare("") == 0)
				break;
			
			unsigned found = line.find("\t");
			string currdate = line.substr (0, found);
			if (date == currdate)
			{
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
	else
	{
		status = Inttostr(NOEVENTEXIST);	//Your profile doesn't exist.
	}
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

/* It returns the event details correspoonding to the seq/entry number in the file
 */ 
string NextEntry ( string username, int seqno )
{
	string status = "";
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
			else
			{
				if ( seqno == currindex )
					status  = line;
				currindex++;
			}
		}
		myfile.close();
	}
	return status;
}

/* ------------------------------------------------- */

//remove all dead entries
void SyncCalender()
{
	DIR *pDIR;
	struct dirent *entry;
	if( (pDIR=opendir(USERDATA)) )
	{
		while( (entry = readdir(pDIR)) )
		{
			if( strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 )
				cout<<entry->d_name<<" ";
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
					if(line.compare("") == 0 || line.find(" ") == 0)
						continue;
					if( !IsEntryValid(line.substr(0,line.find_last_of("\t"))) )	//remove invalid entry
					{
						myfile.seekp(currfptr);
						line.replace(line.begin(),line.end(),line.length(),' ');
						myfile<<line.c_str();
					}
				}
				myfile.close();
			}
			if(!IsFileEmpty(filename))
				remove(filename.c_str());	//remove eventless file
		}
		closedir(pDIR);
	}
}

//1 valid, 0 invalid
// this function is working properly
bool IsEntryValid(string line)
{
	bool valid = false;
	time_t t = time(0);   // get time now
    struct tm * now = localtime( & t );
    /*cout << (now->tm_year - 100) << '-' 
         << (now->tm_mon + 1) << '-'
         << now->tm_mday
         << endl
         << now->tm_hour << ":"
         << now->tm_min << ":"
         << now->tm_sec
         <<endl;*/
    string date = line.substr(0,line.find_last_of("\t"));
    string time = line.substr(line.find_last_of("\t")+1);
	if( (now->tm_year - 100) < atoi(date.substr(4,2).c_str()) )
		valid=true;
	else if( (now->tm_mon + 1) < atoi(date.substr(0,2).c_str()) )
		valid=true;
	else if( (now->tm_mday) < atoi(date.substr(2,2).c_str()) )
		valid=true;
	else if( (now->tm_hour) < atoi(time.substr(0,2).c_str()) )
		valid=true;
	else if( (now->tm_min) < atoi(time.substr(2,2).c_str()) )
		valid=true;
	return valid;
}