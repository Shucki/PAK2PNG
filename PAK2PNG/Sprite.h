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

	std::string spriteName;
	Sprite(int imageTotal, std::string name);
	int SaveToFile(std::string fileName);
	~Sprite();
private:
	Sprite();

};