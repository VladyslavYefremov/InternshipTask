#include "Process.h"

Process::Process() : hProcess(nullptr),
					hThread(nullptr),
					hWait(nullptr),
					exitCallback(nullptr),
					stoppedCallback(nullptr),
					startCallback(nullptr),
					restartCallback(nullptr),
					resumedCallback(nullptr),
					szCmdLine(nullptr),
					id(0){}

Process::Process(TCHAR * _szCmdLine) : hProcess(nullptr),
					hThread(nullptr),
					hWait(nullptr),
					exitCallback(nullptr),
					stoppedCallback(nullptr),
					startCallback(nullptr),
					restartCallback(nullptr),
					resumedCallback(nullptr),
					szCmdLine(nullptr),
					id(0)
{
	Create(_szCmdLine);
}

/* destructor */
Process::~Process()
{
	Destroy();
}

/* function creates new process using command line */
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

		szCmdLine = GetCommandLine(hProcess);

		/* calls 'process started' */
		OnStarted();
	}

	return bProcessCreated;
}

/* function opens an existing process using its id */
BOOL Process::Open(DWORD _id)
{
	/* buffering variable */
	HANDLE lhProcess = OpenProcess(PROCESS_ALL_ACCESS, false, _id);

	if (lhProcess != nullptr)
	{
		if (hProcess != nullptr)
			Destroy();

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

/* resumes the process */
BOOL Process::Resume()
{
	if (ResumeThread(hThread)){
		OnResumed();
		return TRUE;
	}
	return FALSE;
}

/* stopss the process */
BOOL Process::Stop()
{
	if (SuspendThread(hThread) < 1){
		OnStopped();
		return TRUE;
	}
	return FALSE;
}

/* destroys the process */
BOOL Process::Destroy() {
	if (hThread != nullptr) {
		CloseHandle(hThread);
	}

	/* unregister wait */
	if (hWait != nullptr)
	{
		::UnregisterWaitEx(hWait, INVALID_HANDLE_VALUE);	// INVALID_HANDLE_VALUE means "Wait for pending callbacks"
		hWait = nullptr;
	}

	if (hProcess != nullptr)
	{
		/* 
		*	Call 'process exited'
		*	param. TRUE means - not restart process by its command line
		*/
		OnExited(TRUE);

		DWORD code;
		if (GetExitCodeProcess(hProcess, &code))
		{
			/* if the process is still alive, terminate it */
			if (code == STILL_ACTIVE) {
				TerminateProcess(hProcess, 0);
			}
		}
		CloseHandle(hProcess);
	}

	if (szCmdLine != nullptr) {
		delete[] szCmdLine;
		szCmdLine = nullptr;
	}

	id = NULL;
	hProcess = nullptr;
	hThread = nullptr;
	iState = PROC_TERMINATED;

	return TRUE;
}

/* restarts the process */
BOOL Process::Restart()
{
	if (hProcess == nullptr)
		return FALSE;

	/* Call 'process is restarting' */
	OnRestart();

	/* remembers the command line */
	DWORD iLen = strlen(szCmdLine), _id = id;
	TCHAR * buffer = new TCHAR[iLen + 1];
	strcpy_s(buffer, iLen + 1, szCmdLine);

	/* destroy current process */
	Destroy();

	/* if we have the command line, create new process */
	if (iLen){
		/*  create new process (using command line) */
		Create(buffer);
		/* re-registration of 'exit' callback */
		RegisterExitCallback(exitCallback);
		return TRUE;
	}

	delete[] buffer;

	return FALSE;
}

/* function registers `exit` callback */
BOOL Process::RegisterExitCallback(ProcessCallback callback)
{
	if (!callback) 
		return FALSE;

	exitCallback = callback;

	/* directs a wait thread in the thread pool to wait on the object */
	return RegisterWaitForSingleObject(&hWait, hProcess, OnExited, this, INFINITE, WT_EXECUTEONLYONCE);
}

/* function registers `started` callback */
BOOL Process::RegisterStartedCallback(ProcessCallback callback)
{
	if (!callback)
		return FALSE;
	if (startCallback)
		return FALSE;

	startCallback = callback;

	return TRUE;
}

/* function registers `restarting` callback */
BOOL Process::RegisterRestartCallback(ProcessCallback callback)
{
	if (!callback)
		return FALSE;
	if (restartCallback)
		return FALSE;

	restartCallback = callback;

	return TRUE;
}

/* function registers `stopped` callback */
BOOL Process::RegisterStoppedCallback(ProcessCallback callback)
{
	if (!callback)
		return FALSE;
	if (stoppedCallback)
		return FALSE;

	stoppedCallback = callback;

	return TRUE;
}

/* function registers `resumed` callback */
BOOL Process::RegisterResumedCallback(ProcessCallback callback)
{
	if (!callback)
		return FALSE;
	if (resumedCallback)
		return FALSE;

	resumedCallback = callback;

	return TRUE;
}

/* function returns the state of the process */
BYTE	Process::getState()			const { return iState; }

/* function returns the id of the process */
DWORD	Process::getId()				const { return id; }

/* function returns the handle of the process */
HANDLE	Process::getHandle()			const { return hProcess; }

/* function returns pointer to the command line of the process (if it exists. else - ptr to msg) */
TCHAR *	Process::getCommandLine()		const 
{ 
	if (szCmdLine != nullptr && strlen(szCmdLine))
		return szCmdLine; 

	return "< cannot get a commang line >";
}

/* function returns pointer to the name of the process (if it exists. else - nullptr) */
TCHAR *	Process::getProcessName()		const
{
	if (hProcess != nullptr)
		return GetNameByHandle(hProcess);

	return nullptr;
}

/*
	Private methods!
*/

void Process::OnExited(BOOL _dont_restart)
{
	if (exitCallback)
		exitCallback(this);

	/* let's resturt the process */
	if (szCmdLine != nullptr && strlen(szCmdLine) && !_dont_restart){
		Create(szCmdLine);
		RegisterExitCallback(exitCallback);
	}
}

void Process::OnStarted()
{
	iState = PROC_WORKING;

	if (startCallback)
		startCallback(this);
}

void Process::OnRestart()
{
	iState = PROC_RESTARTING;

	if (restartCallback)
		restartCallback(this);
}

void Process::OnStopped()
{
	iState = PROC_STOPPED;

	if (stoppedCallback)
		stoppedCallback(this);
}

void Process::OnResumed()
{
	iState = PROC_WORKING;

	if (resumedCallback)
		resumedCallback(this);
}