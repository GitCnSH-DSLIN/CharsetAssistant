// DlgLookup.cpp : implementation file
//

#include "stdafx.h"
#include "CharsetAssistant.h"
#include "DlgLookup.h"

#include "WubiCode.h"
#include "../utils/src/cutil.h"

using namespace std;

struct text_range{
	int		start;
	int		end;
};


/////////////////////////////////////////////////////////////////////////////
// CDlgLookup dialog


CDlgLookup::CDlgLookup(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgLookup::IDD, pParent)
{
	m_clrHighLight = RGB(255,0,0);
}


void CDlgLookup::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BT_HIGHLIGHT, m_btHighlight);
	DDX_Control(pDX, IDC_ST_TIPS, m_lbTips);
	DDX_Control(pDX, IDC_CB_OTHER_CHARSETS, m_cbCharsets);
	DDX_Control(pDX, IDC_REDT_INPUT, m_edtText);
	DDX_Control(pDX, IDC_RDO_CAT_HEX, m_rdHexCat);
	DDX_Control(pDX, IDC_RDO_CAT_DEC, m_rdOctCat);
	DDX_Control(pDX, IDC_RDO_CAT_BIN, m_rdBinCat);
	DDX_Control(pDX, IDC_CHK_HEX_UPPER, m_ckCapital);
	DDX_Control(pDX, IDC_CHK_DEC_SIGNED, m_ckSigned);
	DDX_Control(pDX, IDOK, m_btLookup);
	DDX_Control(pDX, IDC_BT_CLEAR, m_btClear);
	DDX_Control(pDX, IDC_BT_SAVE, m_btSave);

	//{{AFX_DATA_MAP(CDlgLookup)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgLookup, CDialog)
	//{{AFX_MSG_MAP(CDlgLookup)
	ON_COMMAND_RANGE(IDC_RDO_CS_GBK, IDC_RDO_CS_OTHER, OnSelectCharset)
	ON_COMMAND_RANGE(IDC_RDO_CAT_HEX, IDC_RDO_CAT_BIN, OnSelectScale)
	ON_CBN_SELCHANGE(IDC_CB_OTHER_CHARSETS, OnSelOtherCharsets)
	ON_COMMAND(IDC_BT_CLEAR, OnClear)
	ON_COMMAND(IDC_CHK_HEX_UPPER, ReLookup)
	ON_COMMAND(IDC_CHK_DEC_SIGNED, ReLookup)
	ON_COMMAND(IDC_BT_HIGHLIGHT, OnBnClickedBtColor)
	ON_COMMAND(IDC_BT_SAVE, OnBnClickedBtSave)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgLookup message handlers

BOOL CDlgLookup::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	m_btHighlight.SetFlat(FALSE);
	m_btHighlight.SetColor(CButtonST::BTNST_COLOR_FG_IN, m_clrHighLight);
	m_btHighlight.SetColor(CButtonST::BTNST_COLOR_FG_OUT, m_clrHighLight);
	m_btHighlight.SetColor(CButtonST::BTNST_COLOR_FG_FOCUS, m_clrHighLight);    
	
	m_btSave.EnableWindow(FALSE);

	m_lbTips.SetTextColor(RGB(255,128,0));
	m_lbTips.FlashText(TRUE);

	//����֧�ֵı���
	char *p, *charsets;
	charsets = strdupa(supported_charsets);
	while (p = strsep(&charsets, " "))
		m_cbCharsets.AddString(stringToCString(p));
	m_cbCharsets.SelectString(0, _T("ASCII"));

	//Ĭ�ϲ�ѯGBK����
	((CButton*)GetDlgItem(IDC_RDO_CS_GBK))->SetCheck(TRUE);
	m_strCharset = _T("GBK");

	//Ĭ����ʮ�����ƴ�д��ʾ
	m_rdHexCat.SetCheck(TRUE);
	m_ckCapital.SetCheck(TRUE);
	m_ckSigned.EnableWindow(FALSE);

	//Tooltip
	m_tooltip.Create(this);
	m_tooltip.SetSize(CPPToolTip::PPTTSZ_MARGIN_CX, 8);
	m_tooltip.SetSize(CPPToolTip::PPTTSZ_MARGIN_CY, 8);
	m_tooltip.AddTool(&m_btHighlight, _T("���ı��������ɫ"));
	m_tooltip.AddTool(&m_btSave, _T("�����ѯ�����rtf�ļ�"));
	
	m_tooltip.AddTool(GetDlgItem(IDC_RDO_CS_GB2312), _T("GB2312������2���ֽڱ�ʾһ������\n���뷶Χ��0xA1A1~0xF7FE��\n����¼6763�����ֺ�682�����š�"));
	m_tooltip.AddTool(GetDlgItem(IDC_RDO_CS_GBK), _T("GBK������GB2312�ĳ�������2���ֽڱ�ʾһ�����֣�\n���뷶Χ��0x8140~0xFEFE��������0x??7Fһ���ߣ�\n������21886�����ֺ�883��ͼ�η��š�\nWindows��Ӧ����ҳ��CP936��"));
	m_tooltip.AddTool(GetDlgItem(IDC_RDO_CS_GB18030), _T("GB18030������GBK/GB2312�ĳ�����\n��2��4���ֽڱ�ʾһ�����֣�\nGB18030-2005��׼��������76556���ַ���"));
	m_tooltip.AddTool(GetDlgItem(IDC_RDO_CS_WUBI), _T("���������"));
	m_tooltip.AddTool(GetDlgItem(IDC_RDO_CS_QUWEI), _T("������λ��"));

	m_tooltip.AddTool(GetDlgItem(IDC_RDO_CS_UTF8), _T("UTF-8��UNICODE��һ�ֱ䳤�ַ����룬�ֳ�����룬\n��1~6���ֽڱ�ʾһ���ַ�������ռ3���ֽڣ���\nWindows��Ӧ����ҳ��CP65001��"));
	m_tooltip.AddTool(GetDlgItem(IDC_RDO_CS_UTF16LE), _T("��windows�±�������Unicode���롱��ÿ���ַ�ռ2���ֽڡ�\nUTF-16�ɿ�����UCS-2�ĸ�������û�и���ƽ��ǰUTF-16�͵���UCS-2"));
	m_tooltip.AddTool(GetDlgItem(IDC_RDO_CS_UTF16BE), _T("UTF-16�Ĵ�˱�ʾ��"));
	m_tooltip.AddTool(GetDlgItem(IDC_RDO_CS_UTF32LE), _T("UTF-32 LE��ʹ����unicode��Χ(0��0x10FFFF)��32λ����, ��ռ�ռ��(ÿ�ַ�4���ֽ�)���ٱ�ʹ��"));
	m_tooltip.AddTool(GetDlgItem(IDC_RDO_CS_UTF32BE), _T("UTF-32�Ĵ�˱�ʾ��������UTF-32LE"));
	m_tooltip.AddTool(GetDlgItem(IDC_RDO_CS_BIG5), _T("BIG-5����ͨ����̨�塢��۵�����һ�������ֱ��뷽����\n�׳ơ������롱������¼13,060��������\n"));

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

#define OUTBUF	10

void CDlgLookup::OnOK()
{
	CStringW strFrom, strTo;
	CStringW strOutput, strText;
	std::string from_cs, to_cs;
	const char *inbuf;
	char outbuf[OUTBUF];
	size_t inlen, outlen;
	bool bWarned = false;

	if (m_strCharset.IsEmpty())
	{
		AfxMessageBox(_T("��ѡ��һ���ַ����룡"));
		return;
	}

	strFrom = _T("UTF-16LE");
	strTo = m_strCharset;
	from_cs = CStringTostring(strFrom);			//ԭ�ַ���
	to_cs = CStringTostring(strTo);				//Ŀ���ַ���

	if (strTo == "SECTPOS")
		to_cs = "GB2312";						//�������Ӧ��������

	//strInput�д����Unicode (UTF-16LE)��ʽ������ַ���
	GetDlgItemText(IDC_REDT_INPUT, m_strInput);

	//��ÿ��Unicode�ַ�����ת��
	int pos = 0;
	list<text_range> scope_list;
	list<text_range>::iterator iter;

	for (int i = 0; i < m_strInput.GetLength(); i++)
	{
		wchar_t wch;
		CString strChar, strCode;
		text_range scope;

		//��ת���ַ�
		wch = m_strInput.GetAt(i);

		//��ʼ�����뻺���������ȵ�
		inbuf = (const char*)&wch;
		inlen = sizeof(wch);
		outlen = OUTBUF;

		//�س����з�
		if (wch == '\r' || wch == '\n')
		{
			strOutput += wch;
			pos += 1;
			continue;
		}

		//////////////////////////////////////////////////////////////////////////

		//������ͱ����ѯ
		if (strTo == _T("WUBI"))
		{
			std::string res = CWubiCode::Find(std::wstring(1, wch));
			if (!res.empty() && res.length() <= 4)
			{
				memset(outbuf, 0, OUTBUF);
				strncpy(outbuf, res.c_str(), res.length());
				outlen = res.length();
				strCode = outbuf;
			}
			else
			{
				strOutput += wch;
				pos += wch > CHAR_MAX ? 2 : 1;
				continue;
			}
		}
		//������λ��
		else if (strTo == _T("SECTPOS"))
		{
			//��λ���ǻ���GBK����ģ�����ת��ΪGBK����
			char*p = outbuf;
			if (wch > 127 && convert_to_charset(from_cs.c_str(), "GBK", inbuf, inlen, &p, &outlen, 1))
			{
				outbuf[0] &= ~(1<<7);
				outbuf[0] -= 32;
				outbuf[1] &= ~(1<<7);
				outbuf[1] -= 32;
				strCode.Format(_T("%.2d%.2d"), outbuf[0], outbuf[1]);
			} else {
				strOutput += wch;
				pos += wch > CHAR_MAX ? 2 : 1;
				continue;
			}
		}
		//��������ʹ��ICONV����ת��������Ŀ���ַ������ȸ�ֵ��outlen
		else
		{
			char *p = outbuf;
			if (!convert_to_charset(from_cs.c_str(), to_cs.c_str(), inbuf, inlen, &p, &outlen, 1)) {
				if (!bWarned)
				{
					if (errno == E2BIG)
						AfxMessageBox(_T("Ŀ�껺����̫С�޷�����ת������ַ�"));
					else if (errno == EILSEQ)
						AfxMessageBox(_T("��ת�����ݰ���Ŀ������޷�ʶ����ַ�"));
					if (errno == EINVAL)
						AfxMessageBox(_T("����ȫ�Ĵ�ת����������"));
					
					bWarned = true;
				}
				strOutput += wch;
				pos += wch > CHAR_MAX ? 2 : 1;
			    continue;
			} else {
				for (int j = 0; j < outlen; j++)
				{
					//ʮ��������ʾ��ʽ
					if (m_rdHexCat.GetCheck())
					{
						if (m_ckCapital.GetCheck())
							strChar.Format(_T("%.2X"), (unsigned char)outbuf[j]);
						else
							strChar.Format(_T("%.2x"), (unsigned char)outbuf[j]);
					}
					//ʮ������ʾ��ʽ
					else if (m_rdOctCat.GetCheck())
					{
						if (!m_ckSigned.GetCheck())
							strChar.Format(_T("%.2d"), (unsigned char)outbuf[j]);
						else
							strChar.Format(_T("%.2d"), outbuf[j]);
					}
					else
						//�����Ʋ鿴��ʽ
					{
						strChar = _T("");
						for (int i = 1; i <= 128; i<<=1)
						{
							if (outbuf[j] & i)
								strChar.Insert(0, _T("1"));
							else
								strChar.Insert(0, _T("0"));
						}
					}
					
					strCode += strChar;
					if (j != outlen - 1)
						strCode += _T(" ");
				}
			}
		}

		//���һ���ַ���������ʾ���硰��(e4 b8 ad) ����"��(3287) "��"��(ksk)"
		strOutput += wch;
		strOutput += _T("(");
		strOutput += strCode;
		strOutput += _T(") ");

		//����˱���ĸ�����Χ
		pos += wch > CHAR_MAX ? 2 : 1;
		scope.start = pos + 1 /* '(' */;
		scope.end = scope.start + strCode.GetLength();
		scope_list.push_back(scope);

		pos = scope.end + 2 /* ") " */;

	} //foreach character

	//�����������
	SetDlgItemText(IDC_REDT_INPUT, strOutput);

	//���������ʾ
	for (iter = scope_list.begin(); iter != scope_list.end(); iter++)
	{
		m_edtText.SetSel(iter->start, iter->end);
		m_edtText.SetColour(m_clrHighLight);
	}
	
	m_edtText.SetSel(0, strOutput.GetLength());
	m_edtText.SetFontSize(13);
	m_edtText.SetReadOnly(TRUE);

	m_btLookup.EnableWindow(FALSE);
	m_btClear.EnableWindow(TRUE);
	m_btSave.EnableWindow(TRUE);	
}

void CDlgLookup::ReLookup()
{
	//�û��ڲ�ѯ��һ�ֱ������Ҫ���ٲ�ѯ��������Ľ��
	if (!m_strInput.IsEmpty() && !m_btLookup.IsWindowEnabled())
	{
		m_edtText.SetWindowText(m_strInput);
		OnOK();
	}
}

void CDlgLookup::OnClear()
{
	m_strInput = _T("");
	m_edtText.SetWindowText(_T(""));
	
	m_edtText.SetScrollRange(SB_VERT, 0,0);
	m_edtText.SetScrollRange(SB_HORZ, 0,0);

	m_edtText.SetReadOnly(FALSE);
	m_btClear.EnableWindow(FALSE);
	m_btSave.EnableWindow(FALSE);
	m_btLookup.EnableWindow(TRUE);
}

#define CASE_CHARSET(idc, s)	\
		case idc:				\
		m_strCharset = _T(s);	\
		bValid = true;		\
		break

void CDlgLookup::OnSelectCharset(UINT nID)
{
	bool bValid = false;
	
	switch(nID)
	{
	CASE_CHARSET(IDC_RDO_CS_GBK, "GBK");
	CASE_CHARSET(IDC_RDO_CS_GB2312, "GB2312");
	CASE_CHARSET(IDC_RDO_CS_GB18030, "GB18030");
	CASE_CHARSET(IDC_RDO_CS_BIG5, "BIG5");
	CASE_CHARSET(IDC_RDO_CS_QUWEI, "SECTPOS");
	CASE_CHARSET(IDC_RDO_CS_WUBI, "WUBI");
	CASE_CHARSET(IDC_RDO_CS_UTF7, "UTF-7");
	CASE_CHARSET(IDC_RDO_CS_UTF8, "UTF-8");
	CASE_CHARSET(IDC_RDO_CS_UTF16LE, "UTF-16LE");
	CASE_CHARSET(IDC_RDO_CS_UTF16BE, "UTF-16BE");
	CASE_CHARSET(IDC_RDO_CS_UTF32LE, "UTF-32LE");
	CASE_CHARSET(IDC_RDO_CS_UTF32BE, "UTF-32BE");
	case IDC_RDO_CS_OTHER:
	m_cbCharsets.GetWindowText(m_strCharset);
	bValid = true;
	break;
	default:
	NOT_REACHED();
	m_strCharset = _T("");
	}

	if (bValid)
		ReLookup();
}

void CDlgLookup::OnSelectScale(UINT nID)
{
	switch(nID)
	{
	case IDC_RDO_CAT_HEX:
		m_ckCapital.EnableWindow(TRUE);
		m_ckSigned.EnableWindow(FALSE);
		break;
	case IDC_RDO_CAT_DEC:
		m_ckCapital.EnableWindow(FALSE);
		m_ckSigned.EnableWindow(TRUE);
		break;
	case IDC_RDO_CAT_BIN:
		m_ckCapital.EnableWindow(FALSE);
		m_ckSigned.EnableWindow(FALSE);
		break;
	default:
		NOT_REACHED();
		break;
	}

	ReLookup();
}

//���ĸ�����ɫ
void CDlgLookup::OnBnClickedBtColor()
{
	CColorDialog FarbDlg;
	if( FarbDlg.DoModal() == IDOK )
	{
		m_clrHighLight = FarbDlg.GetColor();
		m_btHighlight.SetColor(CButtonST::BTNST_COLOR_FG_IN, m_clrHighLight);
		m_btHighlight.SetColor(CButtonST::BTNST_COLOR_FG_OUT, m_clrHighLight);
		m_btHighlight.SetColor(CButtonST::BTNST_COLOR_FG_FOCUS, m_clrHighLight);
		
		ReLookup();
	}
}

void CDlgLookup::OnSelOtherCharsets()
{
	m_cbCharsets.GetLBText(m_cbCharsets.GetCurSel(), m_strCharset);
	ReLookup();
}

void CDlgLookup::OnBnClickedBtSave()
{
	static CString strInitDir = _T("");
	static CString strInitExt = _T("rtf");
	static CString strInitFil = _T("����ת�����.rtf");
	static CString strInitNamen = _T("RTF-Files (*.rtf)|All files (*.*)|*.*||");
	
	CString strDateiname;
	if (strInitDir.IsEmpty())	 
	{
		LPTSTR pstr = strInitDir.GetBuffer(1032);
		GetCurrentDirectory(1024,pstr);
		strInitDir.ReleaseBuffer();
	}
	
	UpdateData(true);
	
	CFileDialog DateiDlg (false, strInitExt ,strInitFil,0,strInitNamen);
	DateiDlg.m_ofn.Flags |= OFN_PATHMUSTEXIST;
	DateiDlg.m_ofn.lpstrInitialDir = strInitDir;
	
	if(IDOK == DateiDlg.DoModal())
	{ strDateiname = DateiDlg.GetPathName();
	int iPos = strDateiname.ReverseFind(TCHAR('\\'));
	if( -1 != iPos )
	{ strInitDir = strDateiname.Left(iPos);
	}
	CFileStatus fileStatus;
	BOOL bReturnStatus = FALSE;
	bReturnStatus = CFile::GetStatus(strDateiname,fileStatus);
	if( bReturnStatus )
	{ int iReturn = AfxMessageBox(_T("�ļ��Ѿ����ڣ�Ҫ��������"),MB_YESNOCANCEL);
	if( IDCANCEL == iReturn ) return;
	if( IDNO == iReturn )
	{ return;
	}
	}
	m_edtText.WriteRTF(strDateiname);
	AfxMessageBox(_T("��ѯ�������ɹ���"));
	} 
}

BOOL CDlgLookup::PreTranslateMessage(MSG* pMsg)
{
	m_tooltip.RelayEvent(pMsg);
	
	return CDialog::PreTranslateMessage(pMsg);
}
