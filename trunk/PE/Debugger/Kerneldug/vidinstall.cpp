#include <windows.h>
#include <stdio.h>
#include <imagehlp.h>

#define DBG 1

BOOLEAN IsFileExist (PCHAR Name)
{
	HANDLE hFile = CreateFile (Name, FILE_READ_ATTRIBUTES,
		FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING,
		0, 0);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle (hFile);
		return TRUE;
	}
	return FALSE;
}

char chrupr (char chr)
{
	char str[2] = {chr, 0};
	strupr (str);
	return str[0];
}

int yesno (char *question, char def)
{
	char ans[10];
	bool is_default_yes = ( chrupr(def) == 'Y' );
	char not_def = ( is_default_yes ? 'N' : 'Y' );

	do
	{
		printf("%s (Y/N)? [%c]", question, def);	

		fgets (ans, sizeof(ans)-1, stdin);

		if ( chrupr(ans[0]) == chrupr(def) || ans[0] == '\n' )
		{
			return is_default_yes;
		}
		else if ( chrupr(ans[0]) == not_def)
		{
			return !is_default_yes;
		}
	}
	while (true);
}

int _main()
{
	char ans[256];
	char System32[512];

	GetSystemDirectory (System32, sizeof(System32));

	/*
	//
	// Creating log file
	//

#if DBG == 0
	if (yesno("Do you wish to create debug log file? "
		"It is necessary for debug version and highly recommended in retail version", 'Y'))
#endif

	{
		printf("Creating debug log file\n");

		HANDLE hFile = CreateFile ("C:\\ngdisp.log", GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0 ,0);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			printf("Setup was unable to create debug log file C:\\ngdisp.log. Cannot continue\n");
			return 1;
		}

		// Write four zeroes
		ULONG wr;
		char string[] = "\6\0\0\0\r\n";

		WriteFile (hFile, string, sizeof(string)-1, &wr, 0);

		CloseHandle (hFile);

		printf("Debug log created\n");
	}
*/

	printf( "NGvid video driver installation program\n"
			"Please, answer some questions about your system\n");

	int Video = 0;

	printf("What is the number of your active display adapter? [Video0] ");
	fgets (ans, sizeof(ans)-1, stdin);

	strupr (ans);
	if (strstr(ans, "VIDEO"))
	{
		char *p = (ans + 5); // strlen("VIDEO")
		Video = atoi (p);
	}

	printf("Using VIDEO%d\n", Video);

	HKEY hk = (HKEY) 0;

	if (RegOpenKeyEx (
			HKEY_LOCAL_MACHINE, 
			"HARDWARE\\DEVICEMAP\\VIDEO", 
			0, 
			KEY_QUERY_VALUE, 
			&hk) != ERROR_SUCCESS)
	{
		printf("Setup was unable to open HARDWARE registry key\nCheck access rights\n");
		return 0;
	}

	char value[256];
	sprintf(value, "\\Device\\Video%d", Video);

	char *buf = ans;
	ULONG len = sizeof(ans)-1;
	ans[0] = 0;


	if (RegQueryValueEx (hk, value, 0, 0, (UCHAR*)buf, &len) != ERROR_SUCCESS)
	{
		printf("Setup was unable to query value of HARDWARE key\nCheck access rights\n");
		return 0;
	}

	RegCloseKey (hk);

	buf += strlen("\\Registry\\Machine\\");

	printf("Got video key: %s\n", buf);

	if (RegOpenKeyEx (
			HKEY_LOCAL_MACHINE,
			buf,
			0,
			KEY_QUERY_VALUE,
			&hk) != ERROR_SUCCESS)
	{
		printf("Setup was unable to open video miniport driver service key\n");
		return 0;
	}

	char InstalledDisplayDrivers[512] = "";
	len = sizeof(InstalledDisplayDrivers)-1;
	if (RegQueryValueEx (hk, "InstalledDisplayDrivers", 0, 0, (UCHAR*)InstalledDisplayDrivers, &len) != ERROR_SUCCESS)
	{
		printf("Setup was unable to query list of installed display drivers\n");
		return 0;
	}

	RegCloseKey (hk);

	printf("Installed display drivers: %s\n", InstalledDisplayDrivers);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	printf("Installing font\n");

	if(!CopyFile ("font32.bmp", "C:\\font32.bmp", FALSE))
	{
		printf("Could not copy font\n");
		return 0;
	}
	
	system("reg import ngdbg.reg");	
	
	printf("Copying display driver\n");

	char oldname[512];
	char newname[512];

	sprintf (oldname, "%s\\X_%s.dll", System32, InstalledDisplayDrivers);
	sprintf (newname, "%s\\vid_copy.dll", System32);

	if (!IsFileExist (oldname))
	{
		printf("vid is not installed, using primary name\n");
		sprintf (oldname, "%s\\%s.dll", System32, InstalledDisplayDrivers);
	}
	else
	{
		printf("vid is installed, using secondary name\n");
	}

	if (!CopyFile (oldname, newname, FALSE))
	{
		printf("Could not copy display driver\n");
		return 0;
	}
	
	printf("Display driver copied\n");

	HKEY hKey;
	if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, "Software\\NGdbg", 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
	{
		printf("RegOpeyKeyEx failed\n");
		return 0;
	}

	if(RegSetValueEx (hKey, "DisplayDriver", 0, REG_SZ, (PBYTE)InstalledDisplayDrivers, strlen(InstalledDisplayDrivers))
		!= ERROR_SUCCESS)
	{
		printf("RegSetValueEx failed\n");
		return 0;
	}

	RegCloseKey (hKey);

	printf("Completed successfully\n");

	return 0;



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	//
	// open new driver
	//

	printf("Patching display driver\n");

	char System32Name[512];
	strcpy (System32Name, System32);

	strcat (System32Name, "\\display.dll");

	if(!CopyFile ("display.dll", System32Name, FALSE))
	{
		printf("Setup was unable to copy driver to system32 directory\n");
		return 0;
	}

	HANDLE hFile = CreateFile (System32Name, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING,
		0, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("Setup was unable to open display.dll\nCheck installation package\n");
		return 0;
	}

	HANDLE hMapping = CreateFileMapping (hFile, 0, PAGE_READWRITE, 0, 0, 0);
	if (!hMapping)
	{
		printf("Setup was unable to create mapping for display.dll\n");
		return 0;
	}

	LPVOID lpMap = MapViewOfFile (hMapping, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
	if (!lpMap)
	{
		printf("Setup was unable to map view of display.dll\n");
		return 0;
	}

	WCHAR DriverName[128];
	wsprintfW (DriverName, L"X_%S.dll", InstalledDisplayDrivers);

	ULONG Size = GetFileSize (hFile, 0);
	BOOLEAN Found = FALSE;
	
	for (ULONG i=0; i<Size; i++)
	{
		WCHAR *m = (WCHAR*) &((char*)lpMap)[i];

		if (m[0] == 'X' &&
			m[1] == '_' &&
			m[2] == 'v' &&
			m[3] == 'm' &&
			m[4] == 'x' &&
			m[5] == '_' &&
			m[6] == 'f' &&
			m[7] == 'b' &&
			m[8] == '.' &&
			m[9] == 'd' &&
			m[10] == 'l' &&
			m[11] == 'l' &&
			m[12] == '\0')
		{
			printf("Find string: %S\n", m);
			printf("New name: %S\n", DriverName);

			memcpy (m, DriverName, wcslen(DriverName)*2+2);

			Found = TRUE;

			ULONG HeaderSum, CheckSum;
			PIMAGE_NT_HEADERS nt = CheckSumMappedFile (lpMap, Size, &HeaderSum, &CheckSum);
			nt->OptionalHeader.CheckSum = CheckSum;

			break;
		}
	}

	UnmapViewOfFile (lpMap);
	CloseHandle (hMapping);
	CloseHandle (hFile);

	if (!Found)
	{
		printf("Setup was unable to find driver name in display.dll\n"
			"File is corrupted\n");
		return 0;
	}

	//
	// try to rename old driver
	//

	char OldSystem32Name[512];
	char NewSystem32Name[512];
	strcpy (OldSystem32Name, System32);
	strcpy (NewSystem32Name, System32);
	
	sprintf(OldSystem32Name+strlen(OldSystem32Name),
		"\\%s.dll", InstalledDisplayDrivers);

	sprintf(NewSystem32Name+strlen(NewSystem32Name),
		"\\X_%s.dll", InstalledDisplayDrivers);

	printf("Renaming %s to %s\n", OldSystem32Name, NewSystem32Name);

	if (IsFileExist (NewSystem32Name))
	{
		//
		// Previous version exists
		//

		printf("Previous version exists, renaming it\n");

		char NewOldVersionName[512];
		sprintf (NewOldVersionName,
			"%s\\__vid_old_%08x__%s.dll",
			System32,
			GetTickCount(),
			InstalledDisplayDrivers
			);

		// move old version
		MoveFile (OldSystem32Name, NewOldVersionName);
	}
	else
	{
		printf("No prev version, installing new\n");
		if(!MoveFile (OldSystem32Name, NewSystem32Name))
		{
			printf("Setup was unable to rename existing driver\n");
			return 0;
		}
	}

	//
	// copy new driver
	//

	sprintf(NewSystem32Name,
		"%s\\%s.dll", System32, InstalledDisplayDrivers);

	if(!MoveFile (System32Name, NewSystem32Name))
	{
		printf("Setup was unable to move new driver\n");
		return 0;
	}

	//
	// create unstall script
	//

	hFile = CreateFile ("uninstall.cmd", GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE)
		printf("Installation completed, but uninstall script cannot be created\n");
	else
	{
		char script[1024];

		wsprintf (script, 
			"@echo off\r\n"
			"echo.\r\n"
			"echo NGvid uninstall script\r\n"
			"echo Press Ctrl-C to quit or any other key to uninstall\r\n"
			"pause > nul\r\n"
			"ren %s\\%s.dll %s\\___%s.dll\r\n"
			"ren %s\\X_%s.dll %s\\%s.dll\r\n"
			"echo Uninstall completed\r\n"
			"echo.\r\n"
			"pause > nul\r\n"
			,

			System32, InstalledDisplayDrivers, System32, InstalledDisplayDrivers,
			System32, InstalledDisplayDrivers, System32, InstalledDisplayDrivers
			);

		ULONG wr;
		WriteFile (hFile, script, strlen(script), &wr, 0);
		CloseHandle (hFile);
	}


	printf("Installation complete, reboot is required. Reboot now (Y/N)? [Y]");
	fgets (ans, sizeof(ans)-1, stdin);

	if (ans[0] == 'Y' || ans[0] == 'y' || ans[0] == '\r')
	{
		printf("Installation completed, but you have to reboot manually later\n");
	}
	else
	{
		system("shutdown -r -t 0");
	}

	printf("Press Ctrl-C to quit\n");

	return 0;
}

int main()
{
	_main();
	Sleep(-1);
	return 0;
}