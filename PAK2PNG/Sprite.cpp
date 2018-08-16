#include "Sprite.h"

using namespace std;
using json = nlohmann::json;

Sprite::Sprite() {

}

Sprite::Sprite(int imageTotal) : imageTotal(imageTotal) {
	pngInfo = new pngInformation[imageTotal];
}

Sprite::~Sprite() {
	free(pngInfo);
}

int Sprite::SaveToFile(string fileName, string frameName) {
	json jPNG;
	jPNG["total"] = imageTotal;
	for (int imageNumber = 0; imageNumber < imageTotal; imageNumber++) {
		string s = string(frameName + to_string(imageNumber));
		jPNG[s]["frameCount"] = pngInfo[imageNumber].imageFrames;
		for (int i = 0; i < pngInfo[imageNumber].imageFrames; i++) {
			jPNG[s]["frames"][to_string(i)]["x"] = pngInfo[imageNumber].frameInformation[i].x;
			jPNG[s]["frames"][to_string(i)]["y"] = pngInfo[imageNumber].frameInformation[i].y;
			jPNG[s]["frames"][to_string(i)]["w"] = pngInfo[imageNumber].frameInformation[i].sizeX;
			jPNG[s]["frames"][to_string(i)]["h"] = pngInfo[imageNumber].frameInformation[i].sizeY;
			jPNG[s]["frames"][to_string(i)]["placementOffset"]["x"] = pngInfo[imageNumber].frameInformation[i].placementX;
			jPNG[s]["frames"][to_string(i)]["placementOffset"]["y"] = pngInfo[imageNumber].frameInformation[i].placementY;
		}
	}
	
	ofstream file(fileName);
	if (file.is_open())
	{
		file << jPNG.dump(4);
		file.close();
		return 0;
	}
	else {
		cout << "Unable to open file " << fileName << endl;
		return 1;
	}

	return 0;
}