/**
*
* @file: MfcUtil.h
* @desc: MFC���ʵ�ú���
* 
* @auth: liwei (www.leewei.org)
* @mail: ari.feng@qq.com
* @date: 2012/04/10
* @mdfy: 2012/10/24
* @Ver: 3.8
*
*/

/* ��Ҫ������
 *
 * 1���ַ���������ת��
 * 2��ͼ��ͼ�����(GDI+,OpenCV..)
 * 3������
 */

#ifndef __MFCUTIL_H
#define __MFCUTIL_H

#include <string>

#if _MSC_VER <= 1200  //VC6
#include <ATLBASE.H>
#define atlTraceUI 0
typedef ULONG ULONG_PTR;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;
#include "StringA.h"
#include "StringW.h"
#endif

//GDI+
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

//CxImage
//#include "../CxImage/ximage.h"
//#pragma comment(lib, "../CxImage/cximageu.lib")

namespace mfcutil{

CString			GetModulePath();												//��ȡ��ִ���ļ�����Ŀ¼
void			GetModulePath(CString &rString);

CString			SelectFile(CString desc, CString filter);						//ѡ��һ���ļ�
CString			SelectDirectory(CString title);									//ѡ��һ��Ŀ¼
void			ShellExec(const CString &strFile, const CString &strParam,		//ִ��ϵͳ�������ļ�
							DWORD dwWait = 0, int nShow = SW_SHOW);

BOOL			Shutdown();														//�ػ�

//////////////////////////////////////////////////////////////////////////
//�ַ���������ת��

namespace mfcharset{

//CStringW <==> CStringA
CStringW		CSA2CSW(const CStringA &strA);
CStringA		CSW2CSA(const CStringW &strW);

//CStringW <==> wstring
CStringW		wstringToCStringW(const std::wstring &ws);
std::wstring	CStringWTowstring(const CStringW &strW);

//CStringA <==> string
CStringA		stringToCStringA(const std::string &s);
std::string		CStringATostring(const CStringA &strA);	

#ifdef UNICODE
  #define CStringTowstring(x)	CStringWTowstring((CStringW)x)
  #define wstringToCString(x)	wstringToCStringW(x)
  #define CStringTostring(x)	CStringATostring(CSW2CSA((CStringW)x))
  #define stringToCString(x)	CSA2CSW(stringToCStringA(x))
#else //MBCS
  #define CStringTostring(x)	CStringATostring((CStringA)x)
  #define stringToCString(x)	stringToCStringA(x)
  #define CStringTowstring(x)   CStringWTowstring(CSA2CSW((CStringA)x))
  #define wstringToCString(x)   CSW2CSA(wstringToCStringW(x))
#endif

//CStringW <==> UTF-8
CStringA		CSW2UTF8(const CStringW& strCsw);
CStringW		UTF82CSW(const CStringA& strUtf8);

//CStringA <==> UTF-8
#define CSA2UTF8(x) CSW2UTF8(CSA2CSW(x))
#define UTF82CSA(x) CSW2CSA(UTF82CSW(x))

//CString <==> UTF-8 string
#ifdef UNICODE
  #define CStringToUTF8string(x) CStringATostring(CSW2UTF8((CStringW)x))
  #define UTF8stringToCString(x) UTF82CSW(stringToCStringA(x))
#else
  #define CStringToUTF8string(x) CStringATostring(CSA2UTF8((CStringA)x))
  #define UTF8stringToCString(x) UTF82CSA(stringToCStringA(x))
#endif

}; //namespace charset

//////////////////////////////////////////////////////////////////////////
//ͼ��ͼ����GDI���ơ�OpenCV��CxImage...

//UImgLib
namespace mfgraph{

 int		ColorCmp(Color c1, Color c2);													//�Ƚ�������ɫ�Ƿ���ͬ
 BOOL		PathIsPic(CString path);														//ָ��·���Ƿ�ΪͼƬ�ļ�

 void		RotateByPoints (PointF *point, int iAngle, int &dst_w, int &dst_h);				//������ת
 int		GetEncoderClsid (BSTR format, CLSID* pClsid);

 void		GetColorBitmap(int w, int h, COLORREF color, CBitmap &bmp);

#ifdef _GDIPLUS_H
 void		GetBitmapInfoHeader(Bitmap *pbm, BITMAPINFOHEADER &bmih, BitmapData &bmd);

 Bitmap*	SelectLoadImage();																//�Ӵ���ѡ�񲢼���һ��ͼ��

 BOOL		SaveBitmap(Bitmap *pbm, CString path);											//����λͼΪָ����ʽ�ļ�
 Bitmap*	LoadBitmap(CString path);														//��·������ͼ��

 Bitmap*	CloneBitmap(Bitmap *pbm);														//��������һ��ͼ��
 Bitmap*	CloneBitmapRGB(Bitmap *pbm);													//����һ��ͼƬ(������͸�����)
 Bitmap*	CloneBitmapARGB(Bitmap *pbm);													//����һ��ͼƬ(����͸�����)

 void		CloneAlpha(Bitmap **pbmDst, Bitmap *pbmSrc);									//����Src��͸�����õ�Dst

 void		SetColorTrans(Bitmap *pbm, Color color, unsigned char level);					//Ϊָ����ɫ����������͸����
 void		SetTransColor(Bitmap *pbm, Color color);										//��͸������������Ϊ��ɫcolor
 void		SetColorColor(Bitmap *pbm, Color from, Color to);								//��ͼ���е���ɫfrom�滻Ϊ��ɫto
 BOOL		HasAlpha(Bitmap* pbm);															//ͼ���Ƿ���͸����

 void		RGB2ARGB(Bitmap **pbm);															//��RGBģʽת��ΪARGBģʽ
 void		ARGB2RGB(Bitmap **pbm);

 Bitmap*	GetThumb(Bitmap *pbm, int w, int h, Color *border = NULL);						//��ȡ�ȱ����ź������ͼ��(ͼ�񱣳�͸��)
 Bitmap*	GetThumbnail(Bitmap *pbm, int w, int h, Color &bg = Color(255,255,255));		//��ȡ�ȱ����ź������ͼ��(͸�����)
 Bitmap*	GetThumbAlphaColor(Bitmap *pbm, int w, int h, Color alpha_color, Color bg);		//ͬ�ϣ���ԭͼ͸���������Ϊָ����ɫ

 Bitmap*	RotateBitmap(Bitmap *pbm, int angle, int &w, int &h, PointF *ptf = NULL);		//��תͼ��
 Bitmap*	ScaleImageFitRange(Bitmap *pbm, int *w, int *h);								//�ȱ�����ͼ������Ӧָ���ߴ�

 void		DrawNumber(Graphics *g, PointF &point, int num, int sz, bool bHori = true);
 void		DrawString(Graphics *g, PointF &point, CString text, int sz, bool bHori = true);

#if (defined __CXIMAGE_H) && (defined _GDIPLUS_H)
 BOOL		SaveBitmapX(Bitmap *pbm, CString path);											//�ɱ���bmp͸��ͼ��
 Bitmap* 	LoadBitmapX(CString path);														//��CxImage����ͼ��

 Bitmap*	Ximage2Bitmap (CxImage *pXimage);												//CxImage��ʽת��ΪBitmap��ʽ
 BOOL		Bitmap2Ximage (Bitmap *pBitmap, CxImage &xImage);								//Bitmap��ʽת��ΪCxImage��ʽ
#endif

#ifdef OPENCV
 Bitmap*	FloodFill (Bitmap* pBitmap, unsigned x, unsigned y, Color newclr);				//��ˮ����㷨����ָ���㿪ʼ��ɢ
 Bitmap*	FloodFillCorner (Bitmap* pBitmap, Color clrnew);								//��ˮ����㷨���Ӱ�ɫ�Ķ�����ɢ
 Bitmap*	GetBitmapBorderTrans(Bitmap *pbm);												//��ͼ���ܱߵİ�ɫ��Ϊ͸��

 IplImage*   Bitmap2IplImage (Bitmap* pBitmap);
 Bitmap*	 IplImage2Bitmap (IplImage* pIplImg);
#endif //ifdef OPENCV

#endif //ifdef GDIPLUS

#ifdef OPENCV
 void		cvGetPixel(IplImage* pIplImg, int i, int j, Color &color);						//��ȡĳ�����ص���ɫֵ
 void		cvSetPixel(IplImage* pIplImg, int i, int j, Color color);						//����ĳ�����ص���ɫֵ
#endif //ifdef OPENCV

}; //namespace mfgraph

}; //namespace mfcutil;

#endif