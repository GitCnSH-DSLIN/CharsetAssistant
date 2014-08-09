// DlgFileConv.cpp : implementation file
//

#include "stdafx.h"
#include "CharsetAssistant.h"
#include "DlgFileConv.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define WM_USER_CONV_PROG	WM_USER+1
#define WM_USER_CONV_DONE	WM_USER+2
#define WM_USER_CONV_STOP	WM_USER+3

/////////////////////////////////////////////////////////////////////////////
// CDlgFileConv dialog


CDlgFileConv::CDlgFileConv(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgFileConv::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgFileConv)
	m_strSrcFile = _T("");
	m_strSrcDir = _T("");
	m_strDstDir = _T("");
	m_bSaveSameDir = FALSE;
	m_bRecurDir = FALSE;
	m_strDstCharset = _T("");
	m_strSrcCharset = _T("");
	//}}AFX_DATA_INIT
}


void CDlgFileConv::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgFileConv)
	DDX_Control(pDX, IDC_PROGRESS1, m_progTotal);
	DDX_Control(pDX, IDC_RDO_SRC_FILE, m_rdSrcFile);
	DDX_Control(pDX, IDC_RDO_SRC_DIR, m_rdSrcDir);
	DDX_Text(pDX, IDC_EDT_SRC_FILE, m_strSrcFile);
	DDX_Text(pDX, IDC_EDT_SRC_DIR, m_strSrcDir);
	DDX_Text(pDX, IDC_EDT_DST_DIR, m_strDstDir);
	DDX_Check(pDX, IDC_CHK_SAME_DIR, m_bSaveSameDir);
	DDX_Check(pDX, IDC_CHK_WRITE_BOM, m_bWriteBOM);
	DDX_Check(pDX, IDC_CHK_RECUR_DIR, m_bRecurDir);
	DDX_CBString(pDX, IDC_CB_DST_CHARSET, m_strDstCharset);
	DDX_CBString(pDX, IDC_CB_SRC_CHARSET, m_strSrcCharset);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgFileConv, CDialog)
	//{{AFX_MSG_MAP(CDlgFileConv)
	ON_WM_DROPFILES()
	ON_COMMAND_RANGE(IDC_RDO_SRC_FILE, IDC_RDO_SRC_DIR, OnSelectSrcType)
	ON_COMMAND(IDC_BT_SCAN_FILE, OnSelectSrcFile)
	ON_COMMAND(IDC_BT_SCAN_SRC_DIR, OnSelectSrcDir)
	ON_COMMAND(IDC_BT_SCAN_DST_DIR, OnSelectDstDir)
	ON_COMMAND(IDC_CHK_SAME_DIR, OnChkSaveSameDir)
	ON_MESSAGE(WM_USER_CONV_DONE, OnMsgConvDone)
	ON_MESSAGE(WM_USER_CONV_PROG, OnMsgConvProg)
	ON_MESSAGE(WM_USER_CONV_STOP, OnMsgConvStop)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgFileConv message handlers

BOOL CDlgFileConv::OnInitDialog() 
{
	CDialog::OnInitDialog();

	CWnd::DragAcceptFiles();

	//����֧�ֵı���
	char *p, *charsets;
	charsets = strdupa(supported_charsets);
	while (p = strsep(&charsets, " "))
	{
		((CComboBox*)GetDlgItem(IDC_CB_SRC_CHARSET))->AddString(stringToCString(p));
		((CComboBox*)GetDlgItem(IDC_CB_DST_CHARSET))->AddString(stringToCString(p));
	}

	//Ĭ��ѡ���ļ�
	m_rdSrcFile.SetCheck(TRUE);
	SwitchSrcType();

	m_bRecurDir = TRUE;
	m_bWriteBOM = TRUE;
	m_bConverting = FALSE;

	m_progTotal.SetRange(0, 100);

	UpdateData(FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

bool CDlgFileConv::ConvertFileToCharset(
	FILE* fps, FILE* fpd, 
	const std::string& src_charset,
	const std::string& dst_charset)
{
	char *src_text = NULL, *dst_text = NULL;
	size_t src_len = 0, dst_len = 0;
	int ret;
	
	if (!xfread(fps, -1, 0, &src_text, &src_len))
		return false;

	ret = convert_to_charset(
		src_charset.c_str(), dst_charset.c_str(),
		src_text, src_len, 
		&dst_text, &dst_len, 0);
	if (!ret) {
		xfree(src_text);
		return false;
	}
	
	ret = fwrite(dst_text, dst_len, 1, fpd);

	xfree(src_text);
	xfree(dst_text);

	return ret == 1;
}

//ת���߳����
static thread_ret_t THREAD_CALLTYPE conv_thread_proc(void* arg)
{
	CDlgFileConv *pThis = (CDlgFileConv*)arg;
	pThis->ConvThreadProc();

	return (thread_ret_t)1;
}

//ת���ļ���һ��
static int convert_line(char *line, size_t len, size_t nline, void *arg)
{
	ConvFileInfo *pInfo = (ConvFileInfo*)arg;
	std::string charset;
	char buf[1024], *outbuf = buf;
	size_t outlen = 1024;
	int ret;

	charset = pInfo->src_charset;
	if (!pInfo->src_bom_charset.empty())
		charset = pInfo->src_bom_charset;

	//�ļ�����ת��ʹ�ü���ģʽ
	//TODO:�ڽ�����ѡ���Ƿ�ʹ�ü���ģʽ
	ret = convert_to_charset(charset.c_str(), pInfo->dst_charset.c_str(),
						line, len, &outbuf, &outlen, 0);

	if (ret)
		fwrite(outbuf, outlen, 1, pInfo->fp);	//ת���ɹ���д��ת��������
	else
		fwrite(line, len, 1, pInfo->fp);		//ת��ʧ�ܣ�д��ԭ����

	if (outbuf != buf)
		xfree(outbuf);

	return 1;
}

//ת���߳�
void CDlgFileConv::ConvThreadProc()
{
	std::string srcf, dstf;
	std::string charset;
	FILE *fp, *fps;
	char bom_charset[MAX_CHARSET];
	int count;
	bool success;

	count = m_convFileInfo.src_files.size();
	m_convFileInfo.total = count;

	for (int i = 0; i < count; i++)
	{
		if (m_bTerminate)
		{
			PostMessage(WM_USER_CONV_STOP, count - i);
			return;
		}

		srcf = m_convFileInfo.src_files.front();
		dstf = m_convFileInfo.dst_files.front();	
		m_convFileInfo.src_files.pop_front();
		m_convFileInfo.dst_files.pop_front();

		fp = fps = NULL;

		fps = xfopen(srcf.c_str(), "rb");
		if (!fps)
			goto finish;

		create_directories(dstf.c_str());

		fp = xfopen(dstf.c_str(), "wb");
		if (!fp)
			goto finish;

		//����Դ�ļ���BOMͷ
		m_convFileInfo.src_bom_charset = "";
		if (read_file_bom(fps, bom_charset, MAX_CHARSET)) {
			//�������BOMͷ��������������ַ������û�ѡ����ַ�����һ��
			if (strcasecmp(bom_charset, m_convFileInfo.src_charset.c_str())) {
				//TODO:�����û�
				m_convFileInfo.src_bom_charset = bom_charset;
			}
		}

		charset = m_convFileInfo.src_charset;
		if (!m_convFileInfo.src_bom_charset.empty())
			charset = m_convFileInfo.src_bom_charset;

		//д��Ŀ���ļ���BOMͷ
		if (m_bWriteBOM)
			write_file_bom(fp, m_convFileInfo.dst_charset.c_str());

		// ���ԭ�ļ���UTF-16/32���룬foreach_line�������ܺܺõش�����
		// �����Ҫһ����ת��
		if (charset.find("UTF-16") != NPOS ||
			charset.find("UTF-32") != NPOS) {
			success = ConvertFileToCharset(fps, fp, 
				charset.c_str(), m_convFileInfo.dst_charset);
		} else {
			// ����ת��ԭ�ļ�
			m_convFileInfo.fp = fp;
			success = foreach_line(fps, convert_line, &m_convFileInfo) == 1;
		}

finish:
		if (success) {
			log_dprintf(LOG_INFO, 
				"Converting %s ... %s(%s) -> %s(%s)\n", 
				srcf.c_str(), charset.c_str(), 
				utils::FileSizeReadable(ftell(fps)).c_str(),
				m_convFileInfo.dst_charset.c_str(),
				utils::FileSizeReadable(ftell(fp)).c_str()); 
		} else {
			m_convFileInfo.failed ++;
			log_dprintf(LOG_ERROR, "Failed to convert %s (%s)\n", 
				srcf.c_str(), charset.c_str());
		}

		if (fps)
			xfclose(fps);
		if (fp)
			xfclose(fp);

		//����ת������
		PostMessage(WM_USER_CONV_PROG, i+1, count);
	}

	log_flush(DEBUG_LOG);

	//����ת�������Ϣ
	PostMessage(WM_USER_CONV_DONE);
}

static int handle_src_file(const char* fpath, void *arg)
{
	((utils::string_list*)arg)->push_back(fpath);
	return 1;
}

void CDlgFileConv::OnOK()
{
	UpdateData(TRUE);

	//��ֹת��
	if (m_bConverting)
	{
		m_bTerminate = TRUE;
		return;
	}

	ConvFileInfo& cfi = m_convFileInfo;
	cfi.src_files.clear();
	cfi.dst_files.clear();
	cfi.src_charset = "";
	cfi.dst_charset = "";
	cfi.total = cfi.success = cfi.failed = 0;

	//ת��Դ��Ϣ
	std::string src;
	if (m_rdSrcFile.GetCheck())
	{
		src = CStringToUTF8string(m_strSrcFile);
		if (src.empty())
		{
			AfxMessageBox(_T("��ѡ��һ����ת�����ļ���"));
			return;
		}

		cfi.src_files.push_back(src);
	}
	else
	{
		src = CStringToUTF8string(m_strSrcDir);
		if (src.empty())
		{
			AfxMessageBox(_T("��ѡ��һ����ת����Ŀ¼��"));
			return;
		}
		
		if (src.length() == 3)
		{
			AfxMessageBox(_T("�޷�ת��������������"));
			return;
		}

		foreach_file(src.c_str(), handle_src_file, m_bRecurDir, 1, &cfi.src_files);

		if (cfi.src_files.size() == 0)
		{
			AfxMessageBox(_T("ѡ���Ŀ¼��û����Ҫת�����ļ���"));
			return;
		}
	}

	if (m_strSrcCharset.IsEmpty())
	{
		AfxMessageBox(_T("��ѡ��Դ�ļ������ʽ��"));
		return;
	}
	cfi.src_charset = CStringToUTF8string(m_strSrcCharset);

	//Ŀ����Ϣ
	std::string save_dir;
	if (m_bSaveSameDir)
		save_dir = utils::PathFindDirectory(src);
	else
	{
		save_dir = CStringToUTF8string(m_strDstDir);
		if (save_dir.empty())
		{
			AfxMessageBox(_T("��ѡ�񱣴�Ŀ¼��"));
			return;
		}
	}
	utils::EndWith(save_dir, PATH_SEP_CHAR);

	if (m_strDstCharset.IsEmpty())
	{
		AfxMessageBox(_T("��ѡ��Ŀ���ļ������ʽ��"));
		return;
	}
	cfi.dst_charset = CStringToUTF8string(m_strDstCharset);

	//���ɱ����ļ�·��
	if (m_rdSrcFile.GetCheck())
	{
		std::string dstf;
		if (save_dir == utils::PathFindDirectory(src))
		{
			dstf = src.insert(src.length() - utils::PathFindExtension(src).length(), \
								CStringToUTF8string(m_strDstCharset).insert(0, "_"));
		}
		else
			dstf = save_dir + utils::PathFindFileName(src);

		if (utils::PathFileExists(dstf))
		{
			std::string msg = "�ļ� ";
			msg += utils::UTF8stringTostring(dstf);
			msg += "�Ѿ����ڣ��Ƿ񸲸ǣ�";
			
			if (AfxMessageBox(stringToCString(msg), MB_OKCANCEL) != IDOK)
				return;
		}

		cfi.dst_files.push_back(dstf);
	}
	else
	{
		utils::string_list::iterator it;
		std::string srcf, dstf;

		if (save_dir == utils::PathFindDirectory(src))
		{
			save_dir += utils::PathFindFileName(src);
			save_dir += "_";
			save_dir += CStringToUTF8string(m_strDstCharset);
		}
		else
			save_dir += utils::PathFindFileName(src);

		if (utils::PathFileExists(save_dir))
		{
			std::string msg = "Ŀ¼ ";
			msg += utils::UTF8stringTostring(save_dir);
			msg += "�Ѿ����ڣ��Ƿ񸲸ǣ�";

			if (AfxMessageBox(stringToCString(msg), MB_OKCANCEL) != IDOK)
				return;
		}

		for (it = cfi.src_files.begin(); it != cfi.src_files.end(); it++)
		{
			srcf = *it;
			dstf = srcf.replace(0, src.length(), save_dir);
			cfi.dst_files.push_back(dstf);
		}
	}

	//����ת���߳�
	m_bConverting = TRUE;
	thread_create1(&m_thread, conv_thread_proc, this, 0);

	m_bTerminate = FALSE;
	SetDlgItemText(IDOK, _T("ֹͣת��"));
}

void CDlgFileConv::SwitchSrcType()
{
	BOOL bFileMode = m_rdSrcFile.GetCheck();

	GetDlgItem(IDC_EDT_SRC_FILE)->EnableWindow(bFileMode);
	GetDlgItem(IDC_BT_SCAN_FILE)->EnableWindow(bFileMode);

	GetDlgItem(IDC_EDT_SRC_DIR)->EnableWindow(!bFileMode);
	GetDlgItem(IDC_BT_SCAN_SRC_DIR)->EnableWindow(!bFileMode);
	GetDlgItem(IDC_CHK_RECUR_DIR)->EnableWindow(!bFileMode);
	GetDlgItem(IDC_RDO_FILTER_NONE)->EnableWindow(!bFileMode);
	GetDlgItem(IDC_RDO_FILTER_TYPE)->EnableWindow(!bFileMode);
	GetDlgItem(IDC_RDO_FILTER_WILDCARD)->EnableWindow(!bFileMode);
}

void CDlgFileConv::OnSelectSrcType(UINT nID)
{
	SwitchSrcType();
}

//ת��Դ�ļ�
void CDlgFileConv::OnSelectSrcFile()
{
	CStringW strFile = SelectFile(_T("�����ļ�"), _T(""));
	if (!strFile.IsEmpty())
		m_strSrcFile = strFile;
	else
		return;

	double prob;
	std::string charset = utils::GetFileCharset(CStringToUTF8string(strFile), prob, 1000);
	if (!charset.empty() && prob > 0.95)
		m_strSrcCharset = UTF8stringToCString(charset);
	else
		m_strSrcCharset = _T("");

	UpdateData(FALSE);
}

static int find_file_charset_of_src_dir(const char* fpath, void *arg)
{
	double prob;
	std::string charset = utils::GetFileCharset(fpath, prob, 1000);
	if (!charset.empty() && prob > 0.95)
		*((std::string*)arg) = charset;

	return 0;
}

//ת��ԴĿ¼
void CDlgFileConv::OnSelectSrcDir()
{
	UpdateData(TRUE);

	CStringW strDir = SelectDirectory(_T("��ѡ��һ��Ҫת����Ŀ¼��"));
	if (!strDir.IsEmpty())
		m_strSrcDir = strDir;
	else
		return;

	std::string charset;
	foreach_file(CStringToUTF8string(strDir).c_str(), find_file_charset_of_src_dir, m_bRecurDir, 1, &charset);
	if (!charset.empty())
		m_strSrcCharset = UTF8stringToCString(charset);
	else
		m_strSrcCharset = _T("");

	UpdateData(FALSE);
}

//ѡ�񱣴�Ŀ¼
void CDlgFileConv::OnSelectDstDir()
{
	CStringW strDir = SelectDirectory(_T("��ѡ�񱣴�Ŀ¼��"));
	if (!strDir.IsEmpty())
		m_strDstDir = strDir;
	else
		return;

	UpdateData(FALSE);
}

//�ļ���ק
void CDlgFileConv::OnDropFiles(HDROP hDropInfo)
{
	TCHAR cFileName[MAX_PATH]; 
	UINT uFileCount, i = 0; 
	CString strPath;
	
	uFileCount = ::DragQueryFile(hDropInfo, -1, cFileName, sizeof(cFileName));   //�������ļ����� 
	::DragQueryFile(hDropInfo, i, cFileName, sizeof(cFileName));				//ȡ��ÿ���ļ����ļ��� 
	
	strPath = cFileName;
	if (PathIsDirectory(strPath))
	{
		m_rdSrcDir.SetCheck(TRUE);
		m_rdSrcFile.SetCheck(FALSE);
		SwitchSrcType();

		m_strSrcDir = strPath;

		std::string charset;
		foreach_file(CStringToUTF8string(strPath).c_str(), find_file_charset_of_src_dir, m_bRecurDir, 1, &charset);
		if (!charset.empty())
			m_strSrcCharset = UTF8stringToCString(charset);
		else
			m_strSrcCharset = _T("");
	}
	else
	{
		m_rdSrcFile.SetCheck(TRUE);
		m_rdSrcDir.SetCheck(FALSE);
		SwitchSrcType();

		m_strSrcFile = strPath;

		double prob;
		std::string charset = utils::GetFileCharset(CStringToUTF8string(strPath), prob, 1000);
		if (!charset.empty() && prob > 0.95)
			m_strSrcCharset = UTF8stringToCString(charset);
		else
			m_strSrcCharset = _T("");
	}

	UpdateData(FALSE);

	::DragFinish(hDropInfo);
	
	CDialog::OnDropFiles(hDropInfo);
}

void CDlgFileConv::OnChkSaveSameDir()
{
	UpdateData(TRUE);

	GetDlgItem(IDC_EDT_DST_DIR)->EnableWindow(!m_bSaveSameDir);
	GetDlgItem(IDC_BT_SCAN_DST_DIR)->EnableWindow(!m_bSaveSameDir);
}

//ת������
LRESULT CDlgFileConv::OnMsgConvProg(WPARAM wp, LPARAM lp)
{
	int curr = (int)wp;
	int total = (int)lp;

	m_progTotal.SetPos((curr+1)*100 / total);
	
	CString strProg;
	strProg.Format(_T("%d/%d"), curr, total);
	SetDlgItemText(IDC_ST_PROGRESS, strProg);

	return TRUE;
}

//ת������¼�
LRESULT CDlgFileConv::OnMsgConvDone(WPARAM wp, LPARAM lp)
{
	CString str;
	str.Format(_T("ת����ɣ���ת�� %d ���ļ���ʧ�� %d ����"),
		m_convFileInfo.total, m_convFileInfo.failed);
	AfxMessageBox(str);
	
	m_bConverting = FALSE;
	SetDlgItemText(IDOK, _T("��ʼת��"));

	return TRUE;
}

//ת����ֹ�¼�
LRESULT CDlgFileConv::OnMsgConvStop(WPARAM wp, LPARAM lp)
{
	CString str;
	str.Format(_T("ת������ֹ����ת�� %d ���ļ���ʧ�� %d ����%d ��δת����"),
		m_convFileInfo.total, m_convFileInfo.failed, (int)wp);
	AfxMessageBox(str);

	m_bConverting = FALSE;
	SetDlgItemText(IDOK, _T("��ʼת��"));
	
	return TRUE;
}
