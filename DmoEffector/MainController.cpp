#include "stdafx.h"
#include "MainController.h"

#include "Device.h"

static log4cplus::Logger logger = log4cplus::Logger::getInstance(_T("MainController"));



CMainController::CMainController()
	: m_asioHandler(CAsioHandler::getInstance())
{
}


CMainController::~CMainController()
{
}

HRESULT CMainController::setup(IASIO* asio, HWND hwnd)
{
	HR_ASSERT_OK(m_asioHandler->setup(asio, hwnd));
	return S_OK;
}

HRESULT CMainController::shutdown()
{
	HR_EXPECT_OK(stop());
	HR_EXPECT_OK(m_asioHandler->shutdown());

	return S_OK;
}

HRESULT CMainController::start(CDevice * inputDevice, CDevice* outputDevice)
{
	CT2A in(inputDevice->getName());
	CT2A out(outputDevice->getName());
	LOG4CPLUS_INFO(logger, __FUNCTION__ "('" << (LPCSTR)in << "','" << (LPCSTR)out << "')");

	HR_ASSERT_OK(stop());

	HR_ASSERT_OK(m_asioHandler->start());

	return S_OK;
}

HRESULT CMainController::stop()
{
	HR_ASSERT_OK(m_asioHandler->stop());

	return S_OK;
}
