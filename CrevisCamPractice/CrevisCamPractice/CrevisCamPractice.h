
// CrevisCamPractice.h : PROJECT_NAME ���� ���α׷��� ���� �� ��� �����Դϴ�.
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH�� ���� �� ������ �����ϱ� ���� 'stdafx.h'�� �����մϴ�."
#endif

#include "resource.h"		// �� ��ȣ�Դϴ�.


// CCrevisCamPracticeApp:
// �� Ŭ������ ������ ���ؼ��� CrevisCamPractice.cpp�� �����Ͻʽÿ�.
//

class CCrevisCamPracticeApp : public CWinApp
{
public:
	CCrevisCamPracticeApp();

// �������Դϴ�.
public:
	virtual BOOL InitInstance();

// �����Դϴ�.

	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance();
};

extern CCrevisCamPracticeApp theApp;