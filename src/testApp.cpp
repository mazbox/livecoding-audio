#include "testApp.h"

#include <dlfcn.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "SimpleGui.h"
#include "AudioEffectData.h"
xmlgui::SimpleGui gui;

ofMutex *mutex;
AudioEffect *eff = NULL;
AudioEffect *silent = NULL;
vector<ofFile> cppFiles;
vector<long> updateTimes;
class SilentEffect: public AudioEffect {
public:
	string getName() {
		return "silence";
	}
	void process(float *ins, float *outs, int length, int numChannels) {
		memset(outs, 0, length*numChannels*sizeof(float));
	}
};

void *livecodeLib = NULL;

void testApp::loadDylib(string file) {
	mutex->lock();
	if(livecodeLib!=NULL) {
		dlclose(livecodeLib);
		delete eff;
		livecodeLib = NULL;
	}
	
	
	livecodeLib = dlopen(file.c_str(), RTLD_LAZY);
	
	if (livecodeLib == NULL) {
		// report error ...
		printf("Error: No dice loading %s\n", file.c_str());
	} else {
		// use the result in a call to dlsym
		printf("Success loading\n");
		
		
		void *ptrFunc = dlsym(livecodeLib, "getAudioEffect");
		if(ptrFunc!=NULL) {
			
			AudioEffect *e = ((AudioEffect *(*)())ptrFunc)();
			e->setup();
			doGui(e);
			eff = e;
			
		} else {
			printf("Couldn't find the getAudioEffect() function\n");
		}
	}
	mutex->unlock();
	
}
void testApp::doGui(AudioEffect *e) {
	gui.clear();
	Parameters params;
	e->initParameters(params);
	for(int i = 0; i < params.params.size(); i++) {
		Parameter *p = params.params[i];
		if(p->type=="slider") {
			SliderParameter *s = (SliderParameter*)p;
			gui.addSlider(s->name, *s->value, s->min, s->max);
		} else if(p->type=="intslider") {
			IntSliderParameter *s = (IntSliderParameter*)p;
			gui.addSlider(s->name, *s->value, s->min, s->max);
		} else if(p->type=="meter") {
			MeterParameter *s = (MeterParameter*)p;
			gui.addMeter(s->name, *s->value);
			
		} else if(p->type=="toggle") {
			ToggleParameter *t = (ToggleParameter*)p;
			gui.addToggle(t->name, *t->value);
		} else if(p->type=="switch") {
			SwitchParameter *s = (SwitchParameter*)p;
			gui.addSegmented(s->name, *s->value, s->options);
		}
	}
}

//--------------------------------------------------------------
void testApp::setup(){
	mutex = new ofMutex();
	silent = new SilentEffect();
	eff = silent;
	ofBackground(0);
	ofSetFrameRate(60);
	//loadDylib(ofToDataPath("../../Osc.dylib", true));
	midiIn.openPort();
	midiIn.addListener(this);
	gui.setEnabled(true);
//	ofSetWindowShape(195+50, 400);
	ofSetWindowShape(1024, 400);
	
	ofSoundStreamSetup(2, 2, this, 44100, 256, 1);
}
float inBuff[8192];

void testApp::audioIn( float * input, int bufferSize, int nChannels ) {
	memcpy(input, inBuff, bufferSize*nChannels*sizeof(float));
}

float outLevelL = 0;
float outLevelR = 0;
void testApp::audioOut( float * output, int bufferSize, int nChannels ) {
	mutex->lock();
	eff->process(inBuff, output, bufferSize, nChannels);
	mutex->unlock();

	for(int i = 0; i < bufferSize; i++) {
		if(ABS(output[i*2])>outLevelL) {
			outLevelL = ABS(output[i*2]);
		} else {
			outLevelL *= 0.999;
		}
		if(ABS(output[i*2+1])>outLevelR) {
			
			outLevelR = ABS(output[i*2+1]);
		} else {
			outLevelR *= 0.999;
		}
	}
}


void testApp::newMidiMessage(ofxMidiEventArgs& eventArgs) {
	if(eventArgs.status==MIDI_NOTE_ON) {
		if(eventArgs.byteTwo>0) {
			eff->noteOn(eventArgs.byteOne, eventArgs.byteTwo);
		} else {
			eff->noteOff(eventArgs.byteOne);
		}
	} else if(eventArgs.status==MIDI_NOTE_OFF) {
		eff->noteOff(eventArgs.byteOne);
	} else if(eventArgs.status==MIDI_CONTROL_CHANGE) {
		eff->cc(eventArgs.byteOne, eventArgs.byteTwo);
	}
}


std::string execute(string cmd) {
	cmd += " 2>&1";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while(!feof(pipe)) {
    	if(fgets(buffer, 128, pipe) != NULL)
    		result += buffer;
    }
    pclose(pipe);
    return result;
}



string linkObjects() {
	string dylibName = ofGetTimestampString()+"-livecode.dylib";
	execute("g++ -arch i386 -dynamiclib -o /tmp/livecode/"+dylibName+" /tmp/livecode/*.o ");
	return dylibName;
}


void clean() {
	execute("rm -Rf /tmp/livecode");
	
	execute("mkdir -p /tmp/livecode");
	AEH h;
	ofstream myfile;
	myfile.open ("/tmp/livecode/AudioEffect.h");
	myfile << 	h.getHeader().c_str();
	myfile.close();
}

long getUpdateTime(ofFile &file) {
	struct stat fileStat;
    if(stat(file.path().c_str(), &fileStat) < 0) {
		printf("Couldn't stat file\n");
		return;
	}

	return fileStat.st_mtime;
}

string lastError = "";
bool compile(int i) {
	string include = ofToDataPath("../../src");
	updateTimes[i] = getUpdateTime(cppFiles[i]);
	string basename = cppFiles[i].getBaseName();
	string cpp = cppFiles[i].path();
//	printf("-------------------\n");
	string result = execute("g++ -arch i386 -c "+cpp+" -I"+include+" -I/tmp/livecode -o /tmp/livecode/"+basename+".o");
//	printf(">>>> %ld '%s'\n",result.size(), result.c_str());
	lastError = result;
	
	return result.size()==0;
}



void compile() {
	clean();
	for(int i =0 ; i < cppFiles.size(); i++) {
		if(!compile(i)) {
			ofBackground(100, 0, 0);
			return;
		}
	}
	ofBackground(0);
}


void testApp::setFiles(vector<string> files) {
	updateTimes.clear();
	cppFiles.clear();
	for(int i = 0; i < files.size(); i++) {
		cppFiles.push_back(ofFile(files[i]));
		updateTimes.push_back(getUpdateTime(cppFiles.back()));
	}
	clean();
	compile();
	string dylibName = linkObjects();
	loadDylib("/tmp/livecode/"+dylibName);
}

void testApp::checkSourceForUpdates() {
	bool compiled = false;
	for(int i = 0; i < cppFiles.size(); i++) {
		if(getUpdateTime(cppFiles[i])!=updateTimes[i]) {
			compile(i);
			compiled = true;
		}
	}
	
	if(compiled) {
		string dylibName = linkObjects();
		loadDylib("/tmp/livecode/"+dylibName);
	}
}


//--------------------------------------------------------------
void testApp::update(){
	if(ofGetFrameNum()%60) {
		checkSourceForUpdates();
	}
}

string wrapLine(string inp, int w) {
	int charWidth = 8;
	string o = "";
	int numCharsPerLine = w/charWidth;
	
	for(int i = 0; i < inp.size(); i++) {
		o += inp[i];
		if(i>0 && i % numCharsPerLine==0) {
			o += "\n";
		}
	}
	return o;
}
string wrap(string inp, int w) {
	
	vector<string> lines = ofSplitString(inp, "\n");
	string out = "";
	for(int i = 0; i <lines.size(); i++) {
		out += wrapLine(lines[i], w) + "\n";
	}
	return out;
}
//--------------------------------------------------------------
void testApp::draw(){
	ofEnableAlphaBlending();
	glColor4f(1, 1, 1, 0.05);
	ofRect(0,0, 195, ofGetHeight());
	
	
	// draw volumes
	ofRectangle r(195, ofGetHeight()-1, 25, -(ofGetHeight()-1));
	ofSetHexColor(0xFFFFFF);
	ofNoFill();
	ofRect(r);
	ofFill();
	ofSetHexColor(0xFF9944);
	ofRectangle rr = r;
	rr.height *= outLevelL;
	ofRect(rr);
	
	r.x += r.width;
	ofSetHexColor(0xFFFFFF);
	ofNoFill();
	ofRect(r);
	ofFill();
	ofSetHexColor(0xFF9944);
	rr = r;
	rr.height *= outLevelR;
	ofRect(rr);
	ofSetHexColor(0xFFFFFF);
	if(ofGetWidth()<1024 && lastError!="") {
		ofSetWindowShape(1024, ofGetHeight());
	} else if(lastError=="" && ofGetWidth()>195+50) {
		ofSetWindowShape(195+50, ofGetHeight());
	}
	if(lastError!="") {
		ofDrawBitmapString(wrap(lastError, ofGetWidth() - 20 - (195+50)), rr.x+rr.width, 20);
	}
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){

}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 
	if(dragInfo.files.size()>0) {
		ofFile f(dragInfo.files[0]);
		vector<string> files;
		if(f.isDirectory()) {
			ofDirectory dir(f.path());
			dir.allowExt("cpp");
			
			int numFiles =dir.listDir();
			for(int i = 0; i < numFiles; i++) {
				files.push_back(dir[i].path());
			}
		} else {
			files.push_back(f.path());
		}
		setFiles(files);

	}
}