#include <windows.h>
//ʹ��Windows��HeapAlloc�������ж�̬�ڴ����
#define myheapalloc(x) (HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, x))
#define myheapfree(x)  (HeapFree(GetProcessHeap(), 0, x))

typedef BOOL (WINAPI *SetSecurityDescriptorControlFnPtr)(
   IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
   IN SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
   IN SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet);

typedef BOOL (WINAPI *AddAccessAllowedAceExFnPtr)(
  PACL pAcl,
  DWORD dwAceRevision,
  DWORD AceFlags,
  DWORD AccessMask,
  PSID pSid
);

BOOL AddAccessRights(TCHAR *lpszFileName, TCHAR *lpszAccountName, 
      DWORD dwAccessMask) {

   // ����SID����
   SID_NAME_USE   snuType;

   // ������LookupAccountName��صı�����ע�⣬ȫΪ0��Ҫ�ڳ����ж�̬���䣩
   TCHAR *        szDomain       = NULL;
   DWORD          cbDomain       = 0;
   LPVOID         pUserSID       = NULL;
   DWORD          cbUserSID      = 0;

   // ���ļ���صİ�ȫ������ SD �ı���
   PSECURITY_DESCRIPTOR pFileSD  = NULL;     // �ṹ����
   DWORD          cbFileSD       = 0;        // SD��size

   // һ���µ�SD�ı��������ڹ����µ�ACL�������е�ACL����Ҫ�¼ӵ�ACL����������
   SECURITY_DESCRIPTOR  newSD;

   // ��ACL ��صı���
   PACL           pACL           = NULL;
   BOOL           fDaclPresent;
   BOOL           fDaclDefaulted;
   ACL_SIZE_INFORMATION AclInfo;

   // һ���µ� ACL ����
   PACL           pNewACL        = NULL;  //�ṹָ�����
   DWORD          cbNewACL       = 0;     //ACL��size

   // һ����ʱʹ�õ� ACE ����
   LPVOID         pTempAce       = NULL;
   UINT           CurrentAceIndex = 0;  //ACE��ACL�е�λ��

   UINT           newAceIndex = 0;  //�����ACE��ACL�е�λ��

   //API�����ķ���ֵ���������еĺ���������ʧ�ܡ�
   BOOL           fResult;
   BOOL           fAPISuccess;

   SECURITY_INFORMATION secInfo = DACL_SECURITY_INFORMATION;

   // ����������������µ�API����������Windows 2000���ϰ汾�Ĳ���ϵͳ֧�֡� 
   // �ڴ˽���Advapi32.dll�ļ��ж�̬���롣�����ʹ��VC++ 6.0������򣬶�������
   // ʹ�������������ľ�̬���ӡ�����Ϊ��ı�����ϣ�/D_WIN32_WINNT=0x0500
   // �ı������������ȷ�����SDK��ͷ�ļ���lib�ļ������µġ�
   SetSecurityDescriptorControlFnPtr _SetSecurityDescriptorControl = NULL;
   AddAccessAllowedAceExFnPtr _AddAccessAllowedAceEx = NULL; 

   __try {

      // 
      // STEP 1: ͨ���û���ȡ��SID
      //     ����һ����LookupAccountName���������������Σ���һ����ȡ������Ҫ
      // ���ڴ�Ĵ�С��Ȼ�󣬽����ڴ���䡣�ڶ��ε��ò���ȡ�����û����ʻ���Ϣ��
      // LookupAccountNameͬ������ȡ�����û������û������Ϣ������ο�MSDN��
      //

      fAPISuccess = LookupAccountName(NULL, lpszAccountName,
            pUserSID, &cbUserSID, szDomain, &cbDomain, &snuType);

      // ���ϵ���API��ʧ�ܣ�ʧ��ԭ�����ڴ治�㡣��������Ҫ���ڴ��С������
      // �����Ǵ�����ڴ治��Ĵ���

      if (fAPISuccess)
         __leave;
      else if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
//          _tprintf(TEXT("LookupAccountName() failed. Error %d\n"), 
//                GetLastError());
         __leave;
      }

      pUserSID = myheapalloc(cbUserSID);
      if (!pUserSID) {
//         _tprintf(TEXT("HeapAlloc() failed. Error %d\n"), GetLastError());
         __leave;
      }

      szDomain = (TCHAR *) myheapalloc(cbDomain * sizeof(TCHAR));
      if (!szDomain) {
 //        _tprintf(TEXT("HeapAlloc() failed. Error %d\n"), GetLastError());
         __leave;
      }

      fAPISuccess = LookupAccountName(NULL, lpszAccountName,
            pUserSID, &cbUserSID, szDomain, &cbDomain, &snuType);
      if (!fAPISuccess) {
  //       _tprintf(TEXT("LookupAccountName() failed. Error %d\n"), 
        //       GetLastError());
         __leave;
      }

      // 
      // STEP 2: ȡ���ļ���Ŀ¼����صİ�ȫ������SD
      //     ʹ��GetFileSecurity����ȡ��һ���ļ�SD�Ŀ�����ͬ�����������Ҳ
       // �Ǳ��������Σ���һ��ͬ����ȡSD���ڴ泤�ȡ�ע�⣬SD�����ָ�ʽ������ص�
       // ��self-relative���� ��ȫ�ģ�absolute����GetFileSecurityֻ��ȡ������
       // ��صġ�����SetFileSecurity����Ҫ��ȫ�ġ������Ϊʲô��Ҫһ���µ�SD��
       // ������ֱ����GetFileSecurity���ص�SD�Ͻ����޸ġ���Ϊ������صġ���Ϣ
       // �ǲ������ġ�

      fAPISuccess = GetFileSecurity(lpszFileName, 
            secInfo, pFileSD, 0, &cbFileSD);

      // ���ϵ���API��ʧ�ܣ�ʧ��ԭ�����ڴ治�㡣��������Ҫ���ڴ��С������
      // �����Ǵ�����ڴ治��Ĵ���
      if (fAPISuccess)
         __leave;
      else if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
  //       _tprintf(TEXT("GetFileSecurity() failed. Error %d\n"), 
       //        GetLastError());
         __leave;
      }

      pFileSD = myheapalloc(cbFileSD);
      if (!pFileSD) {
 //        _tprintf(TEXT("HeapAlloc() failed. Error %d\n"), GetLastError());
         __leave;
      }

      fAPISuccess = GetFileSecurity(lpszFileName, 
            secInfo, pFileSD, cbFileSD, &cbFileSD);
      if (!fAPISuccess) {
    //     _tprintf(TEXT("GetFileSecurity() failed. Error %d\n"), 
     //          GetLastError());
         __leave;
      }

      // 
      // STEP 3: ��ʼ��һ���µ�SD
      // 
      if (!InitializeSecurityDescriptor(&newSD, 
            SECURITY_DESCRIPTOR_REVISION)) {
     //    _tprintf(TEXT("InitializeSecurityDescriptor() failed.")
       //     TEXT("Error %d\n"), GetLastError());
         __leave;
      }

      // 
      // STEP 4: ��GetFileSecurity ���ص�SD��ȡDACL
      // 
      if (!GetSecurityDescriptorDacl(pFileSD, &fDaclPresent, &pACL,
            &fDaclDefaulted)) {
       //  _tprintf(TEXT("GetSecurityDescriptorDacl() failed. Error %d\n"),
       //        GetLastError());
         __leave;
      }

      // 
      // STEP 5: ȡ DACL���ڴ�size
      //     GetAclInformation�����ṩDACL���ڴ��С��ֻ����һ������Ϊ
      // ACL_SIZE_INFORMATION��structure�Ĳ�������DACL����Ϣ����Ϊ��
      // �������Ǳ������е�ACE��
      AclInfo.AceCount = 0; // Assume NULL DACL.
      AclInfo.AclBytesFree = 0;
      AclInfo.AclBytesInUse = sizeof(ACL);

      if (pACL == NULL)
         fDaclPresent = FALSE;

      // ���DACL��Ϊ�գ���ȡ����Ϣ�������������¡��Թ�������DACLΪ�գ�
      if (fDaclPresent) {            
         if (!GetAclInformation(pACL, &AclInfo, 
               sizeof(ACL_SIZE_INFORMATION), AclSizeInformation)) {
          //  _tprintf(TEXT("GetAclInformation() failed. Error %d\n"),
          //        GetLastError());
            __leave;
         }
      }

      // 
      // STEP 6: �����µ�ACL��size
      //    ����Ĺ�ʽ�ǣ�ԭ�е�DACL��size������Ҫ��ӵ�һ��ACE��size����
      // ������һ����ACE��ص�SID��size������ȥ�����ֽ��Ի�þ�ȷ�Ĵ�С��
      cbNewACL = AclInfo.AclBytesInUse + sizeof(ACCESS_ALLOWED_ACE) 
            + GetLengthSid(pUserSID) - sizeof(DWORD);


      // 
      // STEP 7: Ϊ�µ�ACL�����ڴ�
      // 
      pNewACL = (PACL) myheapalloc(cbNewACL);
      if (!pNewACL) {
        // _tprintf(TEXT("HeapAlloc() failed. Error %d\n"), GetLastError());
       //  __leave;
      }

      // 
      // STEP 8: ��ʼ���µ�ACL�ṹ
      // 
      if (!InitializeAcl(pNewACL, cbNewACL, ACL_REVISION2)) {
        // _tprintf(TEXT("InitializeAcl() failed. Error %d\n"), 
        //       GetLastError());
         __leave;
      }

      // 
      // STEP 9  ����ļ���Ŀ¼�� DACL �����ݣ��������е�ACE���µ�DACL��
      // 
      //     ����Ĵ���������ȼ��ָ���ļ���Ŀ¼���Ƿ���ڵ�DACL������еĻ���
      // ��ô�Ϳ������е�ACE���µ�DACL�ṹ�У����ǿ��Կ���������ķ����ǲ���
      // ACL_SIZE_INFORMATION�ṹ�е�AceCount��Ա����ɵġ������ѭ���У�
      // �ᰴ��Ĭ�ϵ�ACE��˳�������п�����ACE��ACL�е�˳���Ǻܹؼ��ģ����ڿ�
      // �������У��ȿ����Ǽ̳е�ACE������֪��ACE����ϲ�Ŀ¼�м̳�������
      // 

      newAceIndex = 0;

      if (fDaclPresent && AclInfo.AceCount) {

         for (CurrentAceIndex = 0; 
               CurrentAceIndex < AclInfo.AceCount;
               CurrentAceIndex++) {

            // 
            // STEP 10: ��DACL��ȡACE
            // 
            if (!GetAce(pACL, CurrentAceIndex, &pTempAce)) {
              // _tprintf(TEXT("GetAce() failed. Error %d\n"), 
              //       GetLastError());
               __leave;
            }

            // 
            // STEP 11: ����Ƿ��ǷǼ̳е�ACE
            //     �����ǰ��ACE��һ���Ӹ�Ŀ¼�̳�����ACE����ô���˳�ѭ����
            // ��Ϊ���̳е�ACE�����ڷǼ̳е�ACE֮�󣬶�������Ҫ��ӵ�ACE
            // Ӧ�������еķǼ̳е�ACE֮�����еļ̳е�ACE֮ǰ���˳�ѭ��
            // ����Ϊ��Ҫ���һ���µ�ACE���µ�DACL�У���������ٰѼ̳е�
            // ACE�������µ�DACL�С�
            //
            if (((ACCESS_ALLOWED_ACE *)pTempAce)->Header.AceFlags
               & INHERITED_ACE)
               break;

            // 
            // STEP 12: ���Ҫ������ACE��SID�Ƿ����Ҫ�����ACE��SIDһ����
            // ���һ������ô��Ӧ�÷ϵ��Ѵ��ڵ�ACE��Ҳ����˵��ͬһ���û��Ĵ�ȡ
            // Ȩ�޵����õ�ACE����DACL��Ӧ��Ψһ�������������ͬһ�û�������
            // �˵�ACE�����ǿ��������û���ACE��
            // 
            if (EqualSid(pUserSID,
               &(((ACCESS_ALLOWED_ACE *)pTempAce)->SidStart)))
               continue;

            // 
            // STEP 13: ��ACE���뵽�µ�DACL��
            //    ����Ĵ����У�ע�� AddAce �����ĵ����������������������˼�� 
            // ACL�е�����ֵ����ΪҪ��ACE�ӵ�ĳ����λ��֮�󣬲���MAXDWORD��
              // ��˼��ȷ����ǰ��ACE�Ǳ����뵽����λ�á�
            //
            if (!AddAce(pNewACL, ACL_REVISION, MAXDWORD, pTempAce,
                  ((PACE_HEADER) pTempAce)->AceSize)) {
              // _tprintf(TEXT("AddAce() failed. Error %d\n"), 
               //      GetLastError());
               __leave;
            }

            newAceIndex++;
         }
      }

// 
    // STEP 14: ��һ�� access-allowed ��ACE ���뵽�µ�DACL��
    //     ǰ���ѭ�����������еķǼ̳���SIDΪ�����û���ACE���˳�ѭ���ĵ�һ����
    // ���Ǽ�������ָ����ACE����ע�������ȶ�̬װ����һ��AddAccessAllowedAceEx
    // ��API���������װ�ز��ɹ����͵���AddAccessAllowedAce������ǰһ��������
    // ��Windows 2000�Ժ�İ汾֧�֣�NT��û�У�����Ϊ��ʹ���°汾�ĺ�����������
    // ���ȼ��һ�µ�ǰϵͳ�пɲ�����װ���������������������ʹ�á�ʹ�ö�̬����
    // ��ʹ�þ�̬���ӵĺô��ǣ���������ʱ������Ϊû�����API����������
    // 
    // Ex��ĺ��������һ������AceFlag�������˲�������������������ǿ���������һ
    // ����ACE_HEADER�Ľṹ���Ա������������õ�ACE���Ա�����Ŀ¼���̳���ȥ���� 
    // AddAccessAllowedAce�������ܶ��������������AddAccessAllowedAce����
    // �У�����ACE_HEADER����ṹ���óɷǼ̳еġ�
    // 
      _AddAccessAllowedAceEx = (AddAccessAllowedAceExFnPtr)
            GetProcAddress(GetModuleHandle(TEXT("advapi32.dll")),
            "AddAccessAllowedAceEx");

      if (_AddAccessAllowedAceEx) {
           if (!_AddAccessAllowedAceEx(pNewACL, ACL_REVISION2,
              CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE ,
                dwAccessMask, pUserSID)) {
            // _tprintf(TEXT("AddAccessAllowedAceEx() failed. Error %d\n"),
            //       GetLastError());
             __leave;
          }
      }else{
          if (!AddAccessAllowedAce(pNewACL, ACL_REVISION2, 
                dwAccessMask, pUserSID)) {
//              _tprintf(TEXT("AddAccessAllowedAce() failed. Error %d\n"),
//                    GetLastError());
             __leave;
          }
      }

      // 
      // STEP 15: �����Ѵ��ڵ�ACE��˳�򿽱��Ӹ�Ŀ¼�̳ж�����ACE
      // 
      if (fDaclPresent && AclInfo.AceCount) {

         for (; 
              CurrentAceIndex < AclInfo.AceCount;
              CurrentAceIndex++) {

            // 
            // STEP 16: ���ļ���Ŀ¼����DACL�м���ȡACE
            // 
            if (!GetAce(pACL, CurrentAceIndex, &pTempAce)) {
//                _tprintf(TEXT("GetAce() failed. Error %d\n"), 
//                      GetLastError());
               __leave;
            }

            // 
            // STEP 17: ��ACE���뵽�µ�DACL��
            // 
            if (!AddAce(pNewACL, ACL_REVISION, MAXDWORD, pTempAce,
                  ((PACE_HEADER) pTempAce)->AceSize)) {
//                _tprintf(TEXT("AddAce() failed. Error %d\n"), 
//                      GetLastError());
               __leave;
            }
         }
      }

      // 
      // STEP 18: ���µ�ACL���õ��µ�SD��
      // 
      if (!SetSecurityDescriptorDacl(&newSD, TRUE, pNewACL, 
            FALSE)) {
//          _tprintf(TEXT("SetSecurityDescriptorDacl() failed. Error %d\n"),
//                GetLastError());
         __leave;
      }

      // 
      // STEP 19: ���ϵ�SD�еĿ��Ʊ���ٿ������µ�SD�У�����ʹ�õ���һ���� 
      // SetSecurityDescriptorControl() ��API�������������ͬ��ֻ������
      // Windows 2000�Ժ�İ汾�У��������ǻ���Ҫ��̬�ذ����advapi32.dll 
      // �����룬���ϵͳ��֧������������ǾͲ������ϵ�SD�Ŀ��Ʊ���ˡ�
      // 
      _SetSecurityDescriptorControl =(SetSecurityDescriptorControlFnPtr)
            GetProcAddress(GetModuleHandle(TEXT("advapi32.dll")),
            "SetSecurityDescriptorControl");
      if (_SetSecurityDescriptorControl) {

         SECURITY_DESCRIPTOR_CONTROL controlBitsOfInterest = 0;
         SECURITY_DESCRIPTOR_CONTROL controlBitsToSet = 0;
         SECURITY_DESCRIPTOR_CONTROL oldControlBits = 0;
         DWORD dwRevision = 0;

         if (!GetSecurityDescriptorControl(pFileSD, &oldControlBits,
            &dwRevision)) {
//             _tprintf(TEXT("GetSecurityDescriptorControl() failed.")
//                   TEXT("Error %d\n"), GetLastError());
            __leave;
         }

         if (oldControlBits & SE_DACL_AUTO_INHERITED) {
            controlBitsOfInterest =
               SE_DACL_AUTO_INHERIT_REQ |
               SE_DACL_AUTO_INHERITED ;
            controlBitsToSet = controlBitsOfInterest;
         }
         else if (oldControlBits & SE_DACL_PROTECTED) {
            controlBitsOfInterest = SE_DACL_PROTECTED;
            controlBitsToSet = controlBitsOfInterest;
         }        

         if (controlBitsOfInterest) {
            if (!_SetSecurityDescriptorControl(&newSD,
               controlBitsOfInterest,
               controlBitsToSet)) {
//                _tprintf(TEXT("SetSecurityDescriptorControl() failed.")
//                      TEXT("Error %d\n"), GetLastError());
               __leave;
            }
         }
      }

      // 
      // STEP 20: ���µ�SD�������õ��ļ��İ�ȫ�����У�ǧɽ��ˮ�������ڵ��ˣ�
      // 
      if (!SetFileSecurity(lpszFileName, secInfo,
            &newSD)) {
//          _tprintf(TEXT("SetFileSecurity() failed. Error %d\n"), 
//                GetLastError());
         __leave;
      }

      fResult = TRUE;

   } __finally {

      // 
      // STEP 21: �ͷ��ѷ�����ڴ棬����Memory Leak
      // 
      if (pUserSID)  myheapfree(pUserSID);
      if (szDomain)  myheapfree(szDomain);
      if (pFileSD) myheapfree(pFileSD);
      if (pNewACL) myheapfree(pNewACL);
   }

   return fResult;
}
