// NameSort.h
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
//由qsort.c改造

#pragma once

#pragma optimize("t", on)

#include "Index.h"

__forceinline void _swap(PIndexElemType a,PIndexElemType b)
{ 
    IndexElemType tmp;
    tmp = *a;
    *a = *b;
    *b = tmp;     
}

#define INSERT_SORT
//#define SELECT_SORT

__forceinline void shortsort(PIndexElemType lo,PIndexElemType hi,int (*comp)(IndexElemType,IndexElemType))
#ifdef INSERT_SORT  //插入排序
{
    PIndexElemType pBase,p;
    IndexElemType tmp;
    for(pBase=lo+1;pBase<=hi;++pBase){
        tmp=*pBase;
        for(p=pBase-1;p>=lo && comp(*p,tmp)>0;--p){
            *(p+1)=*p;
        }
        *(p+1)=tmp;
    }
}
#elif defined(SELECT_SORT)   //选择排序
{
    DWORD *p, *max;
    for(;hi>lo;--hi){
        max=lo;
        for(p =lo+1;p<= hi;++p){
            if(comp(*p,*max)>0){
                max=p;
            }
        }
        _swap(max,hi);
    }
}
#else
{

}
#endif


#define CUTOFF 32
#define STKSIZ 30 //(8*sizeof(void*) - 2)


inline void name_qsort(PIndexElemType base,DWORD num,int (*comp)(IndexElemType,IndexElemType))
{
    /* Note: the number of stack entries required is no more than
    1 + log2(num), so 30 is sufficient for any array */
    PIndexElemType lo=base, hi=base+num-1;         /* ends of sub-array currently sorting */
    PIndexElemType mid;                             /* points to middle of subarray */
    PIndexElemType loguy,higuy;                   /* traveling pointers for partition step */
    DWORD size;                             /* size of the sub-array */
    PIndexElemType lostk[STKSIZ], histk[STKSIZ];
    int stkptr;                             /* stack for saving sub-array to be processed */

    if (num < 2) return;                
        
    stkptr = 0;                             /* initialize stack */

    /* this entry point is for pseudo-recursion calling: setting
    lo and hi and jumping to here is like recursion, but stkptr is
    preserved, locals aren't, so we preserve stuff on the stack */
recurse:

    size = hi-lo+1;        /* number of el's to sort */

    /* below a certain size, it is faster to use a O(n^2) sorting method */
    if (size <= CUTOFF) {
        shortsort(lo, hi,comp);
    }
    else {
        /* First we pick a partitioning element.  The efficiency of the
        algorithm demands that we find one that is approximately the median
        of the values, but also that we select one fast.  We choose the
        median of the first, middle, and last elements, to avoid bad
        performance in the face of already sorted data, or data that is made
        up of multiple sorted runs appended together.  Testing shows that a
        median-of-three algorithm provides better performance than simply
        picking the middle element for the latter case. */

        mid = lo + (size>>1);      /* find middle element */

        /* Sort the first, middle, last elements into order */
        if (comp(*lo, *mid) > 0) {
            _swap(lo, mid);
        }
        if (comp(*lo, *hi) > 0) {
            _swap(lo, hi);
        }
        if (comp(*mid, *hi) > 0) {
            _swap(mid, hi);
        }

        /* We now wish to partition the array into three pieces, one consisting
        of elements <= partition element, one of elements equal to the
        partition element, and one of elements > than it.  This is done
        below; comments indicate conditions established at every step. */

        loguy = lo;
        higuy = hi;

        /* Note that higuy decreases and loguy increases on every iteration,
        so loop must terminate. */
        for (;;) {
            /* lo <= loguy < hi, lo < higuy <= hi,
            A[i] <= A[mid] for lo <= i <= loguy,
            A[i] > A[mid] for higuy <= i < hi,
            A[hi] >= A[mid] */

            /* The doubled loop is to avoid calling comp(mid,mid), since some
            existing comparison funcs don't work when passed the same
            value for both pointers. */

            if (mid > loguy) {
                do  {
                    ++loguy;
                } while (loguy < mid && comp(*loguy, *mid) <= 0);
            }
            if (mid <= loguy) {
                do  {
                    ++loguy;
                } while (loguy <= hi && comp(*loguy, *mid) <= 0);
            }

            /* lo < loguy <= hi+1, A[i] <= A[mid] for lo <= i < loguy,
            either loguy > hi or A[loguy] > A[mid] */

            do  {
                --higuy;
            } while (higuy > mid && comp(*higuy, *mid) > 0);

            /* lo <= higuy < hi, A[i] > A[mid] for higuy < i < hi,
            either higuy == lo or A[higuy] <= A[mid] */

            if (higuy < loguy)
                break;

            /* if loguy > hi or higuy == lo, then we would have exited, so
            A[loguy] > A[mid], A[higuy] <= A[mid],
            loguy <= hi, higuy > lo */

            _swap(loguy, higuy);

            /* If the partition element was moved, follow it.  Only need
            to check for mid == higuy, since before the swap,
            A[loguy] > A[mid] implies loguy != mid. */

            if (mid == higuy)
                mid = loguy;

            /* A[loguy] <= A[mid], A[higuy] > A[mid]; so condition at top
            of loop is re-established */
        }

        /*     A[i] <= A[mid] for lo <= i < loguy,
        A[i] > A[mid] for higuy < i < hi,
        A[hi] >= A[mid]
        higuy < loguy
        implying:
        higuy == loguy-1
        or higuy == hi - 1, loguy == hi + 1, A[hi] == A[mid] */

        /* Find adjacent elements equal to the partition element.  The
        doubled loop is to avoid calling comp(mid,mid), since some
        existing comparison funcs don't work when passed the same value
        for both pointers. */

        ++higuy;
        if (mid < higuy) {
            do  {
                --higuy;
            } while (higuy > mid && comp(*higuy, *mid) == 0);
        }
        if (mid >= higuy) {
            do  {
                --higuy;
            } while (higuy > lo && comp(*higuy, *mid) == 0);
        }

        /* OK, now we have the following:
        higuy < loguy
        lo <= higuy <= hi
        A[i]  <= A[mid] for lo <= i <= higuy
        A[i]  == A[mid] for higuy < i < loguy
        A[i]  >  A[mid] for loguy <= i < hi
        A[hi] >= A[mid] */

        /* We've finished the partition, now we want to sort the subarrays
        [lo, higuy] and [loguy, hi].
        We do the smaller one first to minimize stack usage.
        We only sort arrays of length 2 or more.*/

        if ( higuy - lo >= hi - loguy ) {
            if (lo < higuy) {
                lostk[stkptr] = lo;
                histk[stkptr] = higuy;
                ++stkptr;
            }                           /* save big recursion for later */

            if (loguy < hi) {
                lo = loguy;
                goto recurse;           /* do small recursion */
            }
        }
        else {
            if (loguy < hi) {
                lostk[stkptr] = loguy;
                histk[stkptr] = hi;
                ++stkptr;               /* save big recursion for later */
            }

            if (lo < higuy) {
                hi = higuy;
                goto recurse;           /* do small recursion */
            }
        }
    }

    /* We have sorted the array, except for any pending sorts on the stack.
    Check if there are any, and do them. */

    --stkptr;
    if (stkptr >= 0) {
        lo = lostk[stkptr];
        hi = histk[stkptr];
        goto recurse;           /* pop subarray from stack */
    }
    else
        return;                 /* all subarrays done */
}



typedef int (*PComp)(PBYTE,PBYTE const,PBYTE,PBYTE const);
extern PComp comp_name;//文件名比较函数
BOOL Help_InitCompare();
int comp_dir(IndexElemType ,IndexElemType );
int comp_file(IndexElemType ,IndexElemType );