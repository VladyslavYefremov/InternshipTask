#include <Windows.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <psapi.h>

#define USE_STRING_DESPITE_TCHAR

/*
	update.txt
*/

#include "Process.h"
#include "functions.h"
#include "properties.h"

#define ENABLE_LOGGING
#include "logger.h"

using namespace std;

void OnProcessExited(Process const*);
void OnProcessStarted(Process const*);
void OnProcessRestart(Process const*);
void OnProcessStopped(Process const*);
void OnProcessResumed(Process const*);
void OpenProcessById();

#ifdef USE_STRING_DESPITE_TCHAR
void printUpdate(string);
#else
void printUpdate(TCHAR *);
#endif

CRITICAL_SECTION g_coutAccess;
Process * gCurrentProcess;

/*		Scanner type
*
*	read_action		- reading menu item
*	read_id			- reading new process id
*	system_pause	- waiting for key pressed
*/

enum ScanerType {
	read_action,
	read_id,
	system_pause
};

ScanerType gScaner = read_action;

void printUpdate(TCHAR *);

INT main(INT argc, TCHAR ** argv)
{
	InitializeCriticalSection(&g_coutAccess);

	LOG("Program started!");

	// Creation of new process;
	// default_process: command line of a process
	gCurrentProcess = new Process(default_process);

	if (gCurrentProcess->getHandle() != NULL)
	{
		// Register event - process started
		if (!gCurrentProcess->RegisterStartedCallback(OnProcessStarted)){
			LOG_ERR("failed to register 'started' callback! [", ERROR_REGISTER_CALLBACK_STARTED, "]");
			return ERROR_REGISTER_CALLBACK_STARTED;
		}

		// Register event - process restart (pre)
		if (!gCurrentProcess->RegisterRestartCallback(OnProcessRestart)){
			LOG_ERR("failed to register 'restart' callback! [", ERROR_REGISTER_CALLBACK_RESTART, "]");
			return ERROR_REGISTER_CALLBACK_RESTART;
		}

		// Register event - process stopped
		if (!gCurrentProcess->RegisterStoppedCallback(OnProcessStopped)){
			LOG_ERR("failed to register 'stopped' callback! [", ERROR_REGISTER_CALLBACK_STOPPED, "]");
			return ERROR_REGISTER_CALLBACK_STOPPED;
		}

		// Register event - process resumed
		if (!gCurrentProcess->RegisterResumedCallback(OnProcessResumed)){
			LOG_ERR("failed to register 'resumed' callback! [", ERROR_REGISTER_CALLBACK_RESUMED, "]");
			return ERROR_REGISTER_CALLBACK_RESUMED;
		}

		// Register event - process exited
		if (!gCurrentProcess->RegisterExitCallback(OnProcessExited)){
			LOG_ERR("failed to register 'exit' callback! [", ERROR_REGISTER_CALLBACK_EXIT, "]");
			return ERROR_REGISTER_CALLBACK_EXIT;
		}
	}
	else{
		LOG_ERR("failed to create new process by the command line!");
	}

	BOOL bMenu = TRUE;

	// Create Menu
	while (bMenu)
	{
		gScaner = read_action;
#ifdef USE_STRING_DESPITE_TCHAR
		printUpdate(string(""));
#else
		printUpdate(NULL);
#endif

		/*
		*	Get menu item
		*	"Action" - reading word
		*	"[0-9]+" - regular expression (positive numbers only)
		*/

#ifdef USE_STRING_DESPITE_TCHAR
		DWORD iAction = stoi(incoming_data("Action", "[0-9]+"));
#else
		char * pAction = incoming_data("Action", "[0-9]+");
		DWORD iAction = atoi(pAction != NULL ? pAction : "0");

		if (pAction != NULL)
			delete[] pAction;
#endif

		switch (iAction) {
		case 1:
			//Stop the process
			gCurrentProcess->Stop();
			break;

		case 2:
			//Resume the process
			gCurrentProcess->Resume();
			break;

		case 3:
			//Restart the process
			gCurrentProcess->Restart();
			break;

		case 4:
			/*
			*	Displaying process command line
			*
			*	getCommandLine() - returns the process cmd.line
			*	gScaner = system_pause - wait for key pressed (in case menu updated)
			*/
			system("CLS");
			cout << gCurrentProcess << endl;
			cout << "Command Line of this process:" << endl << gCurrentProcess->getCommandLine() << endl << endl;
			gScaner = system_pause;
			system("pause");
			break;

		case 5:
			// Read id & open process
			OpenProcessById();
			break;

		case 6:
			//Destroy the process
			gCurrentProcess->DestroyProcess();
			break;

		default:
			//Exit the menu
			bMenu = FALSE;

		}
	}

	delete gCurrentProcess;

	LOG("Program finished!");
	return 0;
}


#ifdef USE_STRING_DESPITE_TCHAR
void OpenProcessById()
{
	/*
	*	gScaner = read_id - means that we are waiting for inputting of process id
	*	szHelpingInfo - informaton wich prints in case an error of reading data
	*/

	INT _id;
	string sHelpingInfo;

	do {
		_id = 0;
		gScaner = read_id;

		printUpdate(sHelpingInfo);

		if (sHelpingInfo.length())
			sHelpingInfo.clear();

		_id = stoi(incoming_data("Input a process name (-1 to go back)", "-?[0-9]+"));

		// can't open a process by id
		if (_id > 0 && !gCurrentProcess->Create(_id)){
			LOG_DEBUG("Couldn't open a process [ id: ", _id, " ]");

			// To display information about fault
			ostringstream stringStream;
			stringStream << "Couldn't open a process [ id: " << _id << " ]";
			sHelpingInfo = stringStream.str();

			_id = 0;
		}
	} while (_id < 1 && _id != -1);
}
#else
void OpenProcessById()
{
	/*
	*	gScaner = read_id - means that we are waiting for inputting of process id
	*	szHelpingInfo - informaton wich prints in case an error of reading data
	*	pIncomingData - takes pointer which is returned by incoming_data
	*/

	TCHAR * pIncomingData;
	pIncomingData = NULL;

	INT _id;
	TCHAR * szHelpingInfo;
	szHelpingInfo = NULL;

	do {
		_id = 0;
		gScaner = read_id;

		printUpdate(szHelpingInfo);

		// Cleaning
		if (szHelpingInfo != NULL) {
			delete[] szHelpingInfo;
			szHelpingInfo = NULL;
		}

		/* 
		*	incoming_data returning the pointer to string which we've read before
		*
		*		"Input a process name (-1 to go back)" - reading phrase
		*		"-?[0-9]+" - regular expression(numbers only)
		*/
		pIncomingData = incoming_data("Input a process name (-1 to go back)", "-?[0-9]+");

		// have read a data
		if (pIncomingData != NULL) {
			_id = atoi(pIncomingData);

			// can't open a process by id
			if (_id > 0 && !gCurrentProcess->Create(_id)){
				LOG_DEBUG("Couldn't open a process [ id: ", _id, " ]");

				// To display information about fault
				szHelpingInfo = new TCHAR[MAX_PATH];
				sprintf_s(szHelpingInfo, MAX_PATH - 1, "Couldn't open a process [ id: %d ]", _id);

				_id = 0;
			}
			delete[] pIncomingData;
		}
	} while (_id < 1 && _id != -1);

	if (szHelpingInfo != NULL) {
		delete[] szHelpingInfo;
	}
}
#endif

#ifdef USE_STRING_DESPITE_TCHAR
void printUpdate(string helpingString){
#else
void printUpdate(TCHAR * helpingString){
#endif
	system("CLS");

	cout << gCurrentProcess << endl << endl;

	cout << "1. Stop the process" << endl;
	cout << "2. Resume the process" << endl;
	cout << "3. Restart the process" << endl;
	cout << "4. Get process Command Line" << endl;
	cout << "5. Watch another process" << endl;
	cout << "6. Exit the process" << endl << endl;
	cout << "0. Exit Program" << endl << endl;

	// If we want to display helping information
#ifdef USE_STRING_DESPITE_TCHAR
	if (helpingString.length())
#else
	if (helpingString != NULL)
#endif
		cout << helpingString << endl << endl;

	cout << (gScaner == read_action ? "Action: " : (gScaner == read_id ? "Input a process name (-1 to go back): " : "Press any key to continue..."));
}

//	function is called when the process exit
void OnProcessExited(Process const* process)
{
	EnterCriticalSection(&g_coutAccess);
	LOG("Process [ id: ", process->getId(), " ] exited!");
	LeaveCriticalSection(&g_coutAccess);
}

//	function is called when the process start
void OnProcessStarted(Process const* process)
{
	EnterCriticalSection(&g_coutAccess);

#ifdef USE_STRING_DESPITE_TCHAR
	printUpdate(string(""));
#else
	printUpdate(NULL);
#endif
	LOG("Process [ id: ", process->getId(), " | name: ", getStringByPointer(process->getProcessName()), "] started!");

	LeaveCriticalSection(&g_coutAccess);
}

//	function is called when the process restart
void OnProcessRestart(Process const* process)
{
	EnterCriticalSection(&g_coutAccess);

#ifdef USE_STRING_DESPITE_TCHAR
	printUpdate(string(""));
#else
	printUpdate(NULL);
#endif
	LOG("process [ id: ", process->getId(), " | name: ", getStringByPointer(process->getProcessName()), "] is restarting!");

	LeaveCriticalSection(&g_coutAccess);
}

//	function is called when the process is stopped
void OnProcessStopped(Process const* process)
{
	EnterCriticalSection(&g_coutAccess);
	LOG("Process [ id: ", process->getId(), " | name: ", getStringByPointer(process->getProcessName()), "] has been suspended!");
	LeaveCriticalSection(&g_coutAccess);
}

//	function is called when the process is resumed
void OnProcessResumed(Process const* process)
{
	EnterCriticalSection(&g_coutAccess);
	LOG("Process [ id: ", process->getId(), " | name: ", getStringByPointer(process->getProcessName()), "] has been resumed!");
	LeaveCriticalSection(&g_coutAccess);
}