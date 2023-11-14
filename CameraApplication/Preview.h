#pragma once

#include <mfplay.h>

class CPreview : public IMFPMediaPlayerCallback
{
public:
	static HRESULT CreateInstance(HWND hVideo, CPreview** ppPlayer);

	// IUnknown methods
	HRESULT __stdcall QueryInterface(REFIID iid, void** ppv);
	ULONG __stdcall AddRef();
	ULONG __stdcall Release();

	void __stdcall OnMediaPlayerEvent(MFP_EVENT_HEADER* pEventHeader);

	HRESULT SetDevice(IMFActivate* pActivate);
	HRESULT CloseDevice();
	HRESULT UpdateVideo();

protected:

	//Const is private. Use the static method to instantiate.
	CPreview(HWND hVideo);

	virtual ~CPreview();

	// Event Handlers
	void OnMediaItemCreated(MFP_MEDIAITEM_CREATED_EVENT* pEvent);
	void OnMediaItemSet(MFP_MEDIAITEM_SET_EVENT* pEvent);

protected:

	long m_nRefCount;

	IMFPMediaPlayer* m_pPlayer;

	IMFMediaSource* m_pSource;

	HWND m_hwnd;
	BOOL m_bHasVideo;
};
