#include <new>
#include <windows.h>
#include <mfplay.h>
#include <Dbt.h>
#include <shlwapi.h>
#include <winnt.h>
#include <strsafe.h>

#include "Preview.h"

void ShowErrorMessage(HWND hwnd, PCWSTR format, HRESULT hr);

HRESULT CPreview::CreateInstance(HWND hVideo, CPreview** ppPlayer)
{
	if (ppPlayer == NULL)
	{
		return E_POINTER;
	}

	CPreview* pPlayer = new CPreview(hVideo);

	if (pPlayer == NULL)
	{
		return E_OUTOFMEMORY;
	}

	*ppPlayer = pPlayer;

	return S_OK;
}

CPreview::CPreview(HWND hVideo) : m_pPlayer(NULL), m_pSource(NULL), m_nRefCount(1), m_hwnd(hVideo), m_bHasVideo(FALSE)
{

}

CPreview::~CPreview()
{
	CloseDevice();
}

//------------------------------------------------------------------
// IUnknown Methods
// -----------------------------------------------------------------

HRESULT __stdcall CPreview::QueryInterface(REFIID riid, void** ppv)
{
	if (riid == IID_IUnknown)
		*ppv = static_cast<IMFPMediaPlayerCallback*>(this);
	else if(riid == __uuidof(IMFPMediaPlayerCallback))
		*ppv = static_cast<IMFPMediaPlayerCallback*>(this);
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	reinterpret_cast<IUnknown*>(this)->AddRef();

	return S_OK;
}

ULONG __stdcall CPreview::AddRef()
{
	return InterlockedIncrement(&m_nRefCount);
}

ULONG __stdcall CPreview::Release()
{
	ULONG uCount = InterlockedDecrement(&m_nRefCount);
	if (uCount == 0)
	{
		delete this;
	}

	return uCount;
}




VOID __stdcall CPreview::OnMediaPlayerEvent(MFP_EVENT_HEADER* pEventHeader)
{
	if (FAILED(pEventHeader->hrEvent))
	{
		ShowErrorMessage(NULL, L"Preview error.", pEventHeader->hrEvent);
		return;
	}

	switch (pEventHeader->eEventType)
	{
	case MFP_EVENT_TYPE_MEDIAITEM_CREATED:
		OnMediaItemCreated(MFP_GET_MEDIAITEM_CREATED_EVENT(pEventHeader));
		break;

	case MFP_EVENT_TYPE_MEDIAITEM_SET:
		OnMediaItemSet(MFP_GET_MEDIAITEM_SET_EVENT(pEventHeader));
		break;
	}
}

///// Class Methods

HRESULT CPreview::SetDevice(IMFActivate* pActivate)
{
	HRESULT hr = S_OK;

	IMFMediaSource* pSource = NULL;

	// Release the current instance of the player
	CloseDevice();

	// Create a new instance of the player
	hr = MFPCreateMediaPlayer(NULL,
				FALSE,
				0,
				this,
				m_hwnd,
				&m_pPlayer);

	// Create the media source for the device
	if (SUCCEEDED(hr))
	{
		hr = pActivate->ActivateObject(
						__uuidof(IMFMediaSource),
						(void**)&pSource);
	}

	// Create a new media item for this source
	if (SUCCEEDED(hr))
	{
		hr = m_pPlayer->CreateMediaItemFromObject(
			pSource,
			FALSE,
			0,
			NULL);
	}

	if (SUCCEEDED(hr))
	{
		m_pSource = pSource;
		m_pSource->AddRef();
	}

	if (FAILED(hr))
	{
		CloseDevice();
	}

	if (pSource)
	{
		pSource->Release();
	}

	return hr;
}

//-------------------------------------------------------------------
//  OnMediaItemCreated
//
//  Called when the IMFPMediaPlayer::SetMediaItem method completes.
//-------------------------------------------------------------------

void CPreview::OnMediaItemCreated(MFP_MEDIAITEM_CREATED_EVENT* pEvent)
{
	HRESULT hr = S_OK;

	if (m_pPlayer)
	{
		BOOL bHasVideo = FALSE, bIsSelected = FALSE;

		hr = pEvent->pMediaItem->HasVideo(&bHasVideo, &bIsSelected);

		if (SUCCEEDED(hr))
		{
			m_bHasVideo = bHasVideo && bIsSelected;

			hr = m_pPlayer->SetMediaItem(pEvent->pMediaItem);
		}
	}

	if (FAILED(hr))
	{
		ShowErrorMessage(NULL, L"Preview error.", hr);
	}
}

void CPreview::OnMediaItemSet(MFP_MEDIAITEM_SET_EVENT* pEvent)
{
	HRESULT hr = S_OK;

	SIZE szVideo = { 0 };
	RECT rc = { 0 };

	// Adjust the preview window to match the native size of the current window

	hr = m_pPlayer->GetNativeVideoSize(&szVideo, NULL);

	if (SUCCEEDED(hr))
	{
		SetRect(&rc, 0, 0, szVideo.cx, szVideo.cy);

		AdjustWindowRect(
			&rc,
			GetWindowLong(m_hwnd, GWL_STYLE),
			TRUE
			);

		SetWindowPos(m_hwnd, 0, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
			SWP_NOZORDER | SWP_NOMOVE | SWP_NOOWNERZORDER);

		hr = m_pPlayer->Play();
	}

	if (FAILED(hr))
	{
		ShowErrorMessage(NULL, L"Preview error.", hr);
	}
}

HRESULT CPreview::UpdateVideo()
{
	HRESULT hr = S_OK;

	if (m_pPlayer)
	{
		hr = m_pPlayer->UpdateVideo();
	}
	return hr;
}

HRESULT CPreview::CloseDevice()
{
	HRESULT hr = S_OK;

	if (m_pPlayer)
	{
		m_pPlayer->Shutdown();
		m_pPlayer->Release();
		m_pPlayer = NULL;
	}

	if (m_pSource)
	{
		m_pSource->Shutdown();
		m_pSource->Release();
		m_pSource = NULL;
	}

	m_bHasVideo = FALSE;

	return hr;
}