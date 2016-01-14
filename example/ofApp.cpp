#include "ofApp.h"

void ofApp::setup(){
	tex.loadFromFile("zoidberg.png");
}

void ofApp::draw(){
	tex.draw(0,0);
}
