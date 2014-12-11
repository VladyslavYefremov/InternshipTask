#include "Process.h"

Process::Process() : hProcess(NULL),
					hThread(NULL),
					hWait(NULL),
					exitCallback(NULL),
					stoppedCallback(NULL),
					startCallback(NULL),
					restartCallback(NULL),
					resumedCallback(NULL),
					id(0){}

Process::Process(TCHAR * _szCmdLine) : hProcess(NULL),
					hThread(NULL),
					hWait(NULL),
					exitCallback(NULL),
					stoppedCallback(NULL),
					startCallback(NULL),
					restartCallback(NULL),
					resumedCallback(NULL),
					id(0)
{
	Create(_szCmdLine);
}


Process::~Process()
{
	DestroyProcess();
}

BOOL Process::Create(TCHAR * _szCmdLine)
{

	STARTUPINFOA si = { sizeof(STARTUPINFOA) };
	PROCESS_INFORMATION pi;

	BOOL bProcessCreated = CreateProcess(
		NULL,		// No module name (use command line)
		_szCmdLine,	// Command line
		NULL,		// Process handle not inheritable
		NULL,		// Thread handle not inheritable
		FALSE,		// Set handle inheritance to FALSE
		CREATE_NO_WINDOW,		// Process Creation Flags
		NULL,		// Use parent's environment block
		NULL,		// Use parent's starting directory
		&si,		// Pointer to STARTUPINFO structure
		&pi);		// Pointer to PROCESS_INFORMATION structure

	if (bProcessCreated)
	{
		hProcess = pi.hProcess;	// Save the process handle
		hThread = pi.hThread;	// Save the thread handle	
		id = pi.dwProcessId;	// Save the process id

		// Remember the command line of the process
		szCmdLine = new TCHAR[strlen(_szCmdLine) + 1];
		strcpy_s(szCmdLine, strlen(szCmdLine) + 1, _szCmdLine);

		// Call 'process started'
		OnStarted();
	}

	return bProcessCreated;
}

//	Method starts watching for new process, opened by its id
BOOL Process::Create(DWORD _id)
{
	// local
	HANDLE lhProcess = OpenProcess(PROCESS_ALL_ACCESS, false, _id);

	if (lhProcess != NULL)
	{
		if (hProcess)
			DestroyProcess();

		hProcess = lhProcess;
		hThread = GetThreadByID(_id);

		// Register new 'exit' callback for new process
		RegisterExitCallback(exitCallback);

		// Remember the command line of the process
		szCmdLine = GetCommandLine(hProcess);

		id = _id;	// Save the process id

		// Call 'process started'
		OnStarted();

		return TRUE;
	}

	return FALSE;
}

// Resumes the process
BOOL Process::Resume()
{
	if (ResumeThread(hThread)){
		OnResumed();
		return TRUE;
	}
	return FALSE;
}

// Stops the process
BOOL Process::Stop()
{
	if (SuspendThread(hThread) < 1){
		OnStopped();
		return TRUE;
	}
	return FALSE;
}

// Destroys the process
BOOL Process::DestroyProcess() {
	if (hThread) {
		CloseHandle(hThread);
	}

	// Unregister Wait
	if (hWait)
	{
		// INVALID_HANDLE_VALUE means "Wait for pending callbacks"
		::UnregisterWaitEx(hWait, INVALID_HANDLE_VALUE);
		hWait = NULL;
	}

	if (hProcess)
	{
		//	Call 'process exited'
		//	param. TRUE means - not restart process by its command line
		OnExited(TRUE);

		DWORD code;
		if (GetExitCodeProcess(hProcess, &code))
		{
			// If the process is still alive - terminate it
			if (code == STILL_ACTIVE) {
				TerminateProcess(hProcess, 0);
			}
		}
		CloseHandle(hProcess);
	}

	id = NULL;
	hProcess = NULL;
	hThread = NULL;
	iStatus = PROC_TERMINATED;

	return TRUE;
}

// Restarts the process
BOOL Process::Restart()
{
	if (hProcess == NULL)
		return FALSE;

	// Call 'process is restarting'
	OnRestart();

	// Remember the command line
	DWORD iLen = strlen(szCmdLine), _id = id;
	TCHAR * buffer = new TCHAR[iLen + 1];
	strcpy_s(buffer, iLen + 1, szCmdLine);

	// Destroy current process
	DestroyProcess();

	// If we have the command line - create new process
	if (iLen){
		// Create new process (using command line)
		Create(buffer);
		// Re-registration of 'exit' callback
		RegisterExitCallback(exitCallback);
		return TRUE;
	}

	delete[] buffer;

	return FALSE;
}

// Registers 'exit' callback
BOOL Process::RegisterExitCallback(ProcessCallback callback)
{
	if (!callback) return FALSE;

	exitCallback = callback;

	// Directs a wait thread in the thread pool to wait on the object.
	return RegisterWaitForSingleObject(&hWait, hProcess, OnExited, this, INFINITE, WT_EXECUTEONLYONCE);
}

// Registers 'started' callback
BOOL Process::RegisterStartedCallback(ProcessCallback callback)
{
	if (!callback)
		return FALSE;
	if (startCallback)
		return FALSE;

	startCallback = callback;

	return TRUE;
}

// Registers 'is restarting' callback
BOOL Process::RegisterRestartCallback(ProcessCallback callback)
{
	if (!callback)
		return FALSE;
	if (restartCallback)
		return FALSE;

	restartCallback = callback;

	return TRUE;
}

// Registers 'stopped' callback
BOOL Process::RegisterStoppedCallback(ProcessCallback callback)
{
	if (!callback)
		return FALSE;
	if (stoppedCallback)
		return FALSE;

	stoppedCallback = callback;

	return TRUE;
}

// Registers 'resumed' callback
BOOL Process::RegisterResumedCallback(ProcessCallback callback)
{
	if (!callback)
		return FALSE;
	if (resumedCallback)
		return FALSE;

	resumedCallback = callback;

	return TRUE;
}

BYTE	Process::getStatus()			const { return iStatus; }
DWORD	Process::getId()				const { return id; }
HANDLE	Process::getHandle()			const { return hProcess; }
TCHAR *	Process::getCommandLine()		const 
{ 
	if (szCmdLine != NULL && strlen(szCmdLine))
		return szCmdLine; 

	return "< cannot get a commang line >";
}

TCHAR *	Process::getProcessName()		const
{
	if (hProcess != NULL)
		return GetNameByHandle(hProcess);

	return NULL;
}

/*
Private methods!
*/

void Process::OnExited(BOOL _dont_restart)
{
	if (exitCallback)
		exitCallback(this);

	// Restart the process 
	if (szCmdLine!= NULL && strlen(szCmdLine) && !_dont_restart){
		Create(szCmdLine);
		RegisterExitCallback(exitCallback);
	}
}

void Process::OnStarted()
{
	iStatus = PROC_WORKING;

	if (startCallback)
		startCallback(this);
}

void Process::OnRestart()
{
	iStatus = PROC_RESTARTING;

	if (restartCallback)
		restartCallback(this);
}

void Process::OnStopped()
{
	iStatus = PROC_STOPPED;

	if (stoppedCallback)
		stoppedCallback(this);
}

void Process::OnResumed()
{
	iStatus = PROC_WORKING;

	if (resumedCallback)
		resumedCallback(this);
}