#include "directshow.h"

// see https://ffmpeg.zeranoe.com/forum/viewtopic.php?f=15&t=651&p=2963&hilit=enumerate#p2963

#include <Windows.h>
#include <dshow.h>
#include <initguid.h>

#pragma comment(lib, "strmiids")

DEFINE_GUID(CLSID_VideoInputDeviceCategory,0x860bb310,0x5d01,0x11d0,0xbd,0x3b,0x00,0xa0,0xc9,0x11,0xce,0x86);

QHash<QString, QString> directshow::getDeviceList()
{
    QHash<QString, QString>  deviceNames;
    ICreateDevEnum *pDevEnum;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum,0,CLSCTX_INPROC_SERVER,IID_ICreateDevEnum,(void**)&pDevEnum);
    if(FAILED(hr))
        return deviceNames;
    IEnumMoniker *pEnum;
    hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,&pEnum,0);
    if(FAILED(hr))
    {
        return deviceNames;
    }
    IMoniker *pMoniker = NULL;
    while(S_OK == pEnum->Next(1,&pMoniker,NULL))
    {
        IPropertyBag *pPropBag;
        LPOLESTR str = 0;
        hr = pMoniker->GetDisplayName(0,0,&str);
        if(SUCCEEDED(hr))
        {
            hr = pMoniker->BindToStorage(0,0,IID_IPropertyBag,(void**)&pPropBag);
            if(SUCCEEDED(hr))
            {
                VARIANT var;
                VariantInit(&var);
                hr = pPropBag->Read(L"FriendlyName",&var,0);
                QString fName = QString::fromWCharArray(var.bstrVal);
				deviceNames[QString("video=%1").arg(fName)] = fName;
            } 
        }
    }
	
	return deviceNames;
}

