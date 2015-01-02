<h2>General info</h2>
• Language – ++11.<br>
• IDE&Compiler– MS Visual Studio 2012+.<br>
• OS – Windows 8+.<br>
• Don’t use additional libraries. Task can be accomplished using only std
lib of ++ and WinAPI.

<h2>Task</h2>
Create class to launch and monitor win32 process. Class constructor
accepts command line path and arguments. Class instance must launch a
process and watch it's state. In case of crash or exit it must restart
process using command line from constructor.

<h2>Class must</h2>
• Start and restart process.<br>
• Allow to retrieve process info (handle, id, status (is working, restarting,
stopped)).<br>
• Allow to stop process via method call (without restart) and start it again.<br>
• Log all events (start, crash, manual shutdown) to EventLog or file.<br>
Logger instance must be configurable OOP-style.<br>
• Allow to add callbacks to all events (std::function<void()>). For example
OnProcStart, OnProcCrash, OnProcManuallyStopped.<br>
• All methods must be thread-safe.<br>
• All resources (process handles, threads, file handles, logger, etc.) must
be properly released.

<h2>Advanced</h2>
Make class able to watch already running process. User can specify
process ID and your code must start watching this process and extract
command line for this process. In case of exit or crash your code must
start process with exact same arguments as it was started before.
Assume that target process doesn't defend itself from such operations
and doesn`t overwrite memory of process start info structures.
