#pragma once

#include "ofMain.h"
#include "ofxAutoTexture.h"

class ofApp : public ofBaseApp{

public:
	void setup();
	void draw();

	ofxAutoTexture tex;
};
