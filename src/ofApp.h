#pragma once

#include "ofMain.h"
#include "ofxFFmpegRecorder.h"
#include "ofxGui.h"
#include <filesystem>

class ofApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	void exit();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

private:
	struct PaletteEntry {
		char ch;
		ofColor color;
	};
	struct Palette {
		std::string name;
		std::vector<PaletteEntry> entries;
	};

	void loadConfig();
	void installFallbackPalette();
	void setupGui();
	void reloadFont();
	void onPaletteIndexChanged(int & idx);
	void onFontSizeChanged(int & v);
	void onFontPathChanged(std::string & v);

	void startProcessing(const std::filesystem::path & path);
	void finishProcessing();

	ofVideoPlayer player;
	ofFbo renderFbo;
	ofPixels frameBuf;
	ofxFFmpegRecorder recorder;
	ofTrueTypeFont font;

	std::vector<Palette> palettes;
	std::filesystem::path inputPath;
	std::filesystem::path outputPath;

	bool processing = false;
	int framesWritten = 0;

	ofxPanel gui;
	ofParameter<int> paletteIndex;
	ofParameter<std::string> paletteLabel;
	ofParameter<std::string> fontPath;
	ofParameter<int> fontSize;
	ofParameter<int> cellW;
	ofParameter<int> cellH;
};
