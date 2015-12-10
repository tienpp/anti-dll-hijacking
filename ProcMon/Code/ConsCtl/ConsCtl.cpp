//---------------------------------------------------------------------------
//
// ConsCtl.cpp
//
// SUBSYSTEM: 
//              Monitoring process creation and termination  
//
// MODULE:    
//				Control application for monitoring NT process and 
//              DLLs loading observation. 
//
// DESCRIPTION:
//
// AUTHOR:		Ivo Ivanov
//                                                                         
//---------------------------------------------------------------------------

#include "Common.h"
#include <conio.h>
#include <tchar.h>
#include "ApplicationScope.h"
#include "CallbackHandler.h"
#include "LoadLibraryR.h"

#define BREAK_WITH_ERROR( e ) { printf( "[-] %s. Error=%d", e, GetLastError() ); break; }

//
// This constant is declared only for testing putposes and
// determines how many process will be created to stress test
// the system
//
const int MAX_TEST_PROCESSES = 10;

LPVOID lpBuffer       = NULL;
DWORD dwLength        = 0;
DWORD dwBytesRead     = 0;
char * cpDllFile  = "C:\\dll\\prevent_preloadDLL.dll";

//---------------------------------------------------------------------------
// 
// class CWhatheverYouWantToHold
//
// Implements a dummy class that can be used inside provide callback 
// mechanism. For example this class can manage sophisticated methods and 
// handles to a GUI Win32 Window. 
//
//---------------------------------------------------------------------------
class CWhatheverYouWantToHold
{
public:
	CWhatheverYouWantToHold()
	{
		_tcscpy(m_szName, TEXT("This could be any user-supplied data."));
		hwnd = NULL;
	}
private:
	TCHAR  m_szName[MAX_PATH];
	// 
	// You might want to use this attribute to store a 
	// handle to Win32 GUI Window
	//
	HANDLE hwnd;
};


//---------------------------------------------------------------------------
// 
// class CMyCallbackHandler
//
// Implements an interface for receiving notifications
//
//---------------------------------------------------------------------------
class CMyCallbackHandler: public CCallbackHandler
{
	//
	// Implements an event method
	//
	virtual void OnProcessEvent(
		PQUEUED_ITEM pQueuedItem, 
		PVOID        pvParam
		)

	{
		TCHAR szFileName[MAX_PATH];
		//
		// Deliberately I decided to put a delay in order to 
		// demonstrate the queuing / multithreaded functionality 
		//
		::Sleep(500);
		//
		// Get the dummy parameter we passsed when we 
		// initiated process of monitoring (i.e. StartMonitoring() )
		//
		CWhatheverYouWantToHold* pParam = static_cast<CWhatheverYouWantToHold*>(pvParam);
		//
		// And it's about time to handle the notification itself
		//
		if (NULL != pQueuedItem)
		{
			TCHAR szBuffer[1024];
			TCHAR command[1024];
			HANDLE hModule        = NULL;
			HANDLE hProcess       = NULL;
			HANDLE hToken         = NULL;
			DWORD dwProcessId     = 0;
			TOKEN_PRIVILEGES priv = {0};


			::ZeroMemory(
				reinterpret_cast<PBYTE>(szBuffer),
				sizeof(szBuffer)
				);
			if (pQueuedItem->bCreate)
			{
				//
				// At this point you can use OpenProcess() and
				// do something with the process itself
				//
				
				GetProcessName(
					reinterpret_cast<DWORD>(pQueuedItem->hProcessId), 
					szFileName, 
					MAX_PATH
					);
				do{
					if( OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) )
					{
						priv.PrivilegeCount           = 1;
						priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		
						if( LookupPrivilegeValue( NULL, SE_DEBUG_NAME, &priv.Privileges[0].Luid ) )
							AdjustTokenPrivileges( hToken, FALSE, &priv, 0, NULL, NULL );

						CloseHandle( hToken );
					}

					dwProcessId = (DWORD)pQueuedItem->hProcessId;

					hProcess = OpenProcess( PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, dwProcessId );
					if( !hProcess )
						BREAK_WITH_ERROR( "Failed to open the target process" );

					hModule = LoadRemoteLibraryR( hProcess, lpBuffer, dwLength, NULL );
					if( !hModule )
						BREAK_WITH_ERROR( "Failed to inject the DLL" );

					printf( "[+] Injected the '%s' DLL into process %d.", cpDllFile, dwProcessId );
		
					WaitForSingleObject( hModule, -1 );
				} while( 0 );
				//wsprintf(command, TEXT(" %d C:\\dll\\reflective_dll.dll\x00"), pQueuedItem->hProcessId);
				//_tprintf(command);
				//ShellExecute(0, L"open", L"C:\\dll\\inject.exe", command, 0, SW_HIDE);
				wsprintf(
					szBuffer,
					TEXT("Process has been created: PID=%d %s\n"),
					pQueuedItem->hProcessId,
					szFileName
					);
			}
			else
				wsprintf(
					szBuffer,
					TEXT("Process has been terminated: PID=0x%.8X\n"),
					pQueuedItem->hProcessId);
			//
			// Output to the console screen
			//
			_tprintf(szBuffer);
		} // if
	}
};

//---------------------------------------------------------------------------
// Perform
//
// Thin wrapper around __try..__finally
//---------------------------------------------------------------------------
void Perform(
	CCallbackHandler*        pHandler,
	CWhatheverYouWantToHold* pParamObject
	)
{
	DWORD processArr[MAX_TEST_PROCESSES] = {0};
	int i;
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	TCHAR szBuffer[MAX_PATH];  // buffer for Windows directory
	::GetWindowsDirectory(szBuffer, MAX_PATH);
	_tcscat(szBuffer, TEXT("\\notepad.exe"));

	//
	// Create the only instance of this object
	//
	CApplicationScope& g_AppScope = CApplicationScope::GetInstance(
		pHandler     // User-supplied object for handling notifications
		);
	__try
	{
		//
		// Initiate monitoring
		//
		g_AppScope.StartMonitoring(
			pParamObject // Pointer to a parameter value passed to the object 
			);
		while(!kbhit())
			{
			}
		_getch();
	}
	__finally
	{
		//
		// Terminate the process of observing processes
		//
		g_AppScope.StopMonitoring();
	}
}

//---------------------------------------------------------------------------
// 
// Entry point
//
//---------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	CMyCallbackHandler      myHandler;
	CWhatheverYouWantToHold myView; 
	HANDLE hFile          = NULL;


	do{
		hFile = CreateFileA( cpDllFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		if( hFile == INVALID_HANDLE_VALUE )
			BREAK_WITH_ERROR( "Failed to open the DLL file" );

		dwLength = GetFileSize( hFile, NULL );
		if( dwLength == INVALID_FILE_SIZE || dwLength == 0 )
			BREAK_WITH_ERROR( "Failed to get the DLL file size" );

		lpBuffer = HeapAlloc( GetProcessHeap(), 0, dwLength );
		if( !lpBuffer )
			BREAK_WITH_ERROR( "Failed to get the DLL file size" );

		if( ReadFile( hFile, lpBuffer, dwLength, &dwBytesRead, NULL ) == FALSE )
			BREAK_WITH_ERROR( "Failed to alloc a buffer!" );
	}while(0);

	Perform( &myHandler, &myView );

	return 0;
}
//--------------------- End of the file -------------------------------------
