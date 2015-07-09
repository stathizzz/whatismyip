/**
* Win32 service creation guide in http://www.devx.com/cplus/Article/9857/1954
*/
#include <exports.h>
#include <curl/curl.h>
#include <stdlib.h>

#define SLEEP_TIME 300000
#ifdef WIN32
#define HOME "WINDIR"
#define LOGFILE "\\whatismyip.log"
#else
#define HOME "/"
#define LOGFILE "/whatismyip.log"
#endif

SERVICE_STATUS          ServiceStatus; 
SERVICE_STATUS_HANDLE   hStatus; 


void  ServiceMain(int argc, char** argv); 
void  ControlHandler(DWORD request); 
int InitService();
int WriteToLog(char* str);

int WriteToLog(char* str)
{
	FILE* log;
	char *path;
	LPSTR *pa[100];
	GetEnvironmentVariable(HOME, pa,100);
	strncat(pa, LOGFILE, strlen(LOGFILE));
	log = fopen(pa, "a+");
	if (log == NULL)
		return -1;
	fprintf(log, "%s\n", str);
	fclose(log);
	return 0;
}

void ServiceMain(int argc, char** argv) 
{ 
	int error; 
	MEMORYSTATUS memory;
	int result;
	
	CURLcode status ;
	int counter = 0;
	status = (CURLcode)-1;

	ServiceStatus.dwServiceType = SERVICE_WIN32; 
	ServiceStatus.dwCurrentState = SERVICE_START_PENDING; 
	ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	ServiceStatus.dwWin32ExitCode = 0; 
	ServiceStatus.dwServiceSpecificExitCode = 0; 
	ServiceStatus.dwCheckPoint = 0; 
	ServiceStatus.dwWaitHint = 0; 
 
	hStatus = RegisterServiceCtrlHandler("MemoryStatus", (LPHANDLER_FUNCTION)ControlHandler); 
	if (hStatus == (SERVICE_STATUS_HANDLE)0) 
	{ 
		// Registering Control Handler failed
		WriteToLog("RegisterServiceCtrlHandler started.");
		return; 
	}  
	
	// We report the running status to SCM. 
	ServiceStatus.dwCurrentState = SERVICE_RUNNING; 
	SetServiceStatus (hStatus, &ServiceStatus);
	
	WriteToLog("Monitoring started!");
	 
	// The worker loop of a service
	while (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
	{
		int mystatus;
		GlobalMemoryStatus(&memory);
		
		/* whatismyip stuff */
		if (mystatus = get_ip_from_url_in_file("http://checkip.dyndns.com", "ip.log") == CURL_STATUS_FAILURE) 
		{
			result = WriteToLog("Error creating temporary file for upload!");
			goto end;
		};
   
		while(status != CURLE_OK && counter < 3)
		{
			status = ftp_upload("ftp://stathizzz:tsakal1@ftp.drivehq.com/", "ip.log");
			_sleep(100);
			counter++;
		}		
		result = (status != CURLE_OK)?  WriteToLog("Error uploading file!") : WriteToLog("Successfully uploaded file on ftp!");
				
	end:
		if (result)
		{
			ServiceStatus.dwCurrentState = SERVICE_STOPPED; 
			ServiceStatus.dwWin32ExitCode = -1; 
			SetServiceStatus(hStatus, &ServiceStatus);
			return;
		}
		_sleep(SLEEP_TIME);
	}
	return; 
}


void ControlHandler(DWORD request) 
{ 
   switch(request) 
   { 
      case SERVICE_CONTROL_STOP: 
         WriteToLog("Monitoring stopped.");
		 
         ServiceStatus.dwWin32ExitCode = 0; 
         ServiceStatus.dwCurrentState = SERVICE_STOPPED; 
         SetServiceStatus (hStatus, &ServiceStatus);
		 return;  
      case SERVICE_CONTROL_SHUTDOWN: 
         WriteToLog("Monitoring stopped.");

         ServiceStatus.dwWin32ExitCode = 0; 
         ServiceStatus.dwCurrentState = SERVICE_STOPPED; 
         SetServiceStatus (hStatus, &ServiceStatus);
         return; 
        
      default:
         break;
    } 
 
    // Report current status
    SetServiceStatus (hStatus, &ServiceStatus);
 
    return; 
}

void main() 
{
   SERVICE_TABLE_ENTRY ServiceTable[2];
   ServiceTable[0].lpServiceName = "MemoryStatus";
   ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;

   ServiceTable[1].lpServiceName = NULL;
   ServiceTable[1].lpServiceProc = NULL;
   // Start the control dispatcher thread for our service
   StartServiceCtrlDispatcher(ServiceTable);  
}