#ifndef _PROPERTIES
#define _PROPERTIES

//#define default_process "C:\\Program Files (x86)\\Notepad++\\notepad++.exe"
#define default_process "C:\\Windows\\System32\\notepad.exe"

// Status of a process
#define PROC_TERMINATED		0
#define PROC_WORKING		1
#define PROC_RESTARTING		3
#define PROC_STOPPED		4

// For displaying table
#define TOP_L		'\xDA'
#define TOP_C		'\xC2'
#define TOP_R		'\xBF'
#define CENTER_L	'\xC3'
#define CENTER_C	'\xC5'
#define CENTER_R	'\xB4'
#define BOTTOM_L	'\xC0'
#define BOTTOM_C	'\xC1'
#define BOTTOM_R	'\xD9'
#define HORIZONTAL	'\xC4'
#define VERTICAL	'\xB3'

// In case of an error
#define ERROR_REGISTER_CALLBACK_STARTED		101
#define ERROR_REGISTER_CALLBACK_RESTART		102
#define ERROR_REGISTER_CALLBACK_STOPPED		103
#define ERROR_REGISTER_CALLBACK_RESUMED		104
#define ERROR_REGISTER_CALLBACK_EXIT		105
#define ERROR_CONNECT_TO_PROCES				106

#endif