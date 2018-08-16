#pragma once
#include <stdlib.h>
#include <sstream>
#include "json.hpp"
#include <fstream>
#include <string>

class Sprite {
public:
	struct frameInformtation
	{
		short x = 0; //startX
		short y = 0; //startY
		short sizeX = 0; //sizex
		short sizeY = 0; //sizey
		short placementX = 0; //correction for placement
		short placementY = 0; //correction for placement
	};

	struct pngInformation
	{
		int imageFrames = 0; // amount of frames on that sprite
		frameInformtation * frameInformation; // information of each frame
	};

	int imageTotal = 0;
	pngInformation * pngInfo;

	Sprite(int imageTotal);
	//int SaveToFile(std::string fileName);

	int SaveToFile(std::string fileName, std::string frameName);
	~Sprite();
private:
	Sprite();

};