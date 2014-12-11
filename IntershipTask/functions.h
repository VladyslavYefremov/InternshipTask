#ifndef _ADDITIONAL_FUNCTIONS
#define _ADDITIONAL_FUNCTIONS

#include <Windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <fstream>
#include <psapi.h>
#include <regex>

#define TBL_ID			"Process ID"
#define TBL_HP			"Process Handle"
#define TBL_NM			"Name"
#define TBL_ST			"Status"
#define SIZE			32		
#define _ARRAY_SIZE_1	2
#define _ARRAY_SIZE_2	4

using namespace std;

/*
*	string getStringByPointer (TCHAR * )
*	
*	in:  TCHAR pointer
*	out: string
*
*	function takes a TCHAR pointer, copies _str to ret , and deallocates the memory block pointed by _str.
*/
string getStringByPointer(TCHAR * _str){
	if (_str == nullptr)	
		return string("undefined");

	string ret = string(_str);
	delete[] _str;

	return ret;
}

/* a standard 32-bit datatype for system-supplied status code values. */
typedef NTSTATUS(NTAPI *_NtQueryInformationProcess)(
	HANDLE ProcessHandle,
	DWORD ProcessInformationClass,
	PVOID ProcessInformation,
	DWORD ProcessInformationLength,
	PDWORD ReturnLength
	);

/* is used to define Unicode strings */
typedef struct _UNICODE_STRING
{
	USHORT Length;			// The length, in bytes, of the string stored in Buffer.
	USHORT MaximumLength;	// The length, in bytes, of Buffer.
	PWSTR Buffer;			// Pointer to a buffer used to contain a string of wide characters.
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _PROCESS_BASIC_INFORMATION
{
	LONG ExitStatus;
	PVOID PebBaseAddress;
	ULONG_PTR AffinityMask;
	LONG BasePriority;
	ULONG_PTR UniqueProcessId;
	ULONG_PTR ParentProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

/* function returns the address of the process environment block (PEB) for a system process. */
PVOID GetPebAddress(HANDLE ProcessHandle)
{
	_NtQueryInformationProcess NtQueryInformationProcess =
		(_NtQueryInformationProcess)GetProcAddress(
		GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");
	PROCESS_BASIC_INFORMATION pbi;

	NtQueryInformationProcess(ProcessHandle, 0, &pbi, sizeof(pbi), NULL);

	return pbi.PebBaseAddress;
}

/* function returns the command line of the process using handle */
TCHAR * GetCommandLine(HANDLE hProcess){

	PVOID pebAddress;
	PVOID rtlUserProcParamsAddress;
	UNICODE_STRING commandLine;
	WCHAR *commandLineContents;

	pebAddress = GetPebAddress(hProcess);

	/* get the address of ProcessParameters */
	if (!ReadProcessMemory(hProcess, (PCHAR)pebAddress + 0x10,
		&rtlUserProcParamsAddress, sizeof(PVOID), NULL))
	{
		return "Error: Could not read the address of ProcessParameters!";
	}

	/* read the CommandLine UNICODE_STRING structure */
	if (!ReadProcessMemory(hProcess, (PCHAR)rtlUserProcParamsAddress + 0x40,
		&commandLine, sizeof(commandLine), NULL))
	{
		return "Error: Could not read CommandLine!";
	}

	/* allocate memory to hold the command line */
	commandLineContents = (WCHAR *)malloc(commandLine.Length);

	/* read the command line */
	if (!ReadProcessMemory(hProcess, commandLine.Buffer,
		commandLineContents, commandLine.Length, NULL))
	{
		return "Error: Could not read the command line string!";
	}

	/* the length specifier is in characters, but commandLine.Length is in bytes */
	/* a WCHAR is 2 bytes */

	/* szBuffer stays allocated, and will be returned to the caller (to be deallocated later) */
	TCHAR * szBuffer = new TCHAR[commandLine.Length / 2 + 1];
	sprintf_s(szBuffer, commandLine.Length / 2 + 1, "%.*S", commandLine.Length / 2, commandLineContents);

	return szBuffer;
}

/* function returns the name of the process using handle */
TCHAR * GetNameByHandle(HANDLE hProcess)
{
	TCHAR szProcessName[MAX_PATH] = TEXT("cannot get");

	if (hProcess != nullptr)
	{
		HMODULE hMod;
		DWORD cbNeeded;

		/* 4etrieves information about the process. */
		if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
		{
			/* Retrieves the base name of the module. */
			GetModuleBaseName(hProcess, hMod, szProcessName, MAX_PATH);
		}
	}

	/* szBuffer stays allocated, and will be returned to the caller (to be deallocated later) */
	TCHAR * szBuffer = new TCHAR[strlen(szProcessName) + 1];
	sprintf_s(szBuffer, strlen(szProcessName) + 1, "%s", szProcessName);

	return szBuffer;
}

/* function returns the thread of the process by its id */
HANDLE GetThreadByID(DWORD processId)
{
	/* if the function succeeds, it returns an open handle to the specified snapshot */
	HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	HANDLE hThread = nullptr;

	THREADENTRY32 threadEntry;
	threadEntry.dwSize = sizeof(THREADENTRY32);

	/* retrieves information about the first thread of any process encountered in a system snapshot */
	Thread32First(hThreadSnapshot, &threadEntry);

	do
	{
		if (threadEntry.th32OwnerProcessID == processId)
		{
			/* opens an existing thread object */
			hThread = OpenThread(THREAD_ALL_ACCESS, FALSE,
				threadEntry.th32ThreadID);
		}
	} while (Thread32Next(hThreadSnapshot, &threadEntry));

	CloseHandle(hThreadSnapshot);

	return hThread;
}

/* 
*	reads and checks data
*
*	info:		phrase describing the read information
*	pattern:	regular expression
*/
#ifdef USE_STRING_INSTEADOF_TCHAR
string incoming_data(TCHAR* info, TCHAR* pattern)
{
	BOOL is_correct = FALSE, is_first = TRUE;

	regex rx(pattern);
	string sBuffer;

	do {
		if (!is_first)
			cout << endl << info << ": ";

		cin >> sBuffer;
		is_correct = TRUE;

		/* check the correctness of the entering data */
		if (!regex_match(sBuffer, rx)) {
			cout << endl << "Incorrect value! Try again!";
			is_correct = FALSE;
		}

		is_first = FALSE;
	} while (!is_correct);

	return sBuffer;
}
#else
TCHAR * incoming_data(TCHAR* info, TCHAR* pattern)
{
	TCHAR * pStr = NULL;
	BOOL is_correct = FALSE, is_first = TRUE;

	regex rx(pattern);

	do {
		/* free memory */
		if (pStr != nullptr) {
			memset(pStr, '\0', strlen(pStr) + 1);
			delete[] pStr;
		}
		pStr = new TCHAR[MAX_PATH];

		if (!is_first)
			cout << endl << info << ": ";

		cin >> pStr;
		is_correct = TRUE;

		/* check the correctness of the entering data */
		if (!regex_match(pStr, rx)) {
			cout << endl << "Incorrect value! Try again!";
			is_correct = FALSE;
		}

		is_first = FALSE;
	} while (!is_correct);

	return pStr;
}
#endif

/* overload of displaying an object (Process) */
ostream & operator<<(ostream & stream, const Process * Obj)
{
	string strData[_ARRAY_SIZE_1][_ARRAY_SIZE_2];
	TCHAR buffer[SIZE];

	/* despite the fact that this overload is friend-function, I use public methods to get variables */

	unsigned int iLengths[_ARRAY_SIZE_2];
	
	strData[0][0] = TBL_ID;
	strData[0][1] = TBL_HP;
	strData[0][2] = TBL_ST;
	strData[0][3] = TBL_NM;

	for (unsigned int i = 0; i < _ARRAY_SIZE_2; i++){
		iLengths[i] = strData[0][i].length();
	}

	sprintf_s(buffer, "%d", Obj->getId());
	strData[1][0] = buffer;

	sprintf_s(buffer, "%p", Obj->getHandle());
	strData[1][1] = buffer;

	int iState = Obj->getState();

	strData[1][2] = (iState == PROC_WORKING ? "is working" : (iState == PROC_RESTARTING ? "is restarting" : (iState == PROC_STOPPED ? "is stopped" : "undefined")));
	strData[1][3] = getStringByPointer(Obj->getProcessName());

	for (unsigned int i = 0; i < _ARRAY_SIZE_2; i++){
		iLengths[i] = max(iLengths[i], strData[1][i].length());
	}

	/* top of the table */
	stream << TOP_L << setfill(HORIZONTAL) << setw(iLengths[0]) << "" << 
		TOP_C << setfill(HORIZONTAL) << setw(iLengths[1]) << "" << 
		TOP_C << setfill(HORIZONTAL) << setw(iLengths[2]) << "" << 
		TOP_C << setfill(HORIZONTAL) << setw(iLengths[3]) << "" << 
		TOP_R << endl;
	/* outputs title */
	stream << VERTICAL << setfill(' ') << setw(iLengths[0]) << strData[0][0] <<
		VERTICAL << setfill(' ') << setw(iLengths[1]) << strData[0][1] <<
		VERTICAL << setfill(' ') << setw(iLengths[2]) << strData[0][2] <<
		VERTICAL << setfill(' ') << setw(iLengths[3]) << strData[0][3] <<
		VERTICAL << endl;
	/* center of the table */
	stream << CENTER_L << setfill(HORIZONTAL) << setw(iLengths[0]) << "" <<
		CENTER_C << setfill(HORIZONTAL) << setw(iLengths[1]) << "" <<
		CENTER_C << setfill(HORIZONTAL) << setw(iLengths[2]) << "" <<
		CENTER_C << setfill(HORIZONTAL) << setw(iLengths[3]) << "" <<
		CENTER_R << endl;
	/* outputs the data */
	stream << VERTICAL << setfill(' ') << setw(iLengths[0]) << strData[1][0] <<
		VERTICAL << setfill(' ') << setw(iLengths[1]) << strData[1][1] <<
		VERTICAL << setfill(' ') << setw(iLengths[2]) << strData[1][2] << 
		VERTICAL << setfill(' ') << setw(iLengths[3]) << strData[1][3] <<
		VERTICAL << endl;
	/* bottom of the table */
	stream << BOTTOM_L << setfill(HORIZONTAL) << setw(iLengths[0]) << "" <<
		BOTTOM_C << setfill(HORIZONTAL) << setw(iLengths[1]) << "" <<
		BOTTOM_C << setfill(HORIZONTAL) << setw(iLengths[2]) << "" <<
		BOTTOM_C << setfill(HORIZONTAL) << setw(iLengths[3]) << "" <<
		BOTTOM_R << endl;

	return stream;
}

#endif