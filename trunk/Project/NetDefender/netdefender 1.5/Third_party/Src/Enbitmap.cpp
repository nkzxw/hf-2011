// EnBitmap.cpp: implementation of the CEnBitmap class (c) daniel godson 2002.
//
// credits: Peter Hendrix's CPicture implementation for the original IPicture code
//          Yves Maurer's GDIRotate implementation for the idea of working directly on 32 bit representations of bitmaps
//          Karl Lager's 'A Fast Algorithm for Rotating Bitmaps'
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "EnBitmap.h"
#include "color.h"

#include <afxpriv.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

///////////////////////////////////////////////////////////////////////
C32BitImageProcessor::C32BitImageProcessor(BOOL bEnableWeighting) : m_bWeightingEnabled(bEnableWeighting)
{
	m_bBk = FALSE;
}

void C32BitImageProcessor::SetBkColor(int R, int G, int B)
{
	m_bBk = TRUE;
	m_nRed = R;
	m_nGreen = G;
	m_nBlue = B;
}

C32BitImageProcessor::~C32BitImageProcessor()
{
}

CSize C32BitImageProcessor::CalcDestSize(CSize sizeSrc)
{
	return(sizeSrc);	// default
}

BOOL C32BitImageProcessor::IsBkColor(RGBX *p)
{
	if(!m_bBk)
	{
		return(FALSE);
	}

	if(p->btRed == m_nRed && p->btGreen == m_nGreen && p->btBlue == m_nBlue)
	{
		return(TRUE);
	}

	return(FALSE);
}

BOOL C32BitImageProcessor::ProcessPixels(RGBX *pSrcPixels, CSize, RGBX *pDestPixels, CSize sizeDest)
{
	CopyMemory(pDestPixels, pSrcPixels, sizeDest.cx * 4 * sizeDest.cy); // default
	return(TRUE);
}

// C32BitImageProcessor::CalcWeightedColor(...) is inlined in EnBitmap.h

//////////////////////////////////////////////////////////////////////

// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CEnBitmap::CEnBitmap(COLORREF crBkgnd) : m_crBkgnd(crBkgnd)
{
}

CEnBitmap::~CEnBitmap()
{
}

BOOL CEnBitmap::GrayImage()
{
	CImageGrayer	gr;
	return(ProcessImage(&gr));
}

BOOL CEnBitmap::ProcessImage(C32BitImageProcessor *pProcessor)
{
	C32BIPArray aProcessors;

	aProcessors.Add(pProcessor);

	return(ProcessImage(aProcessors));
}

BOOL CEnBitmap::ProcessImage(C32BIPArray &aProcessors)
{
	ASSERT(GetSafeHandle());

	if(!GetSafeHandle())
	{
		return(FALSE);
	}

	if(!aProcessors.GetSize())
	{
		return(TRUE);
	}

	int		nProcessor, nCount = aProcessors.GetSize();

	// retrieve src and final dest sizes
	BITMAP	BM;

	if(!GetBitmap(&BM))
	{
		return(FALSE);
	}

	CSize	sizeSrc(BM.bmWidth, BM.bmHeight);
	CSize sizeDest(sizeSrc), sizeMax(sizeSrc);

	for(nProcessor = 0; nProcessor < nCount; nProcessor++)
	{
		sizeDest = aProcessors[nProcessor]->CalcDestSize(sizeDest);
		sizeMax = CSize(max(sizeMax.cx, sizeDest.cx), max(sizeMax.cy, sizeDest.cy));
	}

	// prepare src and dest bits
	RGBX	*pSrcPixels = GetDIBits32();

	if(!pSrcPixels)
	{
		return(FALSE);
	}

	RGBX	*pDestPixels = new RGBX[sizeMax.cx * sizeMax.cy];

	if(!pDestPixels)
	{
		return(FALSE);
	}

	Fill(pDestPixels, sizeMax, m_crBkgnd);

	BOOL	bRes = TRUE;
	sizeDest = sizeSrc;

	// do the processing
	for(nProcessor = 0; bRes && nProcessor < nCount; nProcessor++)
	{
		// if its the second processor or later then we need to copy
		// the previous dest bits back into source.
		// we also need to check that sizeSrc is big enough
		if(nProcessor > 0)
		{
			if(sizeSrc.cx < sizeDest.cx || sizeSrc.cy < sizeDest.cy)
			{
				delete[] pSrcPixels;
				pSrcPixels = new RGBX[sizeDest.cx * sizeDest.cy];
			}

			CopyMemory(pSrcPixels, pDestPixels, sizeDest.cx * 4 * sizeDest.cy); // default
			Fill(pDestPixels, sizeDest, m_crBkgnd);
		}

		sizeSrc = sizeDest;
		sizeDest = aProcessors[nProcessor]->CalcDestSize(sizeSrc);

		bRes = aProcessors[nProcessor]->ProcessPixels(pSrcPixels, sizeSrc, pDestPixels, sizeDest);
	}

	// update the bitmap
	if(bRes)
	{
		// set the bits
		HDC		hdc = GetDC(NULL);
		HBITMAP hbmSrc = :: CreateCompatibleBitmap(hdc, sizeDest.cx, sizeDest.cy);

		if(hbmSrc)
		{
			BITMAPINFO	bi;

			if(PrepareBitmapInfo32(bi, hbmSrc))
			{
				if(SetDIBits(hdc, hbmSrc, 0, sizeDest.cy, pDestPixels, &bi, DIB_RGB_COLORS))
				{
					// delete the bitmap and attach new
					DeleteObject();
					bRes = Attach(hbmSrc);
				}
			}

			:: ReleaseDC(NULL, hdc);

			if(!bRes)
			{
				:: DeleteObject(hbmSrc);
			}
		}
	}

	delete[] pSrcPixels;
	delete[] pDestPixels;

	return(bRes);
}

RGBX *CEnBitmap::GetDIBits32()
{
	BITMAPINFO	bi;

	int			nHeight = PrepareBitmapInfo32(bi);

	if(!nHeight)
	{
		return(FALSE);
	}

	BYTE	*pBits = (BYTE *) new BYTE[bi.bmiHeader.biSizeImage];
	HDC		hdc = GetDC(NULL);

	if(!GetDIBits(hdc, (HBITMAP) GetSafeHandle(), 0, nHeight, pBits, &bi, DIB_RGB_COLORS))
	{
		delete pBits;
		pBits = NULL;
	}

	:: ReleaseDC(NULL, hdc);

	return (RGBX *) pBits;
}

BOOL CEnBitmap::PrepareBitmapInfo32(BITMAPINFO &bi, HBITMAP hBitmap)
{
	if(!hBitmap)
	{
		hBitmap = (HBITMAP) GetSafeHandle();
	}

	BITMAP	BM;

	if(! :: GetObject(hBitmap, sizeof(BM), &BM))
	{
		return(FALSE);
	}

	bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
	bi.bmiHeader.biWidth = BM.bmWidth;
	bi.bmiHeader.biHeight = -BM.bmHeight;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;			// 32 bit
	bi.bmiHeader.biCompression = BI_RGB;	// 32 bit
	bi.bmiHeader.biSizeImage = BM.bmWidth * 4 * BM.bmHeight;	// 32 bit
	bi.bmiHeader.biClrUsed = 0;
	bi.bmiHeader.biClrImportant = 0;

	return(BM.bmHeight);
}

BOOL CEnBitmap::Fill(RGBX *pPixels, CSize size, COLORREF color)
{
	if(!pPixels)
	{
		return(FALSE);
	}

	if(color == -1 || color == RGB(255, 255, 255))
	{
		FillMemory(pPixels, size.cx * 4 * size.cy, 255);		// white
	}
	else if(color == 0)
	{
		FillMemory(pPixels, size.cx * 4 * size.cy, 0);			// black
		}
		else
		{
			// fill the first line with the color
			RGBX	*pLine = &pPixels[0];
			int		nSize = 1;

			pLine[0] = RGBX(color);

			while(1)
			{
				if(nSize > size.cx)
				{
					break;
				}

				// else
				int nAmount = min(size.cx - nSize, nSize) * 4;

				CopyMemory(&pLine[nSize], pLine, nAmount);
				nSize *= 2;
			}

			// use that line to fill the rest of the block
			int nRow = 1;

			while(1)
			{
				if(nRow > size.cy)
				{
					break;
				}

				// else
				int nAmount = min(size.cy - nRow, nRow) * size.cx * 4;

				CopyMemory(&pPixels[nRow * size.cx], pPixels, nAmount);
				nRow *= 2;
			}
		}

	return(TRUE);
}

BOOL CEnBitmap::MakeDisabled(COLORREF bk)
{
	int			R = GetRValue(bk);
	int			G = GetGValue(bk);
	int			B = GetBValue(bk);

	CImageHigh	high(0.08f);
	high.SetBkColor(R, G, B);

	CImageGrayer	gray;
	gray.SetBkColor(R, G, B);

	C32BIPArray aProcessors;
	aProcessors.Add(&gray);
	for(int i = 0; i < 3; i++)
	{
		aProcessors.Add(&high);
	}

	return(ProcessImage(aProcessors));
}

BOOL CEnBitmap::MakeNotActive(COLORREF bk)
{
	int			R = GetRValue(bk);
	int			G = GetGValue(bk);
	int			B = GetBValue(bk);

	C32BIPArray aProcessors;

	CImageHigh	high2(0.04f);
	high2.SetBkColor(R, G, B);
	aProcessors.Add(&high2);
	aProcessors.Add(&high2);

	return(ProcessImage(aProcessors));
}

BOOL CImageGrayer::ProcessPixels(RGBX *pSrcPixels, CSize sizeSrc, RGBX *pDestPixels, CSize)
{
	for(int nX = 0; nX < sizeSrc.cx; nX++)
	{
		for(int nY = 0; nY < sizeSrc.cy; nY++)
		{
			RGBX	*pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];
			RGBX	*pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];
			if(IsBkColor(pRGBSrc))
			{
				*pRGBDest = *pRGBSrc;
			}
			else
			{
				*pRGBDest = pRGBSrc->Gray();
			}
		}
	}

	return(TRUE);
}

CImageHigh::CImageHigh(float nL)
{
	m_fLumDecr = nL;
}

BOOL CImageHigh::ProcessPixels(RGBX *pSrcPixels, CSize sizeSrc, RGBX *pDestPixels, CSize)
{
	CColor	cnv;

	for(int nX = 0; nX < sizeSrc.cx; nX++)
	{
		for(int nY = 0; nY < sizeSrc.cy; nY++)
		{
			RGBX	*pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];
			RGBX	*pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];
			if(IsBkColor(pRGBSrc))
			{
				*pRGBDest = *pRGBSrc;
			}
			else
			{
				cnv.SetRGB(pRGBSrc->btRed, pRGBSrc->btGreen, pRGBSrc->btBlue);

				float	L = cnv.GetLuminance();

				if(m_fLumDecr > 0 && L < 1.0)
				{
					L = min(1, L + m_fLumDecr);
					cnv.SetLuminance(L);
				}
				else if(m_fLumDecr < 0 && L > 0)
				{
					L = max(0, L + m_fLumDecr);
					cnv.SetLuminance(L);
				}

				pRGBDest->btRed = (BYTE) cnv.GetRed();
				pRGBDest->btBlue = (BYTE) cnv.GetBlue();
				pRGBDest->btGreen = (BYTE) cnv.GetGreen();
			}
		}
	}

	return(TRUE);
}

void CToolBar24::SetFullColorImage(UINT ID, COLORREF rgbBack)
{
	m_bmToolbar.LoadBitmap(ID);
	m_imgToolbar.Create(24, 24, ILC_COLOR24 | ILC_MASK, 1, 1);
	m_imgToolbar.Add(&m_bmToolbar, rgbBack);
	GetToolBarCtrl().SetHotImageList(&m_imgToolbar);

	m_bmToolbarDis.LoadBitmap(ID);
	m_bmToolbarDis.MakeDisabled(RGB(255, 0, 255));
	m_imgToolbarDis.Create(24, 24, ILC_COLOR24 | ILC_MASK, 1, 1);
	m_imgToolbarDis.Add(&m_bmToolbarDis, rgbBack);
	GetToolBarCtrl().SetDisabledImageList(&m_imgToolbarDis);

	m_bmToolbarNA.LoadBitmap(ID);
	m_bmToolbarNA.MakeNotActive(RGB(255, 0, 255));
	m_imgToolbarNA.Create(24, 24, ILC_COLOR24 | ILC_MASK, 1, 1);
	m_imgToolbarNA.Add(&m_bmToolbarNA, rgbBack);
	GetToolBarCtrl().SetImageList(&m_imgToolbarNA);
}
