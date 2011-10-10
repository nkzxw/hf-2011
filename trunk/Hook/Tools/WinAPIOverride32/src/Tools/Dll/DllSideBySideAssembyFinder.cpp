#include "DllSideBySideAssembyFinder.h"
#include "PeManifestAssembly.h"
#include "../File/StdFileOperations.h"
#include <stdio.h>
// MSDN
//Assembly Searching Sequence  
//
//If an isolated application specifies an assembly dependency, side-by-side first searches for the assembly among the shared assemblies in the WinSxS folder. If the required assembly is not found, side-by-side then searches for a private assembly installed in a folder of the application's directory structure. 
//
//Private assemblies may be deployed in the following locations in the application's directory structure: 
//
//In the application's folder. Typically, this is the folder containing the application's executable file.
//In a subfolder in the application's folder. The subfolder must have the same name as the assembly.
//In a language-specific subfolder in the application's folder. The name of the subfolder is a string of DHTML language codes indicating a language-culture or language.
//In a subfolder of a language-specific subfolder in the application's folder. The name of the higher subfolder is a string of DHTML language codes indicating a language-culture or language. The deeper subfolder has the same name as the assembly.
//The first time side-by-side searches for a private assembly, it determines whether a language-specific subfolder exists in the application's directory structure. If no language-specific subfolder exists, side-by-side searches for the private assembly in the following locations using the following sequence.
//
//Side-by-side searches the WinSxS folder.
//\\< appdir>\< assemblyname>.DLL 
//\\< appdir>\< assemblyname>.manifest 
//\\< appdir>\< assemblyname>\< assemblyname>.DLL 
//\\< appdir>\< assemblyname>\< assemblyname>.manifest 
//If a language-specific subfolder exists, the application's directory structure may contain the private assembly localized in multiple languages. Side-by-side searches the language-specific subfolders to ensure that the application uses the specified language or the best available language. Language-specific subfolders are named using a string of DHTML language codes that specify a language-culture or language. If a language-specific subfolder exists, side-by-side searches for the private assembly in the following locations using following sequence.
//
//Side-by-side searches the WinSxS folder.
//\\< appdir>\< language-culture>\< assemblyname>.DLL 
//\\< appdir>\< language-culture>\< assemblyname>.manifest 
//\\< appdir>\< language-culture>\< assemblyname>\< assemblyname>.DLL 
//\\< appdir>\< language-culture>\< assemblyname>\< assemblyname>.manifest 
//Note that the side-by-side search sequence finds a DLL file with the assembly's name and stops before searching for a manifest file having the assembly's name. The recommended way to handle a private assembly that is a DLL is to put the assembly manifest in the DLL file as a resource. The resource ID must be equal to 1 and the name of the private assembly may be the same as the name of the DLL. For example, if the name of the DLL is MICROSOFT.WINDOWS.MYSAMPLE.DLL, the value of the name attribute used in the assemblyIdentity element of the assembly's manifest may also be Microsoft.Windows.mysample. As an alternative, you may put the assembly manifest in a separate file, however, the name of the assembly and its manifest must then be different than the name of the DLL. For example, Microsoft.Windows.mysampleAsm, Microsoft.Windows.mysampleAsm.manifest, and MICROSOFT.WINDOWS.MYSAMPLE.DLL. 
//
//For example, if myapp is installed at the root of drive c: and requires myasm in French-Belgian, side-by-side uses the following sequence to search for the best approximation to a localized instance of myasm.
//
//Side-by-side searches WinSxS for the fr-be version.
//c:\myapp\fr-be\myasm.dll
//c:\myapp\fr-be\myasm.manifest
//c:\myapp\fr-be\myasm\myasm.dll
//c:\myapp\fr-be\myasm\myasm.manifest
//Side-by-side searches WinSxS for the fr version.
//c:\myapp\fr\myasm.dll
//c:\myapp\fr\myasm.manifest
//c:\myapp\fr\myasm\myasm.dll
//c:\myapp\fr\myasm\myasm.manifest
//Side-by-side searches WinSxS for the en-us version.
//c:\myapp\en-us\myasm.dll
//c:\myapp\en-us\myasm.manifest
//c:\myapp\en-us\myasm\myasm.dll
//c:\myapp\en-us\myasm\myasm.manifest
//Side-by-side searches WinSxS for the en version.
//c:\myapp\en\myasm.dll
//c:\myapp\en\myasm.manifest
//c:\myapp\en\myasm\myasm.dll
//c:\myapp\en\myasm\myasm.manifest
//Side-by-side searches WinSxS for the no language version.
//c:\myapp\myasm.dll
//c:\myapp\myasm.manifest
//c:\myapp\myasm\myasm.dll
//c:\myapp\myasm\myasm.manifest
//If side-by-side searching reaches a language-neutral version of the assembly, and a Multilanguage User Interface (MUI) version of Windows is present on the system, side-by-side then attempts to bind to < assemblyname>.mui. Side-by-side does not attempt to bind to < assemblyname>.mui if the search reaches a localized version of the assembly. The assembly manifest of a language-neutral assembly will not have a language attribute in its assemblyIdentity element. If side-by-side reaches a language-neutral assembly, and MUI is installed, side-by-side searches the following locations using the following sequence for < assemblyname>.mui. Side-by-side uses the same search sequence if the assembly is culture-neutral, except < no language> is not searched. 
//
//Side-by-side searches the WinSxS folder for < assemblyname>.mui. 
//\\< user's language-culture>\< assemblyname>.mui 
//\\< user's language>\< assemblyname>.mui 
//\\< system's language-culture>\< assemblyname>.mui 
//\\< system's language>\< assemblyname>.mui 
//\\< no language>\< assemblyname>.mui 
//For example, if side-by-side searching finds the private assembly at c:\myapp\myasm\myasm.manifest, and myasm is a language-neutral assembly. Side-by-side then uses the following sequence to search for myasm.mui. Note that side-by-side will not search for a language-neutral MUI assembly.
//
//Side-by-side searches WinSxS for the fr-be version of the MUI assembly.
//c:\myapp\fr-be\myasm.mui.dll
//c:\myapp\fr-be\myasm.mui.manifest
//c:\myapp\fr-be\myasm\myasm.mui.dll
//c:\myapp\fr-be\myasm\myasm.mui.manifest
//Side-by-side searches WinSxS for the fr version of the MUI assembly.
//c:\myapp\fr\myasm.mui.dll
//c:\myapp\fr\myasm.mui.manifest
//c:\myapp\fr\myasm\myasm.mui.dll
//c:\myapp\fr\myasm\myasm.mui.manifest
//Side-by-side searches WinSxS for the en-us version of the MUI assembly.
//c:\myapp\en-us\myasm.mui.dll
//c:\myapp\en-us\myasm.mui.manifest
//c:\myapp\en-us\myasm\myasm.mui.dll
//c:\myapp\en-us\myasm\myasm.mui.manifest
//Side-by-side searches WinSxS for the en version of the MUI assembly.
//c:\myapp\en\myasm.mui.dll
//c:\myapp\en\myasm.mui.manifest
//c:\myapp\en\myasm\myasm.mui.dll
//c:\myapp\en\myasm\myasm.mui.manifest



// OUT TCHAR* tFullPath : provided pointer have be at least MAX_PATH TCHAR
BOOL CDllSideBySideAssembyFinder::FindSideBySideAssemby(TCHAR* ImportingModulePath,TCHAR* tWorkingDirectory,TCHAR* tDllName,OUT TCHAR* tFullPath)
{
    *tFullPath=0;
    if (ImportingModulePath == NULL)
        return FALSE;
    if (*ImportingModulePath == NULL)
        return FALSE;

    CPeManifestAssembly PeManifest;
    if (!PeManifest.Parse(ImportingModulePath))
        return FALSE;

    int iCount = PeManifest.GetDependenciesCount();
    if(iCount==0)
    {
        // no dependencies have been found in the manifest.
        return FALSE;
    }
    else
    {
        // get windows dir
        TCHAR WindowsDir[MAX_PATH];
        ::GetWindowsDirectory(WindowsDir, MAX_PATH);
        TCHAR ImportingModuleDirectory[MAX_PATH];
        CStdFileOperations::GetFilePath(ImportingModulePath,ImportingModuleDirectory,MAX_PATH);
        TCHAR ModuleNameWithoutExt[MAX_PATH];
        _tcscpy(ModuleNameWithoutExt,tDllName);
        CStdFileOperations::RemoveFileExt(ModuleNameWithoutExt);
        

        CPeManifestAssemblyIdentity* pIdentity = PeManifest.GetFirstDependency();
        while( pIdentity )
        {
            TCHAR ImportedDllFullPath[MAX_PATH];
            *ImportedDllFullPath = 0;

            //pIdentity->GetName()
            //pIdentity->GetProcessorArchitecture();
            //pIdentity->GetPublicKeyToken();
            //pIdentity->GetType();
            //pIdentity->GetPublicLanguage();
            //pIdentity->GetVersion();
            _tcscpy(ImportedDllFullPath,pIdentity->GetDependencyFullPathWithBackSlash());
            _tcscat(ImportedDllFullPath,tDllName);
            
            if (CStdFileOperations::DoesFileExists(ImportedDllFullPath))
            {
                _tcscpy(tFullPath,ImportedDllFullPath);
                return TRUE;
            }
            // else


            //-----------------------------------------------------------
            // < user's language-culture>
            // Side-by-side searches WinSxS 
            // with language-culture\moduleName.dll
            // with language-culture\moduleName\moduleName.dll

            // < user's language>
            // Side-by-side searches WinSxS 
            // with language\moduleName.dll
            // with language\moduleName\moduleName.dll

            // < system's language-culture>
            //Side-by-side searches WinSxS for the system's language-culture version.
            // with en-us\moduleName.dll
            // with en-us\moduleName\moduleName.dll

            // < system's language>
            //Side-by-side searches WinSxS for the system's language version.
            // with en\moduleName.dll
            // with en\moduleName\moduleName.dll

            // < no language >
            //Side-by-side searches WinSxS for the no language version.
            // with \moduleName.dll
            // with \moduleName\moduleName.dll
            //--------------------------------------------------

            // with language-culture
            TCHAR* Language = _tcsdup(pIdentity->GetPublicLanguage());
            TCHAR* Tmp;

            if (*Language)
            {

                // with language-culture\moduleName.dll
                _tcscpy(ImportedDllFullPath,ImportingModuleDirectory);
                _tcscat(ImportedDllFullPath,Language);
                _tcscat(ImportedDllFullPath,_T("\\"));
                _tcscat(ImportedDllFullPath,tDllName);
                if (CStdFileOperations::DoesFileExists(ImportedDllFullPath))
                {
                    _tcscpy(tFullPath,ImportedDllFullPath);
                    return TRUE;
                }

                // with language-culture\moduleName\moduleName.dll
                _tcscpy(ImportedDllFullPath,ImportingModuleDirectory);
                _tcscat(ImportedDllFullPath,Language);
                _tcscat(ImportedDllFullPath,_T("\\"));
                _tcscat(ImportedDllFullPath,ModuleNameWithoutExt);
                _tcscat(ImportedDllFullPath,_T("\\"));
                _tcscat(ImportedDllFullPath,tDllName);
                if (CStdFileOperations::DoesFileExists(ImportedDllFullPath))
                {
                    _tcscpy(tFullPath,ImportedDllFullPath);
                    return TRUE;
                }

                // with language only
                Tmp=_tcschr(Language,'-');
                if(Tmp)
                    *Tmp=0;

// FIX ME XXX to find
//// create winsxs path
//_sntprintf(ImportedDllFullPath,
//            MAX_PATH,
//            _T("%s\\winsxs\\%s_%s_%s_%s_%s_%s\\"),
//            WindowsDir,
//            pIdentity->GetProcessorArchitecture(),
//            pIdentity->GetName(),
//            pIdentity->GetPublicKeyToken(),
//            pIdentity->GetVersion(),
//            Language,
//            XXX
//            );

                // create winsxs path
                _sntprintf(ImportedDllFullPath,
                            MAX_PATH,
                            _T("%s\\winsxs\\%s_%s_%s_%s_%s_%s"),
                            WindowsDir,
                            pIdentity->GetProcessorArchitecture(),
                            pIdentity->GetName(),
                            pIdentity->GetPublicKeyToken(),
                            pIdentity->GetVersion(),
                            Language,
                            _T("*")
                            );


                WIN32_FIND_DATA DirectoryInfos;
                HANDLE hFind = ::FindFirstFile(ImportedDllFullPath,&DirectoryInfos);
                if ( hFind != INVALID_HANDLE_VALUE)
                {
                    ::FindClose(hFind);

                    _sntprintf(ImportedDllFullPath,
                                MAX_PATH,
                                _T("%s\\winsxs\\%s\\"),
                                WindowsDir,
                                DirectoryInfos.cFileName
                                );
// end of FIX ME XXX to find
                    _tcscat(ImportedDllFullPath,tDllName);
                    if (CStdFileOperations::DoesFileExists(ImportedDllFullPath))
                    {
                        _tcscpy(tFullPath,ImportedDllFullPath);
                        return TRUE;
                    }
                }


                // with language\moduleName.dll
                _tcscpy(ImportedDllFullPath,ImportingModuleDirectory);
                _tcscat(ImportedDllFullPath,Language);
                _tcscat(ImportedDllFullPath,_T("\\"));
                _tcscat(ImportedDllFullPath,tDllName);
                if (CStdFileOperations::DoesFileExists(ImportedDllFullPath))
                {
                    _tcscpy(tFullPath,ImportedDllFullPath);
                    return TRUE;
                }

                // with language\moduleName\moduleName.dll
                _tcscpy(ImportedDllFullPath,ImportingModuleDirectory);
                _tcscat(ImportedDllFullPath,Language);
                _tcscat(ImportedDllFullPath,_T("\\"));
                _tcscat(ImportedDllFullPath,ModuleNameWithoutExt);
                _tcscat(ImportedDllFullPath,_T("\\"));
                _tcscat(ImportedDllFullPath,tDllName);
                if (CStdFileOperations::DoesFileExists(ImportedDllFullPath))
                {
                    _tcscpy(tFullPath,ImportedDllFullPath);
                    return TRUE;
                }
            }

            // with \moduleName\moduleName.dll
            _tcscpy(ImportedDllFullPath,ImportingModuleDirectory);
            _tcscat(ImportedDllFullPath,ModuleNameWithoutExt);
            _tcscat(ImportedDllFullPath,_T("\\"));
            _tcscat(ImportedDllFullPath,tDllName);
            if (CStdFileOperations::DoesFileExists(ImportedDllFullPath))
            {
                _tcscpy(tFullPath,ImportedDllFullPath);
                return TRUE;
            }

            pIdentity = PeManifest.GetNextDependency();
        }
    }
    return FALSE;
}