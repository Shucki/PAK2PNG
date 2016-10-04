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
This is an altered version of the bmp2png example from lodepng.
Load a PAK and unpack the BMP files inside and convert them to the PNG file format.
PAK header information will be stored in their respective JSON file.
NOTE: it overwrites the output file without warning if it exists!
*/

#include "pak2png.h"
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
unsigned decodeBMP(std::vector<unsigned char>& image, unsigned& w, unsigned& h, const std::vector<unsigned char>& bmp, LPBITMAPINFO headerDIBInformation)
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
	for (unsigned y = 0; y < h; y++)
		for (unsigned x = 0; x < w; x++)
		{
			//pixel start byte position in the BMP
			unsigned bmpos = pixeloffset + (h - y - 1) * scanlineBytes + numChannels * x;
			//pixel start byte position in the new raw image
			unsigned newpos = 4 * y * w + 4 * x;
			if (numChannels == 3)
			{
				image[newpos + 0] = bmp[bmpos + 2]; //R
				image[newpos + 1] = bmp[bmpos + 1]; //G
				image[newpos + 2] = bmp[bmpos + 0]; //B
				image[newpos + 3] = 255;            //A
			}
			else if (numChannels == 1) // Read the color out of the color table
			{
				image[newpos + 0] = headerDIBInformation->bmiColors[bmp[bmpos]].rgbRed; //R
				image[newpos + 1] = headerDIBInformation->bmiColors[bmp[bmpos]].rgbGreen; //G
				image[newpos + 2] = headerDIBInformation->bmiColors[bmp[bmpos]].rgbBlue; //B
				image[newpos + 3] = 255;
			}
			else
			{
				image[newpos + 0] = bmp[bmpos + 3]; //R
				image[newpos + 1] = bmp[bmpos + 2]; //G
				image[newpos + 2] = bmp[bmpos + 1]; //B
				image[newpos + 3] = bmp[bmpos + 0]; //A
			}

		}
	return 0;
}

/**
* Method:    decodePAK
* param image: output image
* param w: image width
* param h: image height
* pathName: filepath of pak file
* numberOfFile: which bmp file inside the PAK file to extract
* return: non-0 if error
*/
unsigned decodePAK(std::vector<unsigned char>& image, unsigned& w, unsigned& h, char pathName[28], int numberOfFile = 2)
{
	DWORD  readBytes; // Bytes read after readfile, usable for debugging information
	int spriteHeaderStart = 0; // Every bmp has a sprite header above
	BITMAPFILEHEADER bmpFileHeader; // File header of the bmp we read
	HANDLE pakFile;

	long bitmapFileStartLoc;
	std::vector<unsigned char> pak;

	// Create pakfile handle and check for success
	pakFile = CreateFileA(pathName, GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
	if (pakFile == INVALID_HANDLE_VALUE) {
		cout << "CreateFile Error: " << GetLastError() << endl;
		return 4;
	}
	if (pakFile == NULL) {
		std::cout << "hPakFile is null" << std::endl;
		return 4;
	}

	// Read frame information
	pngInformation pngInformation;
	pngInformation.imageFrames = 0;

	// Go to sprite header start bytes and read them
	SetFilePointer(pakFile, 24 + numberOfFile * 8, NULL, FILE_BEGIN);
	if (!ReadFile(pakFile, &spriteHeaderStart, 4, &readBytes, NULL)) std::cout << "ReadFile failed: " << GetLastError() << std::endl;
	
	// Sprite header start + 100 is where the information of how many frames are in the bitmap is stored.
	SetFilePointer(pakFile, spriteHeaderStart + 100, NULL, FILE_BEGIN);
	if(!ReadFile(pakFile, &pngInformation.imageFrames, 4, &readBytes, NULL)) std::cout << "ReadFile failed: " << GetLastError() << std::endl;
	
	// Bitmap information starts after total frame info and the individual sprite info struct. (Unknown: what are the 4 bytes spriteheaderstart + 104 ??
	bitmapFileStartLoc = spriteHeaderStart + (108 + (12 * pngInformation.imageFrames));
	pngInformation.frameInformation = new frameInformtation[pngInformation.imageFrames];
	if(!ReadFile(pakFile, pngInformation.frameInformation, 12 * pngInformation.imageFrames, &readBytes, NULL)) std::cout << "ReadFile failed: " << GetLastError() << std::endl;
	
	// Read bmp header
	SetFilePointer(pakFile, bitmapFileStartLoc, NULL, FILE_BEGIN);
	if(!ReadFile(pakFile, (char *)&bmpFileHeader, 14, &readBytes, NULL)) std::cout << "ReadFile failed: " << GetLastError() << std::endl; // sizeof(bmpHeader) = 14 
	
	// Resize the pak vector to full pakfile size, so it can be used in the bmp decoding function
	pak.resize(bmpFileHeader.bfSize);

	// Read bmp info into pak vector
	SetFilePointer(pakFile, bitmapFileStartLoc, NULL, FILE_BEGIN);
	if(!ReadFile(pakFile, &pak[0], bmpFileHeader.bfSize, &readBytes, NULL)) std::cout << "ReadFile failed: " << GetLastError() << std::endl;

	CloseHandle(pakFile);

	// Read DIB info header
	LPBITMAPINFO headerDIBInformation = (LPBITMAPINFO)&pak[14];
	return decodeBMP(image, w, h, pak, headerDIBInformation);
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
