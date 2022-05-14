/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   EDF.h
 * Author: insus
 *
 * Created on May 18, 2021, 9:01 PM
 */

#ifndef EDF_H
#define EDF_H


#include <cstdlib>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>

 

using namespace std;

#ifndef EDF_DATE
#define EDF_DATE
struct edf_date {
    int year;
    int month;
    string monthName;
    int day;
    int hh;
    int mm;
    int ss;
};
#endif

#ifndef EDF_CHANNEL_INFO
#define EDF_CHANNEL_INFO
struct edf_channel_info {
    std::string label;
    std::string transducer;
    std::string physicalDimension;
    double physicalMin;
    double physicalMax;
    double digitalMin;
    double digitalMax;
    std::string prefilter; 
    int signalSampleCount;
    int bufferOffset;
    std::string reserved;
    vector<double>* signal;
};
#endif

#ifndef ANNOTATION_TYPE_OBJ
#define ANNOTATION_TYPE_OBJ
struct annotation_type_obj {
    double onset;
    double duration;
    std::vector<string> annotationStrings;
};
#endif


#define range_loop(i, b, e, d) for (auto i = (b); i < (e); i+=d)

bool charactersValid(char *str, int len);
std::string convertSpaces(const std::string&);
std::string convertUnderscores(const std::string&);
std::string trim(std::string);

enum class FileType { EDF, EDFPLUS };
enum class Continuity { CONTINUOUS, DISCONTINUOUS };


class EDF {
    
public:
    std::string getPatientCode() { return this->p_code; }
    std::string getPatientName() { return this->p_name; }
    std::string getPatientAdditionalInfo() {   return this->p_additional;    }
    std::string getPatientBirthDate() { return this->p_birthdate; }
    int getPatientGender() { return this->p_gender; }
    
    edf_date getStartDate() { return this->startDate; }
    FileType getFileType() { return this->fileType; }
    Continuity getContinuity() { return this->continuity; }
    
    std::string getRecording() { return this->rs_recording; } 
    std::string getRecordingAdditional() {return this->rs_recordingAdditional;}
    std::string getAdminCode() {return this->rs_adminCode;}
    std::string getTechnician() { return this->rs_adminCode; }
    std::string getEquipment() { return this->rs_equipment; }
    
    int getDataRecordCount() { return this->dataRecordCount; }
    int getDataRecordDuration() { return this->dataRecordDuration; }
    int getSignalCount() { return this->signalCount; }
    
    int getAnnotationIndex() {return this->annotationIndex;}
    edf_channel_info* getChannel(int i) {   return &channels[i];   } 
    std::vector<annotation_type_obj>* getAnnotations() { return annotations; }
    int getDataRecordSize() { return this->dataRecordSize; }
    int getDecordingTime() { return this->recordingTime; }
    
protected:
    std::fstream stream;
    double ADCbits;
    double powerVolts;

    std::string p_code; // patient code
    std::string p_name; // patient name
    std::string p_additional; // additional patient
    int p_gender; // gender
    std::string p_birthdate; // birthdate
    
    edf_date startDate; // date of start
    
    // file type info
    FileType fileType;
    Continuity continuity;
    
    // record string 
    std::string     rs_recording, rs_recordingAdditional, rs_adminCode, rs_technician;
    std::string     rs_equipment;
    
    // data record 
    int dataRecordCount;
    int dataRecordDuration;
    int signalCount;
    
    // channel infos
    edf_channel_info* channels;
    
    int annotationIndex;
    std::vector<annotation_type_obj>* annotations;
    
    int dataRecordSize;
    int recordingTime;
    
public:
    EDF(double adc = 8, double power = 2.5);
    EDF(string file, double adc, double power);
    EDF(const EDF& orig);
    virtual ~EDF();
    
    void read(std::string file);
private:
    void parse_patient(const std::string &pStr);
    void parse_startDate(const string &startDate, const string &time);
    void parse_filetype(const string &rStr);
    void parsePlusRecordInfo(const string &recordStr); 
    void parseStdRecordInfo(const string &recordStr); 
    bool parse_SignalHeaders(std::fstream &in);
    bool signalAvailable(int sigNum) const;
    bool validFileLength(std::fstream &in, int recordSize);
    bool parseAnnotations(std::fstream &in);
    string parseOnset(char* const &tal, int &talOffset, int onsetLength);
    bool validOnset(string& s);
    string parseDuration(char* const &tal, int &talOffset, int onsetLength);
    bool validDuration(string& s);
    string parseAnnotation(char* const &tal, int& talOffset);
    std::vector<double>* parseSignal(std::fstream& in, int signal, double startTime, double length);
    double parseSignalValue(short raw, edf_channel_info channel);
};

#endif /* EDF_H */

