
#include <freerdp/gdi/gdi.h>

#include <freerdp/gdi/dc.h>
#include <freerdp/gdi/pen.h>
#include <freerdp/gdi/shape.h>
#include <freerdp/gdi/region.h>
#include <freerdp/gdi/bitmap.h>

#include <winpr/crt.h>
#include <winpr/print.h>

#include "line.h"
#include "brush.h"
#include "clipping.h"

static int test_gdi_PtInRect(void)
{
	int rc = -1;
	HGDI_RECT hRect = NULL;
	UINT32 left = 20;
	UINT32 top = 40;
	UINT32 right = 60;
	UINT32 bottom = 80;

	if (!(hRect = gdi_CreateRect(
	          WINPR_ASSERTING_INT_CAST(int, left), WINPR_ASSERTING_INT_CAST(int, top),
	          WINPR_ASSERTING_INT_CAST(int, right), WINPR_ASSERTING_INT_CAST(int, bottom))))
	{
		printf("gdi_CreateRect failed\n");
		return rc;
	}

	if (gdi_PtInRect(hRect, 0, 0))
		goto fail;

	if (gdi_PtInRect(hRect, 500, 500))
		goto fail;

	if (gdi_PtInRect(hRect, 40, 100))
		goto fail;

	if (gdi_PtInRect(hRect, 10, 40))
		goto fail;

	if (!gdi_PtInRect(hRect, 30, 50))
		goto fail;

	if (!gdi_PtInRect(hRect, WINPR_ASSERTING_INT_CAST(int, left),
	                  WINPR_ASSERTING_INT_CAST(int, top)))
		goto fail;

	if (!gdi_PtInRect(hRect, WINPR_ASSERTING_INT_CAST(int, right),
	                  WINPR_ASSERTING_INT_CAST(int, bottom)))
		goto fail;

	if (!gdi_PtInRect(hRect, WINPR_ASSERTING_INT_CAST(int, right), 60))
		goto fail;

	if (!gdi_PtInRect(hRect, 40, WINPR_ASSERTING_INT_CAST(int, bottom)))
		goto fail;

	rc = 0;
fail:
	gdi_DeleteObject((HGDIOBJECT)hRect);
	return rc;
}

static int test_gdi_FillRect(void)
{
	int rc = -1;
	HGDI_DC hdc = NULL;
	HGDI_RECT hRect = NULL;
	HGDI_BRUSH hBrush = NULL;
	HGDI_BITMAP hBitmap = NULL;
	UINT32 color = 0;
	UINT32 pixel = 0;
	UINT32 rawPixel = 0;
	UINT32 badPixels = 0;
	UINT32 goodPixels = 0;
	UINT32 width = 200;
	UINT32 height = 300;
	UINT32 left = 20;
	UINT32 top = 40;
	UINT32 right = 60;
	UINT32 bottom = 80;

	if (!(hdc = gdi_GetDC()))
	{
		printf("failed to get gdi device context\n");
		goto fail;
	}

	hdc->format = PIXEL_FORMAT_XRGB32;

	if (!(hRect = gdi_CreateRect(
	          WINPR_ASSERTING_INT_CAST(int, left), WINPR_ASSERTING_INT_CAST(int, top),
	          WINPR_ASSERTING_INT_CAST(int, right), WINPR_ASSERTING_INT_CAST(int, bottom))))
	{
		printf("gdi_CreateRect failed\n");
		goto fail;
	}

	hBitmap = gdi_CreateCompatibleBitmap(hdc, width, height);
	ZeroMemory(hBitmap->data, 1ULL * width * height * FreeRDPGetBytesPerPixel(hdc->format));
	gdi_SelectObject(hdc, (HGDIOBJECT)hBitmap);
	color = FreeRDPGetColor(PIXEL_FORMAT_ARGB32, 0xAA, 0xBB, 0xCC, 0xFF);
	hBrush = gdi_CreateSolidBrush(color);
	gdi_FillRect(hdc, hRect, hBrush);
	badPixels = 0;
	goodPixels = 0;

	for (UINT32 x = 0; x < width; x++)
	{
		for (UINT32 y = 0; y < height; y++)
		{
			rawPixel = gdi_GetPixel(hdc, x, y);
			pixel = FreeRDPConvertColor(rawPixel, hdc->format, PIXEL_FORMAT_ARGB32, NULL);

			if (gdi_PtInRect(hRect, WINPR_ASSERTING_INT_CAST(int, x),
			                 WINPR_ASSERTING_INT_CAST(int, y)))
			{
				if (pixel == color)
				{
					goodPixels++;
				}
				else
				{
					printf("actual:%08" PRIX32 " expected:%08" PRIX32 "\n", gdi_GetPixel(hdc, x, y),
					       color);
					badPixels++;
				}
			}
			else
			{
				if (pixel == color)
				{
					badPixels++;
				}
				else
				{
					goodPixels++;
				}
			}
		}
	}

	if (goodPixels != width * height)
		goto fail;

	if (badPixels != 0)
		goto fail;

	rc = 0;
fail:
	gdi_DeleteObject((HGDIOBJECT)hBrush);
	gdi_DeleteObject((HGDIOBJECT)hBitmap);
	gdi_DeleteObject((HGDIOBJECT)hRect);
	gdi_DeleteDC(hdc);
	return rc;
}

int TestGdiRect(int argc, char* argv[])
{
	WINPR_UNUSED(argc);
	WINPR_UNUSED(argv);

	if (test_gdi_PtInRect() < 0)
		return -1;

	if (test_gdi_FillRect() < 0)
		return -1;

	return 0;
}
