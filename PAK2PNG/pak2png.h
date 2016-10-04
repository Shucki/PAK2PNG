#pragma once
struct frameInformtation
{
	short sx; //startX
	short sy; //startY
	short szx; //sizex
	short szy; //sizey
	short pvx; //correction for placement
	short pvy; //correction for placement
};

typedef struct pngInformation
{
	int imageFrames;
	frameInformtation * frameInformation;
} pngInformation;