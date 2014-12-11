#ifndef _PROCESS_H
#define _PROCESS_H

#include <Windows.h>
#include <iostream>
#include <iomanip>

#include "properties.h"

using namespace std;

TCHAR * GetCommandLine(HANDLE);
TCHAR * GetNameByHandle(HANDLE);
HANDLE	GetThreadByID(DWORD);

class Process
{
public:
	typedef void(*ProcessCallback)(Process const*);
	friend ostream & operator<<(ostream &, const Process *);

	Process();
	Process(TCHAR *);
	~Process();

	BOOL Create(TCHAR *);
	BOOL Create(DWORD);

	BOOL Stop();
	BOOL Resume();
	BOOL Restart();
	BOOL DestroyProcess();

	BOOL RegisterExitCallback(ProcessCallback);
	BOOL RegisterStartedCallback(ProcessCallback);
	BOOL RegisterRestartCallback(ProcessCallback);
	BOOL RegisterStoppedCallback(ProcessCallback);
	BOOL RegisterResumedCallback(ProcessCallback);

	DWORD	getId()				const;
	BYTE	getStatus()			const;
	HANDLE	getHandle()			const;
	TCHAR *	getCommandLine()	const;
	TCHAR * getProcessName()	const;

private:

	static void CALLBACK OnExited(void* context, BOOLEAN isTimeOut)
	{
		((Process*)context)->OnExited();
	}

	void OnExited(BOOL _dont_restart = FALSE);
	void OnStarted();
	void OnRestart();
	void OnStopped();
	void OnResumed();

	TCHAR *				szCmdLine;
	DWORD				id;
	BYTE				iStatus;
	HANDLE				hProcess;
	HANDLE				hThread;
	HANDLE				hWait;
	ProcessCallback		startCallback;
	ProcessCallback		restartCallback;
	ProcessCallback		stoppedCallback;
	ProcessCallback		resumedCallback;
	ProcessCallback		exitCallback;
};

#endif