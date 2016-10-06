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
#include <direct.h>
#include "dirent.h"
#include <sstream>
#include "Sprite.h"
#include "main.h"

using namespace std;

ostringstream logstream;
//beautiful hardcoded filenames, potential for read from textfile
bool isAlphaSprite(string spriteName) {
	if ((spriteName.compare("Tile388-402.pak") == 0) ||
		(spriteName.compare("Tile403-405.pak") == 0) ||
		(spriteName.compare("Tile406-421.pak") == 0) ||
		(spriteName.compare("Tile422-429.pak") == 0) ||
		(spriteName.compare("Tile430-443.pak") == 0) ||
		(spriteName.compare("Tile444-444.pak") == 0) ||
		(spriteName.compare("Tile445-461.pak") == 0) ||
		(spriteName.compare("Tile462-464.pak") == 0) ||
		(spriteName.compare("Tile462-473.pak") == 0) ||
		(spriteName.compare("Tile462-475.pak") == 0) ||
		(spriteName.compare("Tile474-478.pak") == 0) ||
		(spriteName.compare("Tile474-488.pak") == 0) ||
		(spriteName.compare("Tile479-488.pak") == 0) ||
		//(spriteName.compare("Tile489-522.pak") == 0) ||
		(spriteName.compare("Tile523-530.pak") == 0) ||
		(spriteName.compare("Tile523-532.pak") == 0) ||
		(spriteName.compare("Tile531-540.pak") == 0) ||
		(spriteName.compare("Tile541-545.pak") == 0) ||

		(spriteName.compare("maptiles1.pak") == 0) ||
		(spriteName.compare("maptiles2.pak") == 0) ||
		(spriteName.compare("maptiles3.pak") == 0) ||
		(spriteName.compare("maptiles4.pak") == 0) ||
		(spriteName.compare("maptiles5.pak") == 0) ||
		(spriteName.compare("maptiles6.pak") == 0) ||
		(spriteName.compare("maptiles353-361.pak") == 0) ||
		
		(spriteName.compare("ugtile.pak") == 0))
		return false;
	return true;
}

//returns 0 if all went ok, non-0 if error
//output image is always given in RGBA (with alpha channel), even if it's a BMP without alpha channel
unsigned decodeBMP(vector<unsigned char>& image, unsigned& w, unsigned& h, const vector<unsigned char>& bmp, bool isAlphaSprite = false)
{
	static const unsigned MINHEADER = 54; //minimum BMP header size
	RGBQUAD alphaColor;
	if (bmp.size() < MINHEADER) return -1;
	if (bmp[0] != 'B' || bmp[1] != 'M') return 1; //It's not a BMP file if it doesn't start with marker 'BM'
	unsigned pixeloffset = bmp[10] + 256 * bmp[11] + 65536 * bmp[12] + bmp[13] * 16777216; //where the pixel data starts
													//read width and height from BMP header
	w = bmp[18] + bmp[19] * 256;
	h = bmp[22] + bmp[23] * 256;
	//read number of channels from BMP header
	if (bmp[28] != 24 && bmp[28] != 32 && bmp[28] != 8) {
		logstream << "Supports only 8/24/32bit bmp, given bmp is: " << to_string((unsigned char)bmp[28]) << " bit" << endl;
		cout << "Supports only 8/24/32bit bmp, given bmp is: " << to_string((unsigned char)bmp[28]) << " bit" << endl;
		return 2; //only 24-bit and 32-bit BMPs are supported. -> Added 8-bit
	}
	unsigned numChannels = bmp[28] / 8;

	//The amount of scanline bytes is width of image times channels, with extra bytes added if needed
	//to make it a multiple of 4 bytes.
	unsigned scanlineBytes = w * numChannels;
	if (scanlineBytes % 4 != 0) scanlineBytes = (scanlineBytes / 4) * 4 + 4;

	unsigned dataSize = scanlineBytes * h;
	if (bmp.size() < dataSize + pixeloffset) return 3; //BMP file too small to contain all pixels

	// Read DIB Header
	LPBITMAPINFO headerDIBInformation = (LPBITMAPINFO)&bmp[14];

	image.resize(w * h * 4);
	if (numChannels == 1) alphaColor = headerDIBInformation->bmiColors[bmp[pixeloffset + ((h-1) * scanlineBytes)]];
	else {
		alphaColor.rgbRed = bmp[pixeloffset + ((h - 1) * scanlineBytes) + 2];
		alphaColor.rgbGreen = bmp[pixeloffset + ((h - 1) * scanlineBytes) + 1];
		alphaColor.rgbBlue = bmp[pixeloffset + ((h - 1) * scanlineBytes) + 0];
	}
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
				if (isAlphaSprite && (bmp[bmpos + 0] == alphaColor.rgbBlue) &&
					(bmp[bmpos + 2] == alphaColor.rgbRed) &&
					(bmp[bmpos + 1] == alphaColor.rgbGreen)) {
					image[newpos + 3] = 0; // Alpha = 0 means transparent
				}
				else image[newpos + 3] = 255;
			}
			else if (numChannels == 1) // Read the color out of the color table
			{
				image[newpos + 0] = headerDIBInformation->bmiColors[bmp[bmpos]].rgbRed; //R
				image[newpos + 1] = headerDIBInformation->bmiColors[bmp[bmpos]].rgbGreen; //G
				image[newpos + 2] = headerDIBInformation->bmiColors[bmp[bmpos]].rgbBlue; //B
				if (isAlphaSprite && (headerDIBInformation->bmiColors[bmp[bmpos]].rgbBlue == alphaColor.rgbBlue) &&
					(headerDIBInformation->bmiColors[bmp[bmpos]].rgbRed == alphaColor.rgbRed) &&
					(headerDIBInformation->bmiColors[bmp[bmpos]].rgbGreen == alphaColor.rgbGreen)) {
					image[newpos + 3] = 0; // Alpha = 0 means transparent
				}
				else image[newpos + 3] = 255;				
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
* Method:		decodePAK
* param image:	output image
* param w:		image width
* param h:		image height
* pathName:		filepath of pak file
* numberOfFile:	which bmp file inside the PAK file to extract
* return:		non-0 if error
*/
unsigned decodePAK(vector<unsigned char>& image, unsigned& w, unsigned& h, char pathName[28], HANDLE pakFile, int numberOfFile, Sprite::pngInformation &pngInformation)
{
	DWORD  readBytes; // Bytes read after readfile, usable for debugging information
	int spriteHeaderStart = 0; // Every bmp has a sprite header above
	BITMAPFILEHEADER bmpFileHeader; // File header of the bmp we read
	int numberOfFiles = 0;

	long bitmapFileStartLoc;
	vector<unsigned char> bmp;

	// Read frame information
	pngInformation.imageFrames = 0;

	// Go to sprite header start bytes and read them
	SetFilePointer(pakFile, 24 + numberOfFile * 8, NULL, FILE_BEGIN);
	if (!ReadFile(pakFile, &spriteHeaderStart, 4, &readBytes, NULL)) cout << "ReadFile failed: " << GetLastError() << endl;

	// Sprite header start + 100 is where the information of how many frames are in the bitmap is stored.
	SetFilePointer(pakFile, spriteHeaderStart + 100, NULL, FILE_BEGIN);
	if (!ReadFile(pakFile, &(pngInformation.imageFrames), 4, &readBytes, NULL)) cout << "ReadFile failed: " << GetLastError() << endl;

	// Bitmap information starts after total frame info and the individual sprite info struct. (Unknown: what are the 4 bytes spriteheaderstart + 104 ??
	bitmapFileStartLoc = spriteHeaderStart + (108 + (12 * pngInformation.imageFrames));
	pngInformation.frameInformation = new Sprite::frameInformtation[pngInformation.imageFrames];
	if (!ReadFile(pakFile, pngInformation.frameInformation, 12 * pngInformation.imageFrames, &readBytes, NULL)) cout << "ReadFile failed: " << GetLastError() << endl;

	// Read bmp header
	SetFilePointer(pakFile, bitmapFileStartLoc, NULL, FILE_BEGIN);
	if (!ReadFile(pakFile, (char *)&bmpFileHeader, 14, &readBytes, NULL)) cout << "ReadFile failed: " << GetLastError() << endl; // sizeof(bmpHeader) = 14 

	// Resize the bmp vector to bmp size (found in header), if size errors occur check here it is stated online this might be incorrect for some files.
	bmp.resize(bmpFileHeader.bfSize);

	// Read bmp info into pak vector
	SetFilePointer(pakFile, bitmapFileStartLoc, NULL, FILE_BEGIN);
	if (!ReadFile(pakFile, &bmp[0], bmpFileHeader.bfSize, &readBytes, NULL)) cout << "ReadFile failed: " << GetLastError() << endl;

	return decodeBMP(image, w, h, bmp, isAlphaSprite(pathName));
}

/**
param:	pakPathName: name of .PAK file to unpack
*/
void unpackEntirePakFile(char pakPathName[260]) {
	HANDLE pakFile;
	int numberOfFiles = 0;
	DWORD readBytes = 0;
	string fileDest;

	fileDest = pakPathName;

	// Create destination folder
	fileDest.erase(fileDest.end() - 4, fileDest.end());
	_mkdir(fileDest.c_str());

	// Create pakfile handle and check for success
	pakFile = CreateFileA(pakPathName, GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
	if (pakFile == INVALID_HANDLE_VALUE) {
		cout << "CreateFile Error: " << GetLastError() << endl;
		return;
	}
	if (pakFile == NULL) {
		cout << "pakFile is null" << endl;
		return;
	}

	// Check for how many files there are
	SetFilePointer(pakFile, 24, NULL, FILE_BEGIN);
	if (!ReadFile(pakFile, &numberOfFiles, 4, &readBytes, NULL)) cout << "ReadFile failed: " << GetLastError() << endl;

	numberOfFiles = (numberOfFiles - 24) / 8;
	Sprite s(numberOfFiles);

	for (int i = 0; i < numberOfFiles; i++) {
		vector<unsigned char> image;
		unsigned w, h;
		ostringstream oss;

		unsigned error = decodePAK(image, w, h, pakPathName, pakFile, i, s.pngInfo[i]);
		if (error)
		{
			cout << "PAK decoding error " << error << endl;
			logstream << "Pak decoding error " << error << " on file: " << pakPathName << endl;
			return;
		}

		vector<unsigned char> png;
		error = lodepng::encode(png, image, w, h);

		if (error)
		{
			cout << "PNG encoding error " << error << ": " << lodepng_error_text(error) << endl;
			return;
		}
		oss << fileDest << "\\" << fileDest << i << ".png";
		lodepng::save_file(png, oss.str().c_str());
	}
	CloseHandle(pakFile);
	ostringstream oss;
	oss << fileDest << "\\" << fileDest << ".json";
	s.SaveToFile(oss.str());
	return;
}

int executeForEntireDirectory() {
	DIR *dir;
	struct dirent *ent;
	char * fileType;
	int packFiles = 0;

	if ((dir = opendir(".")) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			if ((fileType = strstr(ent->d_name, ".pak")) != NULL) {
				cout << "Unpacking: " << ent->d_name << endl;
				unpackEntirePakFile(ent->d_name);
				packFiles++;
			}
		}
		closedir(dir);
	}
	else {
		return -1;
	}
	return packFiles;
}

int main(int argc, char *argv[]) {
	int result = executeForEntireDirectory();
	if (result == 0) {
		cout << "No .PAK file found in current directory. Please run this program in the sprites folder" << endl;
	}
	else if (result == -1) {
		cout << "Could no open directory, Permissions?" << endl;
	}
	else {
		cout << "Success, unpacked " << result << " .PAK files." << endl;
	}
	ofstream file("pak2pnglog.log");
	if (file.is_open())
	{
		file << logstream.str();
		file.close();
	}

}
