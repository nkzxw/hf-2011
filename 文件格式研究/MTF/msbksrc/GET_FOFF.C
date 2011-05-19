/* get_foff.c 3/30/05 developed in ffndstr.c then moved here.
   see tst_foff.c for a driver routine, plan use as generic
   module for HAS_INT64 code in dumpit and ntbkup.c
   Always display in hex wwith the leading 0x
   Always look for leading 'h' in string for hex.
   if there do as hex and check for HAS_INT64, otherwise only 
   treat as a long.

   The MSVC runtime library docs say printf (and presumably scanf)
   accepts I64 for int64 as well as l for long in format spec

   gcc's libc C seems to have no such built in support so just
   truncate quadword decimal input for these.
*/
disp_foffset(FOFFSET pos)
{
   unsigned long *dw = (unsigned long *)&pos;
   printf("0x");
#ifdef HAS_INT64
   if(*(dw+1) > 0)
      printf("%lx",*(dw+1));  // print high order portion
#endif
   printf("%lx",*dw);
   
}

FOFFSET get_foffset(char *str,FOFFSET *pos)
{
    int suc=0,cnt=0,i,nul;
    char *num=str, ch = toupper(*str);
    *pos = 0; // clear it, may only set low bits below
    if(ch == 'H')
    {
        str++; // skip initial q
        while(*(str+cnt) != 0)
        {
            ch = toupper(*(str+cnt)); 
            if((ch >= '0' && ch <= '9') ||
               (ch >= 'A' && ch <= 'F') )
                cnt++;
            else
                break;
        }
//      if(cnt > 16 just truncate like sscanf(%lx) does
        if(cnt <= 8)
            i = 0;
        else
            i = cnt - 8; 
        suc = sscanf(str+i,"%lx",(unsigned long *)pos); // set low order DWORD

        cnt = i; // chars remaining
        if(cnt && suc) // more than 8 bytes in input string
        {
#ifndef HAS_INT64
        printf("\nWarning, input truncated to 32 bits\n");
#else           
           nul = i;
           ch = *(str+nul);
           *(str+nul) = 0; // temporarily terminate
           if(cnt <= 8)
              i = 0;
           else
              i = cnt -8;
           suc = sscanf(str+i,"%lx",(unsigned long *)pos+1); // set high order DWORD
           *(str+nul) = ch; // restore str
           if(i & suc)
              printf("\nWarning, input truncated to 64 bits\n");
#endif
        }
    }
    else
    {
#if defined(HAS_INT64) && defined(_WIN32)
       suc = sscanf(str,"%I64u",pos);  // unsigned long long
#else // truncate to 32 bits
       suc = sscanf(str,"%lu",(unsigned long *)pos);// unsigned long
#endif
    }
    if(suc == 0)
    {
       printf("\nfailed to parse string: %s",num);
       exit(-1);
    }
    else
       return(*pos);
}

