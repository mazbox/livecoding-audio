#pragma once

#include "ofMain.h"
#include "ofxMidi.h"
#include "AudioEffect.h"

class testApp : public ofBaseApp, public ofxMidiListener {

public:
	void setup();
	void update();
	void draw();

	void keyPressed  (int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);
	void audioIn( float * input, int bufferSize, int nChannels );
	void audioOut( float * output, int bufferSize, int nChannels );
	void newMidiMessage(ofxMidiEventArgs& eventArgs);

private:
	ofxMidiIn midiIn;
	void doGui(AudioEffect *e);
	void loadDylib(string file);
	void setFiles(vector<string> files);
	void checkSourceForUpdates();
};
