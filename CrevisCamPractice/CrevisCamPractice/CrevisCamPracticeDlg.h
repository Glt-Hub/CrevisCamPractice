
// CrevisCamPracticeDlg.h : ��� ����
//

#pragma once
#include "afxcmn.h"
#include "afxwin.h"

#define NORMAL_VID 0
#define BINARY_VID 1
#define ROI_VID 2
#define TOTAL_DISP 3

// CCrevisCamPracticeDlg ��ȭ ����
class CCrevisCamPracticeDlg : public CDialogEx
{
// �����Դϴ�.
public:
	CCrevisCamPracticeDlg(CWnd* pParent = NULL);	// ǥ�� �������Դϴ�.

// ��ȭ ���� �������Դϴ�.
	enum { IDD = IDD_CREVISCAMPRACTICE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV �����Դϴ�.


// �����Դϴ�.
protected:
	HICON m_hIcon;

	// ������ �޽��� �� �Լ�
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedMfcbutton1();
	afx_msg void OnBnClickedOpenbtn();
	afx_msg void OnBnClickedClosebtn();
	afx_msg void OnBnClickedNorplaybtn();
	afx_msg void OnBnClickedNorstopbtn();
	afx_msg void OnBnClickedRoiplaybtn();
	afx_msg void OnBnClickedRoistopbtn();
	afx_msg void OnBnClickedBinplaybtn();
	afx_msg void OnBnClickedBinstopbtn();
	afx_msg void OnDestroy();

public:
	void SetFeature();
	void ReceiveImage();
	void ConvertPalette(int vidType);
	void DrawImage(int vidType);
	void MemoryLeakCheck();

private:
	INT32		m_hDevice; // Device �ڵ�
	INT32		m_Width; 
	INT32		m_Height; // ���� ���� ��
	INT32		m_BufferSize; // �̹��� ���ۻ�����
	INT32		m_deviceEnum; // Device enum id �����
	void*		m_pImage; // �̹��� ���� ������
	BOOL		m_IsOpened; // ī�޶� ���¿��� bool flag
	BOOL		m_IsPlay[TOTAL_DISP]; // Ȱ��ȭ�� ��ũ���� bool flag
	UINT32		m_Threshold; // ����ȭ ���� threshold��
	INT32		m_roiX;
	INT32		m_roiY;
	INT32		m_roiWidth;
	INT32		m_roiHeight;
	// for videos

	CRect		m_rcDisp[TOTAL_DISP]; // �� ���÷��̿� Crect
	HDC			m_hDC[TOTAL_DISP]; // �� ���÷��̿� �ڵ�
	Bitmap*		m_pBitmap; 
	Graphics*	m_pGraphics[TOTAL_DISP]; 
	// GDI+

	HANDLE		m_hThread;
	HANDLE		m_hTerminate;
	
public:
	// Getting user input to find devcie wtih IP address
	CIPAddressCtrl m_DeviceIPControl;
	// User input of Threshold value for binary video
	CEdit m_TBThreshold;
	// Roi video X coord
	CEdit m_TBRoiX;
	// Roi Y coord
	CEdit m_TBRoiY;
	// Roi Width
	CEdit m_TBWidth;
	// Roi Height
	CEdit m_TBRoiHeight;
	afx_msg void OnStnClickedNorDisp();
};
