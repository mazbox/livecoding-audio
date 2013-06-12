
#include <string>
#include <vector>
#include <sstream>
using namespace std;
/*
 
 g++ -arch i386 effect/Osc.cpp -c -I src
 g++ -arch i386 -I. -dynamiclib -o Osc.dylib Osc.o
 rm Osc.o
 */


class Parameter;

class Parameters {
public:
	void addSlider(string name, int &value, int from = 0, int to = 127);
	void addSlider(string name, float &value, float from = 0, float to = 1);
	void addMeter(string name, float &value);
	
	void addToggle(string name, bool &value);
	void addSwitch(string name, int &value, string options);
	vector<Parameter*> params;
};


class AudioEffect {
public:
	virtual void setup() {};
	virtual void initParameters(Parameters &params) {}
	// midi
	virtual void noteOn(int note, int vel) {}
	virtual void noteOff(int note) {}
	virtual void cc(int cc, int value) {}
	
	
	// audio
	virtual void process(float *ins, float *outs, int length, int numChannels) = 0;		
};


class Parameter {
public:
	string type;
	string name;
};

class SliderParameter: public Parameter {
public:
	SliderParameter() { type = "slider"; }
	float min;
	float max;
	float *value;
};

class IntSliderParameter: public Parameter {
public:
	IntSliderParameter() { type = "intslider"; }
	int min;
	int max;
	int *value;
};

class MeterParameter: public Parameter {
public:
	MeterParameter() { type = "meter"; }
	float *value;
};

class ToggleParameter: public Parameter {
public:
	ToggleParameter() { type = "toggle"; }
	bool *value;
};

class SwitchParameter: public Parameter {
public:
	SwitchParameter() { type = "switdh"; }
	vector<string> options;
	int *value;
};

void Parameters::addSlider(string name, int &value, int from, int to) {
	IntSliderParameter *s = new IntSliderParameter();
	s->name = name;
	s->min = from;
	s->max = to;
	s->value = &value;
	params.push_back(s);
}

void Parameters::addSlider(string name, float &value, float from, float to) {
	SliderParameter *s = new SliderParameter();
	s->name = name;
	s->min = from;
	s->max = to;
	s->value = &value;
	params.push_back(s);
}


void Parameters::addMeter(string name, float &value) {
	MeterParameter *s = new MeterParameter();
	s->name = name;
	s->value = &value;
	params.push_back(s);
}

void Parameters::addToggle(string name, bool &value) {
	ToggleParameter *s = new ToggleParameter();
	s->name = name;
	s->value = &value;
	params.push_back(s);
}

inline std::vector<std::string> &___split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}



inline std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    return ___split(s, delim, elems);
}


void Parameters::addSwitch(string name, int &value, string options) {
	SwitchParameter *s = new SwitchParameter();
	s->name = name;
	s->value = &value;
	s->options = split(options, '|');
	params.push_back(s);
}


