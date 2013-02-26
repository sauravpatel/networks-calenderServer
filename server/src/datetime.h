#include <string.h>
#include <time.h>
#include <sstream>


using namespace std;

string Inttostr(int num);
bool isleapyear(int year);
bool validdate(int year, int month, int day);
bool validtime(int hour,int min);
bool CheckDateTime(string date, string start="0000", string end="2400" );

/* Integer to string conversion */
string Inttostr(int num)
{
	stringstream ss;
	ss << num;
	return ss.str();
}

// validate date and time
bool CheckDateTime(string date, string start, string end)
{
	cout<<"Checking date time.\n";
	bool result = true;
	int mm,dd,yyyy,hh[2],min[2];	
	if((date.length() != 6) || (start.length() != 4 ) || ( end.length() != 4 ) )
	{
		cout<<date.length();
		cout<<start.length();
		cout<<end.length();
		//cout<<"Invalid Date or time format.\n";
		result=false;
	}
	else
	{
		mm = atoi(date.substr(0,2).c_str());
		dd = atoi(date.substr(2,2).c_str());
		yyyy = atoi(("20" + date.substr(4,2)).c_str());	//convert from 'yy' to 'yyyy'
		hh[1] = atoi(start.substr(0,2).c_str());
		min[1] = atoi(start.substr(2,2).c_str());
		hh[2] = atoi(end.substr(0,2).c_str());
		min[2] = atoi(end.substr(2,2).c_str());
		
		//validate date-time format
		if ( !validdate ( yyyy, mm, dd ) || !validtime(hh[1],min[1]) || !validtime(hh[2],min[2]) )
		{
			//cout<<"Invalid Date or time.\n";
			result=false;
		}
		else
		{
			/* check Start time is less than end time. */
			if(start.compare(end) >= 0)
			{
				//cout<<"Start time must be less than end time.\n";
				result=false;
			}
			/* check event date is greater than equal to current date */
			else
			{
				time_t t = time(0);   // get time now
				struct tm * now = localtime( & t );
				/* cout << (now->tm_year + 1900) << '-' 
					 << (now->tm_mon + 1) << '-'
					 << now->tm_mday
					 << endl
					 << now->tm_hour << ":"
					 << now->tm_min << ":"
					 << now->tm_sec
					 <<endl; */
				string currtime = Inttostr(now->tm_hour) + Inttostr(now->tm_min);
				if( (now->tm_year + 1900) > yyyy)
				{
					//cout<<"YEAR ERROR\n";
					result=false;
				}
				else if( (now->tm_year + 1900) == yyyy)
				{
					if( (now->tm_mon + 1) > mm)
					{
						//cout<<"MONTH ERROR\n";
						result=false;
					}
					else if ( (now->tm_mon + 1) == mm )
					{
						if (now->tm_mday > dd )
						{
							//cout<<"DAY ERRROR\n";
							result=false;
						}
						else if ( now->tm_mday == dd )
						{
							if ( start.compare(currtime) <= 0 )
							{
								//cout<<"TIME ERROR\n";
								result=false;
							}
						}
					}
				}
			}
		}
	}
	return result;
}

bool isleapyear(int year)
{
	return (!((year%4) && (year%100)) || !(year%400));
}

//1 valid, 0 invalid
bool validdate(int year,int month,int day)
{
	int monthlen[]={31,28,31,30,31,30,31,31,30,31,30,31};
	if (!year || !month || !day || month>12)
		return 0;
	if (isleapyear(year) && month==2)
		monthlen[1]++;
	if (day>monthlen[month-1])
		return 0;
	return 1;
}

//1 valid, 0 invalid
bool validtime(int hour, int min)
{
	if(min>59 || min<0)
		return 0;
	if((hour==24 && min!=0) || hour >24)
		return 0;
	return 1;
}
