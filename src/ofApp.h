#pragma once

#include "ofMain.h"
#include "ofxFFmpegRecorder.h"
#include <filesystem>

class ofApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();

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
	void startProcessing(const std::filesystem::path & path);
	void finishProcessing();

	ofVideoPlayer player;
	ofFbo renderFbo;
	ofPixels frameBuf;
	ofxFFmpegRecorder recorder;
	ofTrueTypeFont font;

	std::string asciiCharacters;
	std::filesystem::path inputPath;
	std::filesystem::path outputPath;

	bool processing = false;
	int framesWritten = 0;

	static constexpr int cellW = 7;
	static constexpr int cellH = 9;
};
