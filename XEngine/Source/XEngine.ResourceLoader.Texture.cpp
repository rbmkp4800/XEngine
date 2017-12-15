#include <wincodec.h>

#include <XLib.Platform.COMPtr.h>

#include "XEngine.ResourceLoader.h"
#include "XEngine.Render.Resources.h"

using namespace XLib::Platform;
using namespace XEngine;

// TODO: rewrite

static COMPtr<IWICImagingFactory> wicFactory;

void XEResourceLoader::LoadTextureFromFile(const wchar* filename, XERDevice* device, XERTexture* texture)
{
	if (!wicFactory.isInitialized())
	{
		CoInitialize(nullptr);
		CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
			wicFactory.uuid(), wicFactory.voidInitRef());
	}

	COMPtr<IWICBitmapDecoder> wicBitmapDecoder;
	COMPtr<IWICBitmapFrameDecode> wicBitmapFrameDecode;
	COMPtr<IWICFormatConverter> wicFormatConverter;

	wicFactory->CreateDecoderFromFilename(filename, nullptr,
		GENERIC_READ, WICDecodeMetadataCacheOnLoad, wicBitmapDecoder.initRef());
	wicBitmapDecoder->GetFrame(0, wicBitmapFrameDecode.initRef());
	wicFactory->CreateFormatConverter(wicFormatConverter.initRef());
	wicFormatConverter->Initialize(wicBitmapFrameDecode, GUID_WICPixelFormat32bppPRGBA,
		WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut);

	UINT width = 0, height = 0;
	wicFormatConverter->GetSize(&width, &height);

	void *data = malloc(width * height * 4);

	wicFormatConverter->CopyPixels(nullptr, width * 4, width * height * 4, (BYTE*)data);
	texture->initialize(device, data, width, height);

	free(data);
}