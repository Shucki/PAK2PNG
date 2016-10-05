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

int Sprite::SaveToFile(string fileName) {
	json jPNG;
	for (int imageNumber = 0; imageNumber < imageTotal; imageNumber++) {
		jPNG[to_string(imageNumber)]["Sprite Frames"] = pngInfo[imageNumber].imageFrames;

		for (int i = 0; i < pngInfo[imageNumber].imageFrames; i++) {
			jPNG[to_string(imageNumber)]["Frame Information"][to_string(i)]["x"] = pngInfo[imageNumber].frameInformation[i].x;
			jPNG[to_string(imageNumber)]["Frame Information"][to_string(i)]["y"] = pngInfo[imageNumber].frameInformation[i].y;
			jPNG[to_string(imageNumber)]["Frame Information"][to_string(i)]["sizeX"] = pngInfo[imageNumber].frameInformation[i].sizeX;
			jPNG[to_string(imageNumber)]["Frame Information"][to_string(i)]["sizeY"] = pngInfo[imageNumber].frameInformation[i].sizeY;
			jPNG[to_string(imageNumber)]["Frame Information"][to_string(i)]["placementX"] = pngInfo[imageNumber].frameInformation[i].placementX;
			jPNG[to_string(imageNumber)]["Frame Information"][to_string(i)]["placementY"] = pngInfo[imageNumber].frameInformation[i].placementY;
		}
	}
	jPNG["Image Total"] = imageTotal;
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