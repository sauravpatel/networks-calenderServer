#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <fstream>
#include <string.h> //include if datetime.h not included
#include "macros.h"

#ifndef USERDATA
#define USERDATA "../users/"
#endif

using namespace std;

string add( string username, string date, string startime, string endtime, string eventname );
string Remove( string username, string date, string startime );
string update( string username, string date, string startime, string endtime, string eventname );
string get( string username, string date, string startime );
string getall( string username, int smode );
string NextEntry( string username, int seqno );
void SyncCalender();
bool IsFileEmpty(string filename);
bool IsEntryValid(string line);	//1 valid, 0 invalid
