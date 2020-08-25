
// CrevisCamPracticeDlg.cpp : ���� ����
//

#include "stdafx.h"
#include "CrevisCamPractice.h"
#include "CrevisCamPracticeDlg.h"
#include "afxdialogex.h"
#include "VirtualFG40.h"

// #ifdef _DEBUG
// #define new DEBUG_NEW
// #endif

// ���� ���α׷� ������ ���Ǵ� CAboutDlg ��ȭ �����Դϴ�.

void ReceiveThreadProc(CCrevisCamPracticeDlg* pPrivate)
{
	pPrivate->ReceiveImage();
}
void NorThreadProc(CCrevisCamPracticeDlg* pPrivate)
{
	pPrivate->ProcessNormalImage();
}
void BinThreadProc(CCrevisCamPracticeDlg* pPrivate)
{
	pPrivate->ProcessBinImage();
}
void ROIThreadProc(CCrevisCamPracticeDlg* pPrivate)
{
	pPrivate->ProcessROIImage();
}
// ThreadProcesses : used for createthread

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// ��ȭ ���� �������Դϴ�.
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �����Դϴ�.

// �����Դϴ�.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CCrevisCamPracticeDlg ��ȭ ����



CCrevisCamPracticeDlg::CCrevisCamPracticeDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CCrevisCamPracticeDlg::IDD, pParent)
	, m_hDevice(-1)
	, m_Width(0)
	, m_Height(0)
	, m_BufferSize(0)
	, m_IsOpened(FALSE)
	, m_deviceEnum(DEVICE_CAMNUM_INIT)
	, m_roiX(0)
	, m_roiY(0)
	, m_roiWidth(0)
	, m_roiHeight(0)
	, m_pOriImage(NULL)
	, m_hGrabThread(NULL)
	, m_pROIImage(NULL)
{
	m_hTerminate[NORMAL_VID] = CreateEvent(NULL, true, false, _T("NORMAL_TERMINATE"));
	m_hTerminate[BINARY_VID] = CreateEvent(NULL, true, false, _T("BINARY_TERMINATE"));
	m_hTerminate[ROI_VID] = CreateEvent(NULL, true, false, _T("ROI_TERMINATE"));
	m_hGrabTerminate = CreateEvent(NULL, true, false, _T("GRAB_TERMINATE"));

	::InitializeCriticalSection(&mSc);

	for (int i = 0; i < TOTAL_DISP; i++)
	{
		m_pGraphics[i] = NULL;
		m_hDC[i] = NULL;
		m_rcDisp[i] = NULL;
		m_IsPlay[i] = FALSE;
		m_hThread[i] = NULL;
		m_pBitmap[i] = NULL;
		m_pImage[i] = NULL;
	}
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCrevisCamPracticeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DEVICEIP, m_DeviceIPControl);
	DDX_Control(pDX, IDC_TBTHRESHOLD, m_TBThreshold);
	DDX_Control(pDX, IDC_ROIXTB, m_TBRoiX);
	DDX_Control(pDX, IDC_ROIYTB, m_TBRoiY);
	DDX_Control(pDX, IDC_ROIWITB, m_TBWidth);
	DDX_Control(pDX, IDC_ROIHETB, m_TBRoiHeight);
}

BEGIN_MESSAGE_MAP(CCrevisCamPracticeDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_OPENBTN, &CCrevisCamPracticeDlg::OnBnClickedOpenbtn)
	ON_BN_CLICKED(IDC_CLOSEBTN, &CCrevisCamPracticeDlg::OnBnClickedClosebtn)
	ON_BN_CLICKED(IDC_NORPLAYBTN, &CCrevisCamPracticeDlg::OnBnClickedNorplaybtn)
	ON_BN_CLICKED(IDC_NORSTOPBTN, &CCrevisCamPracticeDlg::OnBnClickedNorstopbtn)
	ON_BN_CLICKED(IDC_ROIPLAYBTN, &CCrevisCamPracticeDlg::OnBnClickedRoiplaybtn)
	ON_BN_CLICKED(IDC_ROISTOPBTN, &CCrevisCamPracticeDlg::OnBnClickedRoistopbtn)
	ON_BN_CLICKED(IDC_BINPLAYBTN, &CCrevisCamPracticeDlg::OnBnClickedBinplaybtn)
	ON_BN_CLICKED(IDC_BINSTOPBTN, &CCrevisCamPracticeDlg::OnBnClickedBinstopbtn)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CCrevisCamPracticeDlg �޽��� ó����

BOOL CCrevisCamPracticeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// �ý��� �޴��� "����..." �޴� �׸��� �߰��մϴ�.

	// IDM_ABOUTBOX�� �ý��� ��� ������ �־�� �մϴ�.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// �� ��ȭ ������ �������� �����մϴ�.  ���� ���α׷��� �� â�� ��ȭ ���ڰ� �ƴ� ��쿡��
	//  �����ӿ�ũ�� �� �۾��� �ڵ����� �����մϴ�.
	SetIcon(m_hIcon, TRUE);			// ū �������� �����մϴ�.
	SetIcon(m_hIcon, FALSE);		// ���� �������� �����մϴ�.

	// TODO: ���⿡ �߰� �ʱ�ȭ �۾��� �߰��մϴ�.

	if (ST_InitSystem() != MCAM_ERR_SUCCESS)
	{
		AfxMessageBox(_T("System Initializing Failed!"), MB_OK|MB_ICONSTOP);
	}

	m_DeviceIPControl.SetWindowTextW(_T("192.168.20.2"));
	m_TBRoiX.SetWindowTextW(_T("20"));
	m_TBRoiY.SetWindowTextW(_T("20"));
	m_TBWidth.SetWindowTextW(_T("200"));
	m_TBRoiHeight.SetWindowTextW(_T("200"));
	m_TBThreshold.SetWindowTextW(_T("125"));

	static CClientDC norDc(GetDlgItem(IDC_NOR_DISP));
	static CClientDC binDc(GetDlgItem(IDC_BIN_DISP));
	static CClientDC roiDc(GetDlgItem(IDC_ROI_DISP));
	static CClientDC rectDc(GetDlgItem(IDC_NOR_DISP));
	// dc init

	m_hDC[NORMAL_VID] = norDc.GetSafeHdc();
	m_hDC[BINARY_VID] = binDc.GetSafeHdc();
	m_hDC[ROI_VID] = roiDc.GetSafeHdc();
	// handle init

	GetDlgItem(IDC_NOR_DISP)->GetWindowRect(m_rcDisp[NORMAL_VID]);
	GetDlgItem(IDC_ROI_DISP)->GetWindowRect(m_rcDisp[ROI_VID]);
	GetDlgItem(IDC_BIN_DISP)->GetWindowRect(m_rcDisp[BINARY_VID]);
	// rect info init

	m_pGraphics[NORMAL_VID] = Graphics::FromHDC(m_hDC[NORMAL_VID]);
	m_pGraphics[BINARY_VID] = Graphics::FromHDC(m_hDC[BINARY_VID]);
	m_pGraphics[ROI_VID] = Graphics::FromHDC(m_hDC[ROI_VID]);
	// get gdiplus handle

	GetDlgItem(IDC_OPENBTN)->EnableWindow(TRUE);
	GetDlgItem(IDC_NORPLAYBTN)->EnableWindow(FALSE);
	GetDlgItem(IDC_ROIPLAYBTN)->EnableWindow(FALSE);
	GetDlgItem(IDC_BINPLAYBTN)->EnableWindow(FALSE);
	GetDlgItem(IDC_NORSTOPBTN)->EnableWindow(FALSE);
	GetDlgItem(IDC_ROISTOPBTN)->EnableWindow(FALSE);
	GetDlgItem(IDC_BINSTOPBTN)->EnableWindow(FALSE);
	GetDlgItem(IDC_CLOSEBTN)->EnableWindow(FALSE);

/////////////////////////////////////////////
	MemoryLeakCheck();
return TRUE;  // ��Ŀ���� ��Ʈ�ѿ� �������� ������ TRUE�� ��ȯ�մϴ�.
}



void CCrevisCamPracticeDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// ��ȭ ���ڿ� �ּ�ȭ ���߸� �߰��� ��� �������� �׸�����
//  �Ʒ� �ڵ尡 �ʿ��մϴ�.  ����/�� ���� ����ϴ� MFC ���� ���α׷��� ��쿡��
//  �����ӿ�ũ���� �� �۾��� �ڵ����� �����մϴ�.

void CCrevisCamPracticeDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // �׸��⸦ ���� ����̽� ���ؽ�Ʈ�Դϴ�.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Ŭ���̾�Ʈ �簢������ �������� ����� ����ϴ�.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// �������� �׸��ϴ�.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		for (int i = 0; i < TOTAL_DISP; i++)
		{
			if (m_IsPlay[i] && m_pBitmap[i])
			{
				DrawImage(i);
			}
		} // ���÷��� flag Ȯ�� �� TRUE�� ���÷��̿� ���� ����

		CDialogEx::OnPaint();
	}
	MemoryLeakCheck();
}

// ����ڰ� �ּ�ȭ�� â�� ���� ���ȿ� Ŀ���� ǥ�õǵ��� �ý��ۿ���
//  �� �Լ��� ȣ���մϴ�.
HCURSOR CCrevisCamPracticeDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
} 

void CCrevisCamPracticeDlg::MemoryLeakCheck() {	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); } 
// if memory leaks, it shows on the debug output window.

void CCrevisCamPracticeDlg::OnBnClickedOpenbtn()
{
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	INT32 status = MCAM_ERR_SUCCESS;
	UINT32 bufsize = 40;
	UINT32 camNum = 0;
	CString availableIPstr;
	CString inputIP;
	CString deviceIPCstr;
	CString strErr;
	char * deviceIp = new char[bufsize];
	if (m_deviceEnum == DEVICE_DISCONNECTED)
	{
		ST_AcqStop(m_hDevice);
		ST_CloseDevice(m_hDevice);
	}
	m_deviceEnum = DEVICE_CAMNUM_INIT;
	m_DeviceIPControl.GetWindowTextW(inputIP);
	// ���� �Է� IP�� �޴� �κ�

	// for when device is disconnected and than connected
	status = ST_UpdateDevice();
	if (status != MCAM_ERR_SUCCESS)
	{
		strErr.Format(_T("Device Update Fail! : %d"), status);
		AfxMessageBox(strErr, MB_OK | MB_ICONSTOP);
		delete[] deviceIp;
		deviceIp = NULL;
		return;
	} // Device Update : updates list of devices.
	status = ST_GetAvailableCameraNum(&camNum);
	if (status != MCAM_ERR_SUCCESS)
	{
		strErr.Format(_T("No Camera Available! Check connection."));
		AfxMessageBox(strErr, MB_OK | MB_ICONSTOP);
		delete[] deviceIp;
		deviceIp = NULL;
		return;
	} // get numbers of connected cams and check if at least 1 is connected
	
	for (unsigned int i = 0; i < camNum; i++)
	{
		status = ST_GetEnumDeviceInfo(i, MCAM_DEVICEINFO_IP_ADDRESS, deviceIp, &bufsize);
		if (status != MCAM_ERR_SUCCESS)
		{
			strErr.Format(_T("IP Address retrieving fail : Check network configuration! || code: %d"), status);
			AfxMessageBox(strErr, MB_OK | MB_ICONSTOP);
			delete[] deviceIp;
			deviceIp = NULL;
			return;
		}
		// getting IP addresses of all connected devices.
		else
		{
			deviceIPCstr = deviceIp;
			if (deviceIPCstr == inputIP)
			{
				m_deviceEnum = i;
			}
		}
		// compares with user input, and if matches sets enum to get handle of the device.
	}

	if (m_deviceEnum < 0)
	{
		strErr = (_T("Device not found with your input IP.\nAvailable IP list is : \n"));
		for (unsigned int i = 0; i < camNum; i++)
		{
			status = ST_GetEnumDeviceInfo(i, MCAM_DEVICEINFO_IP_ADDRESS, deviceIp, &bufsize);
			if (status != MCAM_ERR_SUCCESS)
			{
				strErr.Format(_T("IP Address retrieving fail : Check network configuration! || code: %d"), status);
				AfxMessageBox(strErr, MB_OK | MB_ICONSTOP);
				delete[] deviceIp;
				deviceIp = NULL;
				return;
			}
			availableIPstr = deviceIp;
			availableIPstr += _T("\n");
			strErr += availableIPstr;
		}
		AfxMessageBox(strErr);
		delete[] deviceIp;
		deviceIp = NULL;
		return;
	} // if no device found that matches with user IP input
	// Checks with user input IP address if a device has same IP address setting with user input
	
	status = ST_OpenDevice(m_deviceEnum, &m_hDevice);
	if (status != MCAM_ERR_SUCCESS)
	{
		strErr.Format(_T("Device Open Fail! : %d"), status);
		AfxMessageBox(strErr, MB_OK | MB_ICONSTOP);
		return;
	}

	SetFeature();

	//Get Width
	status = ST_GetIntReg(m_hDevice, MCAM_WIDTH, &m_Width);
	if (status != MCAM_ERR_SUCCESS)
	{
		strErr.Format(_T("Read Register failed : %d"), status);
		AfxMessageBox(strErr, MB_OK | MB_ICONSTOP);
		return;
	}

	//Get Height
	status = ST_GetIntReg(m_hDevice, MCAM_HEIGHT, &m_Height);
	if (status != MCAM_ERR_SUCCESS)
	{
		strErr.Format(_T("Read Register failed : %d"), status);
		AfxMessageBox(strErr, MB_OK | MB_ICONSTOP);
		return;
	}

	// Image Buffer Allocatation
	m_BufferSize = m_Width * m_Height;
	for (int i = 0; i < TOTAL_DISP; i++)
	{
		m_pBitmap[i] = new Bitmap(m_Width, m_Height, PixelFormat8bppIndexed);
		m_pImage[i] = (char *)malloc(m_BufferSize);
		m_pGraphics[i]->DrawImage(m_pBitmap[i], 0, 0, m_Width, m_Height);
		memset((char *)m_pImage[i], 0, m_BufferSize);
	}

	m_pOriImage = (char *)malloc(m_BufferSize);
	memset((char *)m_pOriImage, 0, m_BufferSize);
	status = ST_AcqStart(m_hDevice);

	if (status != MCAM_ERR_SUCCESS)
	{
		strErr.Format(_T("Acquisition start failed : %d"), status);
		AfxMessageBox(strErr, MB_OK | MB_ICONSTOP);
		return;
	}
	// Camera acquitision starts when camera is been opened.

	ResetEvent(m_hGrabTerminate); // terminate thread for normal video streaming signal off
	m_hGrabThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReceiveThreadProc, this, 0, NULL);
	// Image Grab Starts here.
	AfxMessageBox(_T("Camera open complete! Acquisition started."));

	GetDlgItem(IDC_OPENBTN)->EnableWindow(FALSE);
	GetDlgItem(IDC_CLOSEBTN)->EnableWindow(TRUE);
	GetDlgItem(IDC_BINPLAYBTN)->EnableWindow(TRUE);
	GetDlgItem(IDC_NORPLAYBTN)->EnableWindow(TRUE);
	GetDlgItem(IDC_ROIPLAYBTN)->EnableWindow(TRUE);

	delete[] deviceIp;
	deviceIp = NULL;
	m_IsOpened = TRUE;
	SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED | ES_CONTINUOUS);
	// disabling power saver mode. it causes error.

	MemoryLeakCheck();
} // When "Open" button click

void CCrevisCamPracticeDlg::OnBnClickedClosebtn()
{
	INT32 status = MCAM_ERR_SUCCESS;
	CString strErr;
	for (int i = 0; i < TOTAL_DISP; i++)
	{
		if (m_hThread[i] != NULL)
		{
			SetEvent(m_hTerminate[i]);
			WaitForSingleObject(m_hThread[i], INFINITE);
			CloseHandle(m_hThread[i]);
			m_hThread[i] = NULL;
		}// stops and frees threads.
		m_IsPlay[i] = FALSE;
	}

	if (m_hGrabThread != NULL)
	{
		SetEvent(m_hGrabTerminate);
		WaitForSingleObject(m_hGrabThread, INFINITE);
		CloseHandle(m_hGrabThread);
		m_hGrabThread = NULL;
	}

	status = ST_AcqStop(m_hDevice);
	if (status != MCAM_ERR_SUCCESS)
	{
		strErr.Format(_T("Acquisition stop failed : %d"), status);
		AfxMessageBox(strErr, MB_OK | MB_ICONSTOP);
		return;
	}
	// Camera acquisition stops
	for (int i = 0; i < TOTAL_DISP; i++)
	{
		if (m_pImage[i] != NULL)
		{
			free(m_pImage[i]);
			m_pImage[i] = NULL;
		}
		delete[] m_pBitmap[i];
	}

	if (m_pOriImage != NULL)
	{
		free(m_pOriImage);
		m_pOriImage = NULL;
	}

	if (m_pROIImage != NULL)
	{
		free(m_pROIImage);
		m_pROIImage = NULL;
	}

	// image buffer free and nullification
	ST_CloseDevice(m_hDevice);

	GetDlgItem(IDC_OPENBTN)->EnableWindow(TRUE);
	GetDlgItem(IDC_NORPLAYBTN)->EnableWindow(FALSE);
	GetDlgItem(IDC_ROIPLAYBTN)->EnableWindow(FALSE);
	GetDlgItem(IDC_BINPLAYBTN)->EnableWindow(FALSE);
	GetDlgItem(IDC_NORSTOPBTN)->EnableWindow(FALSE);
	GetDlgItem(IDC_ROISTOPBTN)->EnableWindow(FALSE);
	GetDlgItem(IDC_BINSTOPBTN)->EnableWindow(FALSE);
	GetDlgItem(IDC_CLOSEBTN)->EnableWindow(FALSE);
	
	m_IsOpened = FALSE;
	SetThreadExecutionState(ES_CONTINUOUS);
	MemoryLeakCheck();
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
} // when "Close" button click


void CCrevisCamPracticeDlg::OnBnClickedNorplaybtn()
{
	DWORD threadName = NORMAL_VID;
	m_IsPlay[NORMAL_VID] = TRUE;

	ResetEvent(m_hTerminate[NORMAL_VID]); // terminate thread for normal video streaming signal off
	m_hThread[NORMAL_VID] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)NorThreadProc, this, 0, &threadName);

	GetDlgItem(IDC_NORPLAYBTN)->EnableWindow(FALSE);
	GetDlgItem(IDC_NORSTOPBTN)->EnableWindow(TRUE);
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	MemoryLeakCheck();
}// when normal "play" button click


void CCrevisCamPracticeDlg::OnBnClickedNorstopbtn()
{
	if (m_hThread[NORMAL_VID] != NULL)
	{
		SetEvent(m_hTerminate[NORMAL_VID]);
		WaitForSingleObject(m_hThread[NORMAL_VID], INFINITE);
		CloseHandle(m_hThread[NORMAL_VID]);
		m_hThread[NORMAL_VID] = NULL;
	}

	m_IsPlay[NORMAL_VID] = FALSE;
	GetDlgItem(IDC_NORPLAYBTN)->EnableWindow(TRUE);
	GetDlgItem(IDC_NORSTOPBTN)->EnableWindow(FALSE);
	MemoryLeakCheck();
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
} // when normal "stop" button click

void CCrevisCamPracticeDlg::OnBnClickedRoiplaybtn()
{
	
	CString CroiX, CroiY, CroiHeight, CroiWidth;
	CString strErr;
	DWORD threadName = ROI_VID;
	m_TBRoiX.GetWindowTextW(CroiX);
	m_roiX = _ttoi(CroiX);
	m_TBRoiY.GetWindowTextW(CroiY);
	m_roiY = _ttoi(CroiY);
	m_TBRoiHeight.GetWindowTextW(CroiHeight);
	m_roiHeight = _ttoi(CroiHeight);
	m_TBWidth.GetWindowTextW(CroiWidth);
	m_roiWidth = _ttoi(CroiWidth);
	// Getting & setting coord and height, width info to designate ROI info

	if (m_roiX < 0 || m_roiY < 0 || m_roiHeight <= 0 || m_roiWidth <= 0)
	{
		AfxMessageBox(_T("Plase check ROI input : no value allowed under 0"));
		return;
	}

	if (m_roiX >= m_rcDisp[NORMAL_VID].Width() || 
		m_roiY >= m_rcDisp[NORMAL_VID].Height())
	{
		strErr.Format(_T("Coordinate out of bound || X MAX : %d / Y MAX : %d"), 
			m_rcDisp[NORMAL_VID].Width(), m_rcDisp[NORMAL_VID].Height());
		AfxMessageBox(strErr);
		return;
	}
	
	if ((m_roiWidth + m_roiX) > m_rcDisp[NORMAL_VID].Width()
		|| (m_roiHeight + m_roiY) > m_rcDisp[NORMAL_VID].Height())
	{
		strErr.Format(_T("Length out of bound || Width MAX : %d / Height MAX : %d"),
			m_rcDisp[NORMAL_VID].Width() - m_roiX, m_rcDisp[NORMAL_VID].Height() - m_roiY);
		AfxMessageBox(strErr);
		return;
	}

	delete[] m_pBitmap[ROI_VID];
	// to assign ROI image size bitmap buffer
	m_pROIImage = (char *)malloc(m_roiWidth * m_roiHeight);
	m_pBitmap[ROI_VID] = new Bitmap(m_roiWidth, m_roiHeight, PixelFormat8bppIndexed);
	m_pGraphics[ROI_VID]->DrawImage(m_pBitmap[ROI_VID], 0, 0, m_roiWidth, m_roiHeight);
	memset((char *)m_pROIImage, 0, m_roiWidth * m_roiHeight);

	m_IsPlay[ROI_VID] = TRUE;
	
	ResetEvent(m_hTerminate[ROI_VID]); // terminate thread for normal video streaming signal off
	m_hThread[ROI_VID] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ROIThreadProc, this, 0, &threadName);
	
	GetDlgItem(IDC_ROIPLAYBTN)->EnableWindow(FALSE);
	GetDlgItem(IDC_ROISTOPBTN)->EnableWindow(TRUE);
	MemoryLeakCheck();
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
} // when roi "play" button click



void CCrevisCamPracticeDlg::OnBnClickedRoistopbtn()
{
	if (m_hThread[ROI_VID] != NULL)
	{
		SetEvent(m_hTerminate[ROI_VID]);
		WaitForSingleObject(m_hTerminate[ROI_VID], INFINITE);
		CloseHandle(m_hThread[ROI_VID]);
		m_hThread[ROI_VID] = NULL;
	}
	if (m_pROIImage != NULL)
	{
		free(m_pROIImage);
		m_pROIImage = NULL;
	}

	m_IsPlay[ROI_VID] = FALSE;
	GetDlgItem(IDC_ROIPLAYBTN)->EnableWindow(TRUE);
	GetDlgItem(IDC_ROISTOPBTN)->EnableWindow(FALSE);
	MemoryLeakCheck();
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
}// when roi "stop" button click


void CCrevisCamPracticeDlg::OnBnClickedBinplaybtn()
{
	CString CThreshold;
	DWORD threadName = BINARY_VID;

	m_TBThreshold.GetWindowTextW(CThreshold);
	m_Threshold = _ttoi(CThreshold);
	// Getting threshold value
	if (m_Threshold < 0 || m_Threshold > 255)
	{
		AfxMessageBox(TEXT("Please input a number between 0~255"));
		return;
	} // Threashold error value check

	
	ResetEvent(m_hTerminate[BINARY_VID]); // terminate thread for normal video streaming signal off
	m_hThread[BINARY_VID] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BinThreadProc, this, 0, &threadName);
	m_IsPlay[BINARY_VID] = TRUE;

	GetDlgItem(IDC_BINPLAYBTN)->EnableWindow(FALSE);
	GetDlgItem(IDC_BINSTOPBTN)->EnableWindow(TRUE);
	MemoryLeakCheck();
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
}// when binary "play" button click


void CCrevisCamPracticeDlg::OnBnClickedBinstopbtn()
{

	if (m_hThread[BINARY_VID] != NULL)
	{
		SetEvent(m_hTerminate[BINARY_VID]);
		WaitForSingleObject(m_hThread[BINARY_VID], INFINITE);
		CloseHandle(m_hThread[BINARY_VID]);
		m_hThread[BINARY_VID] = NULL;
	}
	m_IsPlay[BINARY_VID] = FALSE;

	GetDlgItem(IDC_BINPLAYBTN)->EnableWindow(TRUE);
	GetDlgItem(IDC_BINSTOPBTN)->EnableWindow(FALSE);
	MemoryLeakCheck();
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
}// when binary "play" button click


void CCrevisCamPracticeDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	INT32 status = MCAM_ERR_SUCCESS;
	CString strErr;
	// cycles on m_hThreadArr and checks if any thread is being used, and terminates
	// sets interrupt, wait for thread to be halted then close handle & frees pointer.
	if (m_IsOpened == TRUE)
	{
		for (int i = 0; i < TOTAL_DISP; i++){
			if (m_hThread[i] != NULL)
			{
				SetEvent(m_hTerminate[i]);
				WaitForSingleObject(m_hThread[i], INFINITE);
				CloseHandle(m_hThread[i]);
				m_hThread[i] = NULL;
			}
		}
		if (m_hGrabThread != NULL)
		{
			SetEvent(m_hGrabTerminate);
			WaitForSingleObject(m_hGrabThread, INFINITE);
			CloseHandle(m_hGrabThread);
			m_hGrabThread = NULL;
		}
		
		status = ST_AcqStop(m_hDevice);
		if (status != MCAM_ERR_SUCCESS)
		{
			strErr.Format(_T("Acquisition stop fail. code :%d"), status);
			AfxMessageBox(strErr, MB_OK | MB_ICONSTOP);
			return;
		}

		for (int i = 0; i < TOTAL_DISP; i++){
			if (m_pImage[i] != NULL)
			{
				free(m_pImage[i]);
				m_pImage[i] = NULL;
			}
			delete[] m_pBitmap[i];
		}

		if (m_pOriImage != NULL)
		{
			free(m_pOriImage);
			m_pOriImage = NULL;
		}

		if (m_pROIImage != NULL)
		{
			free(m_pROIImage);
			m_pROIImage = NULL;
		}

		
		// image grab buffer free & nullification
		ST_CloseDevice(m_hDevice);
	}
	// camera close procedure
	ST_FreeSystem();
	::DeleteCriticalSection(&mSc);
	SetThreadExecutionState(ES_CONTINUOUS);
	MemoryLeakCheck();
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
} 

void CCrevisCamPracticeDlg::SetFeature()
{
	INT32 status = MCAM_ERR_SUCCESS;
	CString strErr;

	status = ST_SetEnumReg(m_hDevice, MCAM_TRIGGER_MODE, TRIGGER_MODE_OFF);
	if (status != MCAM_ERR_SUCCESS)
	{
		strErr.Format(_T("Write Register failed : %d"), status);
		AfxMessageBox(strErr, MB_OK | MB_ICONSTOP);
	}
	// Trigger mode off!
	MemoryLeakCheck();
}


void CCrevisCamPracticeDlg::ReceiveImage()
{
	INT32 status = MCAM_ERR_SUCCESS;

	while (WaitForSingleObject(m_hGrabTerminate, 0) != WAIT_OBJECT_0)
	{
		status = ST_GrabImage(m_hDevice, m_pOriImage, m_BufferSize);

		::EnterCriticalSection(&mSc);
			if (status == MCAM_ERR_SUCCESS)
			{
				for (int dispidx = 0; dispidx < TOTAL_DISP; dispidx++)
				{
					memcpy(m_pImage[dispidx], m_pOriImage, m_BufferSize);
					
				
					if (dispidx == BINARY_VID){
						// if each display is on play, copies original image to buffers
						for (int bufidx = 0; bufidx < m_BufferSize; bufidx++)
						{
							if (m_pImage[BINARY_VID][bufidx] >= m_Threshold)
							{
								m_pImage[BINARY_VID][bufidx] = 255;
							}
							else
							{
								m_pImage[BINARY_VID][bufidx] = 0;
							}
						} // Algorithm for thresholding image
					}
					else if (dispidx == ROI_VID && m_IsPlay[ROI_VID])
					{
						int stpt = (m_roiY * m_Width) + m_roiX;
						int CpyIndex = 0;

						for (int i = 0; i < m_roiHeight; i++)
						{
							memcpy(&m_pROIImage[CpyIndex], &m_pImage[ROI_VID][stpt], m_roiWidth);
							CpyIndex += m_roiWidth;
							stpt += m_Width;
						}
					} // Algorithm for ROI image

				}
			}
			else
			{
				AfxMessageBox(_T("Image receiving fail!"));
				for (int i = 0; i < TOTAL_DISP; i++)
				{
					m_IsPlay[i] = FALSE;
					CloseHandle(m_hThread[i]);
					m_hThread[i] = NULL;
					if (m_pImage[i] != NULL)
						{
							free(m_pImage[i]);
							m_pImage[i] = NULL;
							// image buffer free and nullification
						}
				} // turning off all displays
				if (m_pOriImage != NULL)
				{
					free(m_pOriImage);
					m_pOriImage = NULL;
				}

				GetDlgItem(IDC_OPENBTN)->EnableWindow(TRUE);
				GetDlgItem(IDC_NORPLAYBTN)->EnableWindow(FALSE);
				GetDlgItem(IDC_ROIPLAYBTN)->EnableWindow(FALSE);
				GetDlgItem(IDC_BINPLAYBTN)->EnableWindow(FALSE);
				GetDlgItem(IDC_NORSTOPBTN)->EnableWindow(FALSE);
				GetDlgItem(IDC_ROISTOPBTN)->EnableWindow(FALSE);
				GetDlgItem(IDC_BINSTOPBTN)->EnableWindow(FALSE);
				GetDlgItem(IDC_CLOSEBTN)->EnableWindow(FALSE);
				m_IsOpened = FALSE;
				m_deviceEnum = DEVICE_DISCONNECTED;

				SetThreadExecutionState(ES_CONTINUOUS);
				return;
		}
		::LeaveCriticalSection(&mSc);
	}
} // Grab once, copies into other buffers. and calls image processing functions here.

void CCrevisCamPracticeDlg::ProcessNormalImage()
{
	if (!m_IsOpened)
	{
		return;
	}
	while (WaitForSingleObject(m_hTerminate[NORMAL_VID], 0) != WAIT_OBJECT_0)
	{
		Sleep(10);
		// access to the memory which contains image from device
		// and changes it to 0 or 255 on m_Threshold
		InvalidateRect(m_rcDisp[NORMAL_VID], NULL);
	}
}


void CCrevisCamPracticeDlg::ProcessROIImage()
{
	if (!m_IsOpened)
	{
		return;
	}
	while (WaitForSingleObject(m_hTerminate[ROI_VID], 0) != WAIT_OBJECT_0)
	{
		
		Sleep(10);
		InvalidateRect(m_rcDisp[ROI_VID], NULL);
	}
	// grab image by and sends to display which has vidType id.
}


void CCrevisCamPracticeDlg::ProcessBinImage()
{
	if (!m_IsOpened)
	{
		return;
	}
	while (WaitForSingleObject(m_hTerminate[BINARY_VID], 0) != WAIT_OBJECT_0)
	{
		Sleep(10);
		InvalidateRect(m_rcDisp[BINARY_VID], NULL);
	}
	// grab image by and sends to display which has vidType id.
	MemoryLeakCheck();
}
void CCrevisCamPracticeDlg::DrawImage(int vidType)
{
	::EnterCriticalSection(&mSc);
	if (!m_IsOpened)
	{
		return;
	}
	BitmapData bitmapdata;
	Rect rc(0, 0, m_Width, m_Height);

	Rect roirc(m_roiX, m_roiY, m_roiWidth, m_roiHeight); // rectangular for ROI section square on normal video
	Rect dispRc(0, 0, m_rcDisp[vidType].Width(), m_rcDisp[vidType].Height());
	Pen pen(Color(255, 0, 255), 5);
	Graphics mDC(m_pBitmap[NORMAL_VID]);
	
	// if video is not ROI just change color of the video and streams
	if (vidType != ROI_VID)
	{
		m_pBitmap[vidType]->LockBits(&rc, 0, PixelFormat8bppIndexed, &bitmapdata);
		memcpy(bitmapdata.Scan0, m_pImage[vidType], m_BufferSize);
		m_pBitmap[vidType]->UnlockBits(&bitmapdata);
	}
	else
	{
		rc = Rect(0, 0, m_roiWidth, m_roiHeight);
		m_pBitmap[ROI_VID]->LockBits(&rc, 0, PixelFormat8bppIndexed, &bitmapdata);
		memcpy(bitmapdata.Scan0, m_pROIImage, m_roiWidth * m_roiHeight);
		m_pBitmap[ROI_VID]->UnlockBits(&bitmapdata);
	}

	ConvertPalette(vidType);
	switch (vidType)
	{
	case NORMAL_VID:
		if (m_IsPlay[ROI_VID])
		{
			mDC.DrawRectangle(&pen, roirc);
			// to prevent double buffering issue
			// draws on the bitmap that has video
		}
		m_pGraphics[NORMAL_VID]->DrawImage(m_pBitmap[NORMAL_VID], dispRc, 0, 0, dispRc.Width, dispRc.Height, UnitPixel);
		break;

	case BINARY_VID:
		m_pGraphics[BINARY_VID]->DrawImage(m_pBitmap[BINARY_VID], dispRc, 0, 0, dispRc.Width, dispRc.Height, UnitPixel);
		break;

	case ROI_VID:
		m_pGraphics[ROI_VID]->DrawImage(m_pBitmap[ROI_VID], dispRc, 0, 0, m_roiWidth, m_roiHeight, UnitPixel);
		break;
	}
	::LeaveCriticalSection(&mSc);
	MemoryLeakCheck();
}

void CCrevisCamPracticeDlg::ConvertPalette(int vidType)
{
	int paletteSize = m_pBitmap[vidType]->GetPaletteSize();
	ColorPalette* pPalette = new ColorPalette[paletteSize];
	m_pBitmap[vidType]->GetPalette(pPalette, paletteSize);
	// gets palette info of bitmap image to set color info of the bitmap
	switch (vidType)
	{
	case NORMAL_VID:
	case ROI_VID:
		for (unsigned int i = 0; i < pPalette->Count; i++)
		{
			pPalette->Entries[i] = Color::MakeARGB(255, i, i, i);
		}
		m_pBitmap[vidType]->SetPalette(pPalette);
		break;
		// Normal video || ROI video color set
	case BINARY_VID:
		break;

	default:
		AfxMessageBox(TEXT("vidtype error : on converting palette!"));
		delete[] pPalette;
		return;
		break;
	}
	delete[] pPalette;
	MemoryLeakCheck();
}

