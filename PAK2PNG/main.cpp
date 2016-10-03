/*
LodePNG Examples

Copyright (c) 2005-2010 Lode Vandevenne

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

/*
Load a BMP image and convert it to a PNG image. This example also shows how
to use other data with the same memory structure as BMP, such as the image
format native to win32, GDI (HBITMAP, BITMAPINFO, ...) often encountered if
you're programming for Windows in Visual Studio.

This example only supports uncompressed 24-bit RGB or 32-bit RGBA bitmaps.
For other types of BMP's, use a full fledged BMP decoder, or convert the
bitmap to 24-bit or 32-bit format.

NOTE: it overwrites the output file without warning if it exists!
*/

//g++ lodepng.cpp example_bmp2png.cpp -ansi -pedantic -Wall -Wextra -O3

#include "lodepng.h"

#include <iostream>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <Windows.h>
using namespace std;
//returns 0 if all went ok, non-0 if error
//output image is always given in RGBA (with alpha channel), even if it's a BMP without alpha channel
unsigned decodeBMP(std::vector<unsigned char>& image, unsigned& w, unsigned& h, const std::vector<unsigned char>& bmp, LPBITMAPINFO m_bmpInfo, long m_dwBitmapFileStartLoc)
{
	static const unsigned MINHEADER = 54; //minimum BMP header size

	if (bmp.size() < MINHEADER) return -1;
	if (bmp[0] != 'B' || bmp[1] != 'M') return 1; //It's not a BMP file if it doesn't start with marker 'BM'
	unsigned pixeloffset = bmp[10] + 256 * bmp[11] + 65536 * bmp[12] + bmp[13] * 16777216; //where the pixel data starts
													//read width and height from BMP header
	w = bmp[18] + bmp[19] * 256;
	h = bmp[22] + bmp[23] * 256;
	//read number of channels from BMP header
	if (bmp[28] != 24 && bmp[28] != 32 && bmp[28] != 8) return 2; //only 24-bit and 32-bit BMPs are supported. -> Added 8-bit
	unsigned numChannels = bmp[28] / 8;

	//The amount of scanline bytes is width of image times channels, with extra bytes added if needed
	//to make it a multiple of 4 bytes.
	unsigned scanlineBytes = w * numChannels;
	if (scanlineBytes % 4 != 0) scanlineBytes = (scanlineBytes / 4) * 4 + 4;

	unsigned dataSize = scanlineBytes * h;
	if (bmp.size() < dataSize + pixeloffset) return 3; //BMP file too small to contain all pixels

	image.resize(w * h * 4);

	/*
	There are 3 differences between BMP and the raw image buffer for LodePNG:
	-it's upside down
	-it's in BGR instead of RGB format (or BRGA instead of RGBA)
	-each scanline has padding bytes to make it a multiple of 4 if needed
	The 2D for loop below does all these 3 conversions at once.
	*/
	if (numChannels != 1)
		for (unsigned y = 0; y < h; y++)
			for (unsigned x = 0; x < w; x++)
			{
				//pixel start byte position in the BMP
				unsigned bmpos = pixeloffset + (h - y) * w + numChannels * x;
				//pixel start byte position in the new raw image
				unsigned newpos = 4 * y * w + 4 * x;
				if (numChannels == 3)
				{
					image[newpos + 0] = bmp[bmpos + 2]; //R
					image[newpos + 1] = bmp[bmpos + 1]; //G
					image[newpos + 2] = bmp[bmpos + 0]; //B
					image[newpos + 3] = 255;            //A
				}
				else
				{
					image[newpos + 0] = bmp[bmpos + 3]; //R
					image[newpos + 1] = bmp[bmpos + 2]; //G
					image[newpos + 2] = bmp[bmpos + 1]; //B
					image[newpos + 3] = bmp[bmpos + 0]; //A
				}
			}
	if (numChannels == 1)
		for (unsigned y = 0; y < h; y++)
			for (unsigned x = 0; x < w; x++)
			{
				//pixel start byte position in the BMP
				unsigned bmpos = m_dwBitmapFileStartLoc + pixeloffset + (h - y - 1) * scanlineBytes + x;
				
				//pixel start byte position in the new raw image
				unsigned newpos = 4 * y * w + 4 * x;

				cout << "m_dwBitmapFileStartLoc: " << m_dwBitmapFileStartLoc << " Pixeloffset: "<< pixeloffset << " BMPOS: "<< bmpos << endl;
			//	std::cout << "BMPos: "<< bmpos <<" R: " << (int)m_bmpInfo->bmiColors[bmpos].rgbRed << " G: " << (int)m_bmpInfo->bmiColors[bmpos].rgbGreen << " B: " << (int)m_bmpInfo->bmiColors[bmpos].rgbBlue << std::endl;

				image[newpos + 0] = m_bmpInfo->bmiColors[bmp[bmpos]].rgbRed; //R
				image[newpos + 1] = m_bmpInfo->bmiColors[bmp[bmpos]].rgbGreen; //G
				image[newpos + 2] = m_bmpInfo->bmiColors[bmp[bmpos]].rgbBlue; //B
				image[newpos + 3] = 255;//bmp[bmpos + 0]; //A
				//Sleep(3);

			}

	return 0;
}
/***********Spritevalue explanations:*************
24 = PAK Header

hPakFile = FileHandle
sNthFile = File number inside pak
iASDstart = Buffer(Integer), + 104 = m_iTotalFrame
nCount = Number of bytes read




*/

typedef struct stBrushtag
{
	short sx; //startX
	short sy; //startY
	short szx; //sizex
	short szy; //sizey
	short pvx; //correction for placement
	short pvy; //correction for placement
} stBrush;


unsigned decodePAK(std::vector<unsigned char>& image, unsigned& w, unsigned& h, CHAR PathName[28])
{
	DWORD  nCount;
	int iASDstart = 0;
	HANDLE hPakFile;
	int sNthFile = 0; // Number of file1
	long m_dwBitmapFileStartLoc;
	std::vector<unsigned char> pak;
	cout << PathName << endl;
	hPakFile = CreateFileA(PathName, GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
	if (hPakFile == INVALID_HANDLE_VALUE)
		cout << "CreateFile Error: " << GetLastError() << endl;
	if (hPakFile == NULL) {
		std::cout << "hPakFile is null" << std::endl;
	}
	stBrushtag *m_stBrush;
	int m_iTotalFrame = 0;

	SetFilePointer(hPakFile, 24 + sNthFile * 8, NULL, FILE_BEGIN);
	if (!ReadFile(hPakFile, &iASDstart, 4, &nCount, NULL)) std::cout << "ReadFile failed: " << GetLastError() << std::endl;
	//i+100       Sprite Confirm
	std::cout << iASDstart << std::endl;
	SetFilePointer(hPakFile, iASDstart + 100, NULL, FILE_BEGIN);
	ReadFile(hPakFile, &m_iTotalFrame, 4, &nCount, NULL);
	m_dwBitmapFileStartLoc = iASDstart + (108 + (12 * m_iTotalFrame));
	m_stBrush = new stBrush[m_iTotalFrame];
	ReadFile(hPakFile, m_stBrush, 12 * m_iTotalFrame, &nCount, NULL);

	BITMAPFILEHEADER fh; //bmp 

	SetFilePointer(hPakFile, m_dwBitmapFileStartLoc, NULL, FILE_BEGIN);
	ReadFile(hPakFile, (char *)&fh, 14, &nCount, NULL);//sizeof(bmpHeader)==14
	SetFilePointer(hPakFile, m_dwBitmapFileStartLoc, NULL, FILE_BEGIN);
	pak.resize(GetFileSize(hPakFile, NULL));
	ReadFile(hPakFile, &pak[0], fh.bfSize, &nCount, NULL);
	CloseHandle(hPakFile);
	LPBITMAPINFOHEADER bmpInfoHeader = (LPBITMAPINFOHEADER)&pak[14];
	LPBITMAPINFO m_bmpInfo = (LPBITMAPINFO)&pak[14];
	WORD m_wWidthX = (WORD)(bmpInfoHeader->biWidth);
	WORD m_wWidthY = (WORD)(bmpInfoHeader->biHeight);
	WORD m_wColorNums;
	if (bmpInfoHeader->biClrUsed == 0)
	{
		if (bmpInfoHeader->biBitCount == 24) m_wColorNums = 0;
		else if (bmpInfoHeader->biBitCount == 8) m_wColorNums = 256;
		else if (bmpInfoHeader->biBitCount == 1) m_wColorNums = 2;
		else if (bmpInfoHeader->biBitCount == 4) m_wColorNums = 16;
		else m_wColorNums = 0;
	}
	else m_wColorNums = (WORD)(bmpInfoHeader->biClrUsed);
	//	for (int i = 0; i < 256; i++)
		//	std::cout << "R: " << (int)m_bmpInfo->bmiColors[i].rgbRed << " G: " << (int)m_bmpInfo->bmiColors[i].rgbGreen << " B: " << (int)m_bmpInfo->bmiColors[i].rgbBlue << std::endl;


	return decodeBMP(image, w, h, pak, m_bmpInfo, m_dwBitmapFileStartLoc);
}



int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		std::cout << "Please provide input pak and output png file names" << std::endl;
		return 0;
	}

	std::vector<unsigned char> image;
	unsigned w, h;
	unsigned error = decodePAK(image, w, h, argv[1]);

	if (error)
	{
		std::cout << "PAK decoding error " << error << std::endl;
		return 0;
	}

	std::vector<unsigned char> png;
	error = lodepng::encode(png, image, w, h);

	if (error)
	{
		std::cout << "PNG encoding error " << error << ": " << lodepng_error_text(error) << std::endl;
		return 0;
	}

	lodepng::save_file(png, argv[2]);

}
