// CameraApplication.cpp : Defines the entry point for the application.
//

#include <windows.h>
#include <windowsx.h>
#include <dbt.h>
#include <Ks.h>
#include <mfapi.h>
#include <Mfidl.h>
#include <mfobjects.h>
#include <strsafe.h>
#include "framework.h"
#include "CameraApplication.h"
#include "Preview.h"
#pragma comment(lib, "Mfplat.lib")
#pragma comment(lib, "Mf.lib")
#pragma comment(lib, "Mfplay.lib")

HDEVNOTIFY g_hdevnotify;

CPreview* g_pPreview;

struct ChooseDeviceParam
{
    IMFActivate** ppDevices;
    UINT32 count;
    UINT selection;
};

void ShowErrorMessage(HWND hwnd, PCWSTR format, HRESULT hr);


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

template <typename T> void SafeRelease(T** ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

// entry-point Function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int iCmdShow)
{
    // local variable declarations
    WNDCLASSEX wndclass;
    HWND hwnd;
    MSG msg;
    TCHAR szAppName[] = TEXT("RPMWindow");

    // initialize COM
    HRESULT hr = S_OK;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    // error handling code

    // code
    // WNDCLASSEX initialization
    wndclass.cbSize = sizeof(WNDCLASSEX);
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.lpfnWndProc = WndProc;
    wndclass.hInstance = hInstance;
    wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CAMERAAPPLICATION));;
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.lpszClassName = szAppName;
    wndclass.lpszMenuName = MAKEINTRESOURCE(IDC_CAMERAAPPLICATION);
    wndclass.hIconSm = LoadIcon(NULL, MAKEINTRESOURCE(IDI_CAMERAAPPLICATION));

    // Register WNDCLASSEX
    RegisterClassEx(&wndclass);

    // Create Window
    hwnd = CreateWindow(szAppName,
        TEXT("Renuka Prashant Mane"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL);

    // Show The Window
    ShowWindow(hwnd, iCmdShow);


    // Paint/Redraw the window
    UpdateWindow(hwnd);

    // Message Loop
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return((int)msg.wParam);
}

HRESULT OnInitDialog(HWND hwnd, ChooseDeviceParam* pParam)
{
    HRESULT hr = S_OK;

    HWND hList = GetDlgItem(hwnd, IDC_DEVICE_LIST);

    // DISPLAY LIST OF DEVICES

    for (DWORD i = 0; i < pParam->count; i++)
    {
        WCHAR* szFriendlyName = NULL;

        hr = pParam->ppDevices[i]->GetAllocatedString(
            MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
            &szFriendlyName,
            NULL
        );

        if (FAILED(hr))
        {
            break;
        }

        int index = ListBox_AddString(hList, szFriendlyName);

        ListBox_SetItemData(hList, index, i);

        CoTaskMemFree(szFriendlyName);
    }

    // Assume No selection
    pParam->selection = (UINT32)-1;

    if (pParam->count == 0)
    {
        EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
    }
    else
    {
        ListBox_SetCurSel(hList, 0);
    }

    return hr;
}


HRESULT OnOK(HWND hwnd, ChooseDeviceParam* pParam)
{
    HWND hList = GetDlgItem(hwnd, IDC_DEVICE_LIST);

    int sel = ListBox_GetCurSel(hList);

    if (sel != LB_ERR)
    {
        pParam->selection = (UINT)ListBox_GetItemData(hList, sel);
    }

    return S_OK;
}

INT_PTR CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static ChooseDeviceParam* pParam = NULL;

    switch (msg)
    {
    case WM_INITDIALOG:
        pParam = (ChooseDeviceParam*)lParam;
        OnInitDialog(hwnd, pParam);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            OnOK(hwnd, pParam);
            EndDialog(hwnd, LOWORD(wParam));
            return TRUE;

        case IDCANCEL:
            EndDialog(hwnd, LOWORD(wParam));
            return TRUE;
        }
        break;
    }
 
    return FALSE;

}



LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    // code
    switch (iMsg)
    {

    case WM_CREATE:
        {
        // contains info about a class of devices (dbt.h)
        DEV_BROADCAST_DEVICEINTERFACE di = { 0 };
        di.dbcc_size = sizeof(di);
        di.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        di.dbcc_classguid = KSCATEGORY_CAPTURE; //(Ks.h)

        g_hdevnotify = RegisterDeviceNotification(
           hwnd,
            &di,
            DEVICE_NOTIFY_WINDOW_HANDLE
        );

        if (g_hdevnotify == NULL)
        {
            MessageBox(NULL, TEXT("RegisterDeviceNotification Failed"), TEXT("Error"), MB_ICONERROR | MB_OK);
        }

        }
        break;

    case WM_CLOSE:
        if (g_hdevnotify)
        {
            UnregisterDeviceNotification(g_hdevnotify);
        }
        break;

    case WM_COMMAND:
        switch (wParam)
        {
        case ID_FILE_CHOOSEDEVICE:
        {
            HRESULT hr = S_OK;
            ChooseDeviceParam param = { 0 };

            IMFAttributes* pAttributes = NULL;

            if (g_pPreview)
            {
                g_pPreview->CloseDevice();
                SafeRelease(&g_pPreview);
            }

            hr = CPreview::CreateInstance(hwnd, &g_pPreview);


            // create an attribute store to specify the enumeration parameters
            
            if (SUCCEEDED(hr))
            {
                hr = MFCreateAttributes(&pAttributes, 1);
            }

            if (SUCCEEDED(hr))
            {
                hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                    MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
            }
            
            // Enumerate Devices

            if (SUCCEEDED(hr))
            {
                hr = MFEnumDeviceSources(pAttributes, &param.ppDevices, &param.count);
            }

            if (SUCCEEDED(hr))
            {
                INT_PTR result = DialogBoxParam(
                    GetModuleHandle(NULL),
                    MAKEINTRESOURCE(IDD_CHOOSE_DEVICE),
                    hwnd,
                    DlgProc,
                    (LPARAM)&param
                );

                if((result == IDOK) && (param.selection != (UINT32)-1))
                {
                    UINT iDevice = param.selection;

                    if (iDevice >= param.count)
                    {
                        hr = E_UNEXPECTED;
                    }
                    else
                    {
                        hr = g_pPreview->SetDevice(param.ppDevices[iDevice]);
                    }
                }
            }

            SafeRelease(&pAttributes);

            for (DWORD i = 0; i < param.count; i++)
            {
                SafeRelease(&param.ppDevices[i]);
            }
            CoTaskMemFree(param.ppDevices);

            if (FAILED(hr))
            {
                ShowErrorMessage(hwnd, L"Cannot create the video capture device", hr);
            }
        
        }
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        break;
    }

    return(DefWindowProc(hwnd, iMsg, wParam, lParam));
}


//--------------------------------------------------------

void ShowErrorMessage(HWND hwnd, PCWSTR format, HRESULT hrErr)
{
    HRESULT hr = S_OK;
    WCHAR msg[1024];

    hr = StringCbPrintf(msg, sizeof(msg), L"%s (hr=0x%X)", format, hrErr);

    if (SUCCEEDED(hr))
    {
        MessageBox(hwnd, msg, L"Error", MB_ICONERROR);
    }
}
