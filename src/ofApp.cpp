#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
	ofBackground(0);
	ofSetVerticalSync(true);
	ofEnableAlphaBlending();

	font.load("Courier New Bold.ttf", 9);

	asciiCharacters = std::string("  ..,,,'''``--_:;^^**\"\"=+<>iv%&xclrs)/){}I?!][1taeo7zjLunT#@JCwfy325Fp6mqSghVd4EgXPGZbYkOA8U$KHDBWNMR0Q");

	ofSetWindowTitle("asciiVideo — drag a video file onto this window");
}

//--------------------------------------------------------------
void ofApp::update() {
	if (!processing) return;

	player.update();

	if (player.getIsMovieDone()) {
		finishProcessing();
	}
}

//--------------------------------------------------------------
void ofApp::draw() {
	if (!processing) {
		ofSetColor(255);
		ofDrawBitmapString("Drag a video file onto this window to render it as ASCII.\nOutput will be written next to the source as <name>_ascii.mp4.",
			20, 40);
		return;
	}

	if (player.isFrameNew()) {
		const ofPixels & src = player.getPixels();
		const int w = src.getWidth();
		const int h = src.getHeight();

		renderFbo.begin();
		ofClear(0, 0, 0, 255);
		ofSetColor(255);
		for (int i = 0; i < w; i += cellW) {
			for (int j = 0; j < h; j += cellH) {
				float lightness = src.getColor(i, j).getLightness();
				int idx = powf(ofMap(lightness, 0, 255, 0, 1), 2.5f) * (asciiCharacters.size() - 1);
				idx = ofClamp(idx, 0, (int)asciiCharacters.size() - 1);
				font.drawString(ofToString(asciiCharacters[idx]), i, j);
			}
		}
		renderFbo.end();

		renderFbo.readToPixels(frameBuf);
		if (frameBuf.getWidth() > 0 && frameBuf.getHeight() > 0) {
			recorder.addFrame(frameBuf);
			framesWritten++;
		}
	}

	ofSetColor(255);
	renderFbo.draw(0, 0);

	std::string hud = "rec  " + ofToString(framesWritten) + " / " + ofToString(player.getTotalNumFrames()) + " frames";
	ofDrawBitmapStringHighlight(hud, 10, 20);
}

//--------------------------------------------------------------
void ofApp::startProcessing(const std::filesystem::path & path) {
	if (processing) {
		ofLogWarning() << "already processing, ignoring drop: " << path;
		return;
	}

	inputPath = path;
	if (!player.load(inputPath.string())) {
		ofLogError() << "failed to load video: " << inputPath;
		return;
	}

	const int w = player.getWidth();
	const int h = player.getHeight();
	if (w <= 0 || h <= 0) {
		ofLogError() << "video reports zero dimensions: " << inputPath;
		return;
	}

	outputPath = inputPath.parent_path() / (inputPath.stem().string() + "_ascii.mp4");

	renderFbo.allocate(w, h, GL_RGB);
	ofSetWindowShape(w, h);

	float fps = 30.0f;
	if (player.getTotalNumFrames() > 0 && player.getDuration() > 0.0f) {
		fps = player.getTotalNumFrames() / player.getDuration();
	}

	recorder.setup(true, false, glm::vec2(w, h), fps);
	recorder.setOverWrite(true);
	recorder.setFFmpegPathToAddonsPath();
	recorder.setInputPixelFormat(OF_IMAGE_COLOR);
	recorder.setOutputPath(outputPath.string());
	recorder.startCustomRecord();

	framesWritten = 0;
	player.play();
	processing = true;

	ofLogNotice() << "processing " << inputPath << " (" << w << "x" << h << " @ " << fps << " fps) -> " << outputPath;
}

//--------------------------------------------------------------
void ofApp::finishProcessing() {
	if (!processing) return;
	recorder.stop();
	player.stop();
	player.close();
	processing = false;
	ofLogNotice() << "wrote " << framesWritten << " frames to " << outputPath;
	ofSetWindowTitle("asciiVideo — done. Drag another video to process.");
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {
	if (dragInfo.files.empty()) return;
	startProcessing(std::filesystem::path(dragInfo.files.front()));
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	if (key == 'q' && processing) {
		ofLogNotice() << "user requested early stop";
		finishProcessing();
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {}
void ofApp::mouseMoved(int x, int y) {}
void ofApp::mouseDragged(int x, int y, int button) {}
void ofApp::mousePressed(int x, int y, int button) {}
void ofApp::mouseReleased(int x, int y, int button) {}
void ofApp::mouseEntered(int x, int y) {}
void ofApp::mouseExited(int x, int y) {}
void ofApp::windowResized(int w, int h) {}
void ofApp::gotMessage(ofMessage msg) {}
